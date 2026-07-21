// Package middleware implements the Gin middleware chain for TitanKV Gateway.
// 洋葱模型顺序: RequestID → Logger → Recover → RateLimit → Auth → RBAC → Handler
package middleware

import (
	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
)

const requestIDHeader = "X-Request-ID"
const requestIDKey = "request_id"

// RequestID 注入 X-Request-ID, 没有则生成 UUID.
// 必须最先注册, 后续所有日志都能关联同一 ID, 便于全链路追踪.
func RequestID() gin.HandlerFunc {
	return func(c *gin.Context) {
		rid := c.GetHeader(requestIDHeader)
		if rid == "" {
			rid = uuid.NewString()
		}
		c.Set(requestIDKey, rid)
		c.Writer.Header().Set(requestIDHeader, rid)
		c.Next()
	}
}

// GetRequestID 从 ctx 取 request_id, 用于其他中间件/handler.
func GetRequestID(c *gin.Context) string {
	if v, ok := c.Get(requestIDKey); ok {
		if s, ok := v.(string); ok {
			return s
		}
	}
	return ""
}
