package observability

import (
	"encoding/json"
	"fmt"
	"net/http"

	"github.com/gin-gonic/gin"
)

// RegisterRoutes 注册 Observability 服务路由.
func RegisterRoutes(r *gin.Engine, svc *Service) {
	r.GET("/healthz", func(c *gin.Context) {
		c.JSON(http.StatusOK, svc.Health())
	})

	// 当前指标快照 (RSC 首屏用)
	r.GET("/api/observability/metrics", func(c *gin.Context) {
		c.JSON(http.StatusOK, svc.Current())
	})

	// SSE 实时指标流 (控制台仪表盘用)
	r.GET("/api/metrics/stream", streamMetrics(svc))
}

// streamMetrics SSE 推送. 每秒一条 metrics.
//
// SSE 关键点:
//   - Content-Type: text/event-stream
//   - X-Accel-Buffering: no 关闭 Nginx 缓冲
//   - 每条立即 flusher.Flush()
//   - 客户端断开 (ctx.Done) 时及时 Unsubscribe
func streamMetrics(svc *Service) gin.HandlerFunc {
	return func(c *gin.Context) {
		c.Writer.Header().Set("Content-Type", "text/event-stream")
		c.Writer.Header().Set("Cache-Control", "no-cache")
		c.Writer.Header().Set("Connection", "keep-alive")
		c.Writer.Header().Set("X-Accel-Buffering", "no")

		flusher, ok := c.Writer.(http.Flusher)
		if !ok {
			c.JSON(http.StatusInternalServerError, gin.H{"error": "no flusher"})
			return
		}

		ch := svc.Subscribe()
		defer svc.Unsubscribe(ch)

		// 先推一帧当前快照
		data, _ := json.Marshal(svc.Current())
		fmt.Fprintf(c.Writer, "event: metrics\ndata: %s\n\n", data)
		flusher.Flush()

		ctx := c.Request.Context()
		for {
			select {
			case <-ctx.Done():
				return
			case m, ok := <-ch:
				if !ok {
					return
				}
				data, _ := json.Marshal(m)
				fmt.Fprintf(c.Writer, "event: metrics\ndata: %s\n\n", data)
				flusher.Flush()
			}
		}
	}
}
