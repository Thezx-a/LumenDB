package middleware

import (
	"log"
	"time"

	"github.com/gin-gonic/gin"
)

// Logger 在 Recover 之前注册: 即使 panic 也能记录请求信息.
// 进入时记请求阶段, c.Next() 之后记响应阶段 (状态码/耗时).
func Logger() gin.HandlerFunc {
	return func(c *gin.Context) {
		start := time.Now()
		path := c.Request.URL.Path
		method := c.Request.Method
		rid := GetRequestID(c)

		c.Next()

		latency := time.Since(start)
		status := c.Writer.Status()
		log.Printf("[GIN] rid=%s %s %s %d %v ip=%s",
			rid, method, path, status, latency, c.ClientIP())
	}
}
