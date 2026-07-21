package auth

import (
	"bytes"
	"context"
	"encoding/json"
	"fmt"
	"io"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
)

// Client 是 Gateway 调用 Auth 服务的客户端.
// Gateway 把 /api/auth/* 路由直接转发 (HTTP 反向代理也可, 这里用客户端
// 便于在 Gateway 内做错误转换 / 协议适配).
type Client struct {
	baseURL string
	http    *http.Client
}

func NewClient(baseURL string) *Client {
	return &Client{
		baseURL: baseURL,
		http:    &http.Client{Timeout: 3 * time.Second},
	}
}

// RegisterHandler Gateway 端的注册 handler: 转发到 Auth 服务.
func (c *Client) RegisterHandler(gc *gin.Context) { c.proxy(gc, "/api/auth/register") }
func (c *Client) LoginHandler(gc *gin.Context)    { c.proxy(gc, "/api/auth/login") }
func (c *Client) RefreshHandler(gc *gin.Context)  { c.proxy(gc, "/api/auth/refresh") }
func (c *Client) LogoutHandler(gc *gin.Context)   { c.proxy(gc, "/api/auth/logout") }
func (c *Client) IssueAPIKeyHandler(gc *gin.Context) {
	c.proxy(gc, "/api/auth/apikey")
}

// VerifyAPIKey 通过 HTTP 调用 Auth 服务的 /api/auth/verify-apikey.
// Gateway 在 middleware/auth.go 中校验 API Key 时调用.
func (c *Client) VerifyAPIKey(ctx context.Context, key string) (userID, role string, ok bool) {
	body, _ := json.Marshal(map[string]string{"api_key": key})
	req, err := http.NewRequestWithContext(ctx, http.MethodPost,
		c.baseURL+"/api/auth/verify-apikey", bytes.NewReader(body))
	if err != nil {
		return "", "", false
	}
	req.Header.Set("Content-Type", "application/json")
	resp, err := c.http.Do(req)
	if err != nil || resp.StatusCode != http.StatusOK {
		if resp != nil {
			resp.Body.Close()
		}
		return "", "", false
	}
	defer resp.Body.Close()
	var out struct {
		UserID string `json:"user_id"`
		Role   string `json:"role"`
	}
	if err := json.NewDecoder(resp.Body).Decode(&out); err != nil {
		return "", "", false
	}
	return out.UserID, out.Role, true
}

// proxy 通用转发: 把 Gateway 收到的请求转发到 Auth 服务对应 path.
func (c *Client) proxy(gc *gin.Context, path string) {
	url := c.baseURL + path
	req, err := http.NewRequestWithContext(gc.Request.Context(), gc.Request.Method, url, gc.Request.Body)
	if err != nil {
		gc.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	req.Header = gc.Request.Header.Clone()
	resp, err := c.http.Do(req)
	if err != nil {
		gc.JSON(http.StatusBadGateway, gin.H{"error": fmt.Sprintf("auth service: %v", err)})
		return
	}
	defer resp.Body.Close()
	body, _ := io.ReadAll(resp.Body)
	gc.Data(resp.StatusCode, resp.Header.Get("Content-Type"), body)
}
