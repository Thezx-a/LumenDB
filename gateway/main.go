// TitanKV Gateway 入口 (Phase 3).
// 启动: go run ./gateway
// 或:   go build -o bin/gateway ./gateway && ./bin/gateway
package main

import (
	"github.com/titan-kv/titan/gateway"
)

func main() {
	gateway.Run()
}
