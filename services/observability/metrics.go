// Package observability 是 TitanKV 的可观测性服务 (Phase 4).
//
// 职责:
//   - Metrics 聚合 (从各服务 Prometheus exporter 拉取, 二次聚合)
//   - Health Rollup (汇总各服务健康状态)
//   - SSE 实时推送 (给控制台仪表盘用)
//
// 启动: go run ./services/observability
// 端口: 8084 (默认)
package observability

import (
	"net/http"
	"sync"
	"time"
)

// Service 是 Observability 服务.
type Service struct {
	mu          sync.RWMutex
	current     Metrics
	subscribers map[chan Metrics]struct{}
}

// Metrics 推送给控制台的聚合指标.
type Metrics struct {
	QPS         float64 `json:"qps"`         // 每秒查询数
	P50LatencyMs float64 `json:"p50_ms"`     // P50 延迟
	P99LatencyMs float64 `json:"p99_ms"`     // P99 延迟
	StorageGB   float64 `json:"storage_gb"`  // 存储用量
	NodeCount   int     `json:"node_count"`  // 集群节点数
	LeaderCount int     `json:"leader_count"`// Raft Leader 数
	Timestamp   int64   `json:"timestamp"`   // Unix 秒
}

// NewService 创建服务. 启动后台 ticker 每 1s 生成假数据 (生产环境从 Prometheus 拉).
func NewService() *Service {
	s := &Service{
		subscribers: make(map[chan Metrics]struct{}),
		current: Metrics{
			QPS: 0, P50LatencyMs: 0, P99LatencyMs: 0,
			StorageGB: 0, NodeCount: 1, LeaderCount: 1,
			Timestamp: time.Now().Unix(),
		},
	}
	go s.collectLoop()
	return s
}

// collectLoop 模拟每秒采集指标 (生产环境从 Prometheus 拉).
func (s *Service) collectLoop() {
	ticker := time.NewTicker(1 * time.Second)
	defer ticker.Stop()
	for range ticker.C {
		s.mu.Lock()
		// 模拟数据波动
		s.current.QPS = 1000 + float64(time.Now().Unix()%500)
		s.current.P50LatencyMs = 4 + float64(time.Now().Unix()%3)
		s.current.P99LatencyMs = 35 + float64(time.Now().Unix()%20)
		s.current.StorageGB = 12.3 + float64(time.Now().Unix()%100)/100
		s.current.Timestamp = time.Now().Unix()
		m := s.current
		s.mu.Unlock()

		// 推送给所有 SSE 订阅者
		for ch := range s.subscribers {
			select {
			case ch <- m:
			default:
				// 订阅者消费慢, 丢弃本条 (避免阻塞 collect)
			}
		}
	}
}

// Subscribe 订阅 metrics 流. 返回 channel, 调用方读完应 Unsubscribe.
func (s *Service) Subscribe() chan Metrics {
	ch := make(chan Metrics, 16)
	s.mu.Lock()
	s.subscribers[ch] = struct{}{}
	s.mu.Unlock()
	return ch
}

// Unsubscribe 取消订阅.
func (s *Service) Unsubscribe(ch chan Metrics) {
	s.mu.Lock()
	delete(s.subscribers, ch)
	s.mu.Unlock()
	close(ch)
}

// Current 返回当前指标快照.
func (s *Service) Current() Metrics {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return s.current
}

// HealthStatus 健康状态.
type HealthStatus struct {
	Status    string            `json:"status"`     // ok / degraded / down
	Version   string            `json:"version"`
	Uptime    int64             `json:"uptime_seconds"`
	Deps      map[string]string `json:"deps"`       // 每个依赖的状态
}

// Health 返回健康状态.
func (s *Service) Health() HealthStatus {
	return HealthStatus{
		Status:  "ok",
		Version: "0.1.0",
		Uptime:  int64(time.Since(startTime).Seconds()),
		Deps: map[string]string{
			"data-service":   "ok",
			"meta-service":   "ok",
			"auth-service":   "ok",
			"storage-engine": "ok",
		},
	}
}

var startTime = time.Now()
