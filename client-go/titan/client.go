package titan

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"math/rand"
	"net/http"
	"net/url"
	"time"
)

// Client 是 TitanKV Go SDK 客户端.
type Client struct {
	gatewayURL string
	apiKey     string
	http       *http.Client
	maxRetries int
}

// Option 可选配置.
type Option func(*Client)

// WithMaxRetries 设置最大重试次数 (默认 3).
func WithMaxRetries(n int) Option {
	return func(c *Client) { c.maxRetries = n }
}

// WithHTTPClient 自定义 HTTP 客户端 (如需代理/超时).
func WithHTTPClient(h *http.Client) Option {
	return func(c *Client) { c.http = h }
}

// New 创建客户端.
//
//	gatewayURL: Gateway 地址, 如 http://localhost:8080
//	apiKey: API Key (机器对机器), 或空字符串 (用 JWT 时单独调 Login)
func New(gatewayURL, apiKey string, opts ...Option) *Client {
	c := &Client{
		gatewayURL: gatewayURL,
		apiKey:     apiKey,
		http:       &http.Client{Timeout: 10 * time.Second},
		maxRetries: 3,
	}
	for _, o := range opts {
		o(c)
	}
	return c
}

// Login 用用户名密码换 token.
func (c *Client) Login(ctx context.Context, username, password string) (*LoginResponse, error) {
	body, _ := json.Marshal(map[string]string{
		"username": username,
		"password": password,
	})
	resp, err := c.do(ctx, http.MethodPost, "/api/auth/login", body, false)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return nil, mapError(resp.StatusCode, resp.Body)
	}
	var out LoginResponse
	if err := json.NewDecoder(resp.Body).Decode(&out); err != nil {
		return nil, fmt.Errorf("decode: %w", err)
	}
	return &out, nil
}

// Put 写入 KV.
func (c *Client) Put(ctx context.Context, key, value string) error {
	body, _ := json.Marshal(map[string]string{"key": key, "value": value})
	resp, err := c.do(ctx, http.MethodPost, "/api/data/kv", body, false)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return mapError(resp.StatusCode, resp.Body)
	}
	return nil
}

// Get 读取 KV. 重试 (幂等).
func (c *Client) Get(ctx context.Context, key string) ([]byte, error) {
	var lastErr error
	for i := 0; i <= c.maxRetries; i++ {
		resp, retryable, err := c.doGet(ctx, key)
		if err == nil {
			defer resp.Body.Close()
			if resp.StatusCode == http.StatusOK {
				return io.ReadAll(resp.Body)
			}
			lastErr = mapError(resp.StatusCode, resp.Body)
			if !isRetryableStatus(resp.StatusCode) {
				return nil, lastErr
			}
		} else {
			lastErr = err
			if !retryable {
				return nil, err
			}
		}
		// 指数退避 + jitter (避免 thundering herd)
		backoff := time.Duration(1<<uint(i)) * 100 * time.Millisecond
		jitter := time.Duration(rand.Intn(50)) * time.Millisecond
		select {
		case <-ctx.Done():
			return nil, ctx.Err()
		case <-time.After(backoff + jitter):
		}
	}
	return nil, lastErr
}

// Delete 删除 KV.
func (c *Client) Delete(ctx context.Context, key string) error {
	resp, err := c.do(ctx, http.MethodDelete, "/api/data/kv?key="+url.QueryEscape(key), nil, false)
	if err != nil {
		return err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return mapError(resp.StatusCode, resp.Body)
	}
	return nil
}

// Scan 流式扫描. 调用方传入回调, 每条 KV 调一次.
// 客户端解析 SSE 流, 调用方无需关心格式.
func (c *Client) Scan(ctx context.Context, start, end string, fn func(KVPair) error) (int, error) {
	u := fmt.Sprintf("/api/data/scan?start=%s&end=%s", url.QueryEscape(start), url.QueryEscape(end))
	resp, err := c.do(ctx, http.MethodGet, u, nil, false)
	if err != nil {
		return 0, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return 0, mapError(resp.StatusCode, resp.Body)
	}
	// 简化 SSE 解析: 读 "data: <json>\n\n"
	buf := make([]byte, 0, 4096)
	tmp := make([]byte, 1024)
	count := 0
	for {
		n, err := resp.Body.Read(tmp)
		if n > 0 {
			buf = append(buf, tmp[:n]...)
			for {
				// 找 "\n\n"
				idx := bytes.Index(buf, []byte("\n\n"))
				if idx < 0 {
					break
				}
				line := string(buf[:idx])
				buf = buf[idx+2:]
				if len(line) > 6 && line[:6] == "data: " {
					var pair KVPair
					if json.Unmarshal([]byte(line[6:]), &pair) == nil {
						if err := fn(pair); err != nil {
							return count, err
						}
						count++
					}
				}
			}
		}
		if err == io.EOF {
			break
		}
		if err != nil {
			return count, err
		}
	}
	return count, nil
}

// GetMetrics 获取当前指标快照.
func (c *Client) GetMetrics(ctx context.Context) (*Metrics, error) {
	resp, err := c.do(ctx, http.MethodGet, "/api/observability/metrics", nil, false)
	if err != nil {
		return nil, err
	}
	defer resp.Body.Close()
	if resp.StatusCode != http.StatusOK {
		return nil, mapError(resp.StatusCode, resp.Body)
	}
	var m Metrics
	if err := json.NewDecoder(resp.Body).Decode(&m); err != nil {
		return nil, err
	}
	return &m, nil
}

// doGet 内部: 发起 GET, 返回 resp + retryable flag.
func (c *Client) doGet(ctx context.Context, key string) (*http.Response, bool, error) {
	resp, err := c.do(ctx, http.MethodGet, "/api/data/kv?key="+url.QueryEscape(key), nil, false)
	if err != nil {
		return nil, true, err // 网络错误可重试
	}
	return resp, isRetryableStatus(resp.StatusCode), nil
}

// do 发起 HTTP 请求. retryable=false 表示不重试.
func (c *Client) do(ctx context.Context, method, path string, body []byte, retryable bool) (*http.Response, error) {
	req, err := http.NewRequestWithContext(ctx, method, c.gatewayURL+path, bytes.NewReader(body))
	if err != nil {
		return nil, err
	}
	if body != nil {
		req.Header.Set("Content-Type", "application/json")
	}
	if c.apiKey != "" {
		req.Header.Set("X-API-Key", c.apiKey)
	}
	return c.http.Do(req)
}

// isRetryableStatus 判断 HTTP 状态码是否可重试.
//   - 5xx: 服务端错误, 可重试
//   - 429: 限流, 可重试
//   - 4xx (其他): 客户端错误, 不重试
func isRetryableStatus(code int) bool {
	return code >= 500 || code == http.StatusTooManyRequests
}

// mapError HTTP 状态码 → typed error.
func mapError(code int, body io.Reader) error {
	switch code {
	case http.StatusNotFound:
		return ErrNotFound
	case http.StatusConflict:
		return ErrConflict
	case http.StatusUnauthorized:
		return ErrUnauthorized
	case http.StatusForbidden:
		return ErrForbidden
	case http.StatusTooManyRequests:
		return ErrRateLimited
	case http.StatusBadRequest:
		return ErrInvalidArgument
	default:
		if code >= 500 {
			return ErrInternal
		}
		return fmt.Errorf("titan: http %d", code)
	}
}
