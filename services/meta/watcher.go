package meta

import (
	"context"
	"encoding/json"
	"log"
	"strings"
	"time"

	clientv3 "go.etcd.io/etcd/client/v3"
)

// etcdPrefix etcd 中 Collection 元数据的 key 前缀.
const etcdPrefix = "/titan/collections/"

// WatchEtcd 启动 etcd watch, 实时同步 Collection 变更到本地缓存.
//
// 工作方式:
//   - Meta 服务启动时调用, 阻塞直到 ctx 取消
//   - 任何 CRUD 都先写 etcd (调用方实现), 所有 Meta 实例收到 watch 事件
//   - PUT 事件 → 本地缓存更新
//   - DELETE 事件 → 本地缓存删除
//
// 为什么不用 Redis:
//   - etcd 强一致 (Raft), 元数据不能丢
//   - Redis 异步复制可能丢
func (s *Service) WatchEtcd(ctx context.Context, etcd *clientv3.Client) {
	rch := etcd.Watch(ctx, etcdPrefix, clientv3.WithPrefix())
	for {
		select {
		case <-ctx.Done():
			return
		case wresp, ok := <-rch:
			if !ok {
				log.Println("[WARN] etcd watch channel closed, retrying in 3s...")
				time.Sleep(3 * time.Second)
				rch = etcd.Watch(ctx, etcdPrefix, clientv3.WithPrefix())
				continue
			}
			for _, ev := range wresp.Events {
				name := strings.TrimPrefix(string(ev.Kv.Key), etcdPrefix)
				var c *Collection
				if len(ev.Kv.Value) > 0 {
					if err := json.Unmarshal(ev.Kv.Value, &c); err == nil {
						// 确保 name 一致 (防止 etcd 数据被篡改)
						c.Name = name
					}
				}
				switch ev.Type {
				case clientv3.EventTypePut:
					s.store.ApplyWatchEvent("PUT", c)
					log.Printf("[INFO] meta watch PUT %s", name)
				case clientv3.EventTypeDelete:
					s.store.ApplyWatchEvent("DELETE", &Collection{Name: name})
					log.Printf("[INFO] meta watch DELETE %s", name)
				}
			}
		}
	}
}
