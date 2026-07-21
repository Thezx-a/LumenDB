package middleware

import (
	"log"
	"net/http"
	"runtime/debug"

	"github.com/gin-gonic/gin"
)

// Recover 捕获 handler 中的 panic, 返回 500, 防止进程崩溃.
// 必须在业务 handler 之前注册 (内层).
func Recover() gin.HandlerFunc {
	return func(c *gin.Context) {
		defer func() {
			if r := recover(); r != nil {
				log.Printf("[PANIC] rid=%s err=%v\n%s",
					GetRequestID(c), r, debug.Stack())
				c.AbortWithStatusJSON(http.StatusInternalServerError, gin.H{
					"error":      "internal server error",
					"request_id": GetRequestID(c),
				})
			}
		}()
		c.Next()
	}
}
