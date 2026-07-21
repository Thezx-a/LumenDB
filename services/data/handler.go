package data

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/gin-gonic/gin"
)

// Service 是 Data 服务.
type Service struct {
	store *Store
}

// NewService 创建 data 服务.
func NewService(store *Store) *Service {
	return &Service{store: store}
}

// PutRequest POST /api/data/kv 请求体.
type PutRequest struct {
	Key   string `json:"key" binding:"required"`
	Value string `json:"value" binding:"required"`
}

// Put POST /api/data/kv — 写入 KV.
//   - body: {"key":"...","value":"..."}
//   - 200: 写入成功
//   - 400: 参数缺失
func (s *Service) Put(c *gin.Context) {
	var req PutRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	s.store.Put(req.Key, req.Value)
	c.JSON(http.StatusOK, gin.H{"ok": true})
}

// Get GET /api/data/kv?key=xxx — 读取 KV.
//   - 200: 返回 {"key":"...","value":"..."}
//   - 404: key 不存在
func (s *Service) Get(c *gin.Context) {
	key := c.Query("key")
	if key == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "missing key"})
		return
	}
	v, ok := s.store.Get(key)
	if !ok {
		c.JSON(http.StatusNotFound, gin.H{"error": "key not found"})
		return
	}
	c.JSON(http.StatusOK, KVPair{Key: key, Value: v})
}

// Delete DELETE /api/data/kv?key=xxx — 删除 KV.
//   - 200: 删除成功 (幂等, key 不存在也返回 200)
func (s *Service) Delete(c *gin.Context) {
	key := c.Query("key")
	if key == "" {
		c.JSON(http.StatusBadRequest, gin.H{"error": "missing key"})
		return
	}
	s.store.Delete(key) // 幂等: 不存在不报错
	c.JSON(http.StatusOK, gin.H{"ok": true})
}

// Scan GET /api/data/scan?start=xxx&end=xxx — SSE 流式扫描.
// 响应格式 (text/event-stream):
//
//	data: {"key":"k1","value":"v1"}
//
//	data: {"key":"k2","value":"v2"}
//
//	event: end
//	data: {"count":2}
//
// 客户端 (web/components/data/page.tsx 和 client-go/titan/client.go)
// 按 "\n\n" 切帧, 解析 "data:" 行为 JSON, "event: end" 结束.
func (s *Service) Scan(c *gin.Context) {
	start := c.Query("start")
	end := c.Query("end")

	// SSE 头
	c.Writer.Header().Set("Content-Type", "text/event-stream")
	c.Writer.Header().Set("Cache-Control", "no-cache")
	c.Writer.Header().Set("Connection", "keep-alive")
	c.Writer.Header().Set("X-Accel-Buffering", "no") // 禁用 Nginx 缓冲 (Gateway 反代场景)

	flusher, ok := c.Writer.(http.Flusher)
	if !ok {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "streaming unsupported"})
		return
	}

	items := s.store.Scan(start, end)
	count := 0
	for _, kv := range items {
		buf, err := json.Marshal(kv)
		if err != nil {
			continue
		}
		fmt.Fprintf(c.Writer, "data: %s\n\n", buf)
		flusher.Flush()
		count++
	}
	// 结束帧: event: end + data: {"count":N}
	endPayload, _ := json.Marshal(map[string]int{"count": count})
	fmt.Fprintf(c.Writer, "event: end\ndata: %s\n\n", endPayload)
	flusher.Flush()
}

// RegisterRoutes 注册路由 (被 main.go 调用).
func (s *Service) RegisterRoutes(r *gin.Engine) {
	r.GET("/healthz", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"status":  "ok",
			"service": "data",
			"keys":    s.store.Size(),
		})
	})
	r.POST("/api/data/kv", s.Put)
	r.GET("/api/data/kv", s.Get)
	r.DELETE("/api/data/kv", s.Delete)
	r.GET("/api/data/scan", s.Scan)
}
