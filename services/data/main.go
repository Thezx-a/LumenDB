// TitanKV Data 服务入口 (Phase 4).
// 启动: go run ./services/data
// 端口: 8081 (默认)
//
// 接口:
//
//	POST   /api/data/kv            写入 KV
//	GET    /api/data/kv?key=xxx    读取 KV
//	DELETE /api/data/kv?key=xxx    删除 KV
//	GET    /api/data/scan?start=&end=  SSE 流式扫描
//	GET    /healthz                健康检查
package main

import (
	"context"
	"log"
	"net/http"
	"os"
	"os/signal"
	"syscall"
	"time"

	"github.com/gin-gonic/gin"

	"github.com/titan-kv/titan/services/data"
)

func main() {
	addr := getenv("DATA_ADDR", ":8081")

	store := data.NewStore()
	svc := data.NewService(store)

	r := gin.New()
	r.Use(gin.Recovery())
	svc.RegisterRoutes(r)

	srv := &http.Server{Addr: addr, Handler: r}

	go func() {
		log.Printf("[INFO] data service listening on %s", addr)
		if err := srv.ListenAndServe(); err != nil && err != http.ErrServerClosed {
			log.Fatalf("listen: %v", err)
		}
	}()

	quit := make(chan os.Signal, 1)
	signal.Notify(quit, syscall.SIGINT, syscall.SIGTERM)
	<-quit
	log.Println("[INFO] data service shutting down...")
	ctx, cancel := context.WithTimeout(context.Background(), 5*time.Second)
	defer cancel()
	srv.Shutdown(ctx)
}

func getenv(key, def string) string {
	if v := os.Getenv(key); v != "" {
		return v
	}
	return def
}
