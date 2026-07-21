package handler

import (
	"net/http"

	"github.com/gin-gonic/gin"
)

// Ping 简单健康检查, 验证中间件链.
// GET /ping → 200 {"pong": true, "request_id": ...}
func Ping(c *gin.Context) {
	c.JSON(http.StatusOK, gin.H{
		"pong":       true,
		"request_id": c.GetString("request_id"),
	})
}

// Healthz 健康检查, 给 K8s liveness/readiness probe 用.
// GET /healthz → 200 {"status":"ok","version":...}
func Healthz(version string) gin.HandlerFunc {
	return func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{
			"status":  "ok",
			"version": version,
		})
	}
}
