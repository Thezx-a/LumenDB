// TitanKV Data service entry (Phase 4).
// Start: go run ./cmd/data
// Port: 8081 (default)
//
// Routes:
//
//	POST   /api/data/kv
//	GET    /api/data/kv?key=xxx
//	DELETE /api/data/kv?key=xxx
//	GET    /api/data/scan?start=&end=
//	GET    /healthz
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
