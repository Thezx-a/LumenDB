// Package meta 是 TitanKV 的元数据服务 (Phase 4).
//
// 职责:
//   - Collection 元数据 CRUD (name / ttl / schema / created_at / updated_at)
//   - etcd watch 实现配置热更新 (所有 Meta 实例实时收到变更通知)
//
// 为什么用 etcd 而不是 Redis:
//   - etcd 基于 Raft 强一致, 元数据丢失会导致业务异常
//   - Redis 主从异步复制, 故障切换可能丢数据
//   - 限流/session 这种 "可丢可重建" 数据用 Redis 即可
//
// 启动: go run ./services/meta
// 端口: 8083 (默认)
package meta

import (
	"errors"
	"sync"
	"time"
)

// Collection 命名空间元数据.
type Collection struct {
	Name      string            `json:"name"`
	TTL       int               `json:"ttl_seconds"`
	Schema    map[string]string `json:"schema"`
	CreatedAt int64             `json:"created_at"`
	UpdatedAt int64             `json:"updated_at"`
}

var (
	ErrCollectionNotFound      = errors.New("collection not found")
	ErrCollectionAlreadyExists = errors.New("collection already exists")
)

// Store 元数据存储. 内存版 + 可选 etcd watch 同步.
type Store struct {
	mu   sync.RWMutex
	data map[string]*Collection
}

func NewStore() *Store {
	return &Store{data: make(map[string]*Collection)}
}

// Create 创建 Collection.
func (s *Store) Create(c *Collection) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.data[c.Name]; ok {
		return ErrCollectionAlreadyExists
	}
	now := time.Now().Unix()
	c.CreatedAt = now
	c.UpdatedAt = now
	cp := *c
	s.data[c.Name] = &cp
	return nil
}

// Find 查找.
func (s *Store) Find(name string) (*Collection, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	c, ok := s.data[name]
	if !ok {
		return nil, ErrCollectionNotFound
	}
	cp := *c
	return &cp, nil
}

// List 列出所有.
func (s *Store) List() []*Collection {
	s.mu.RLock()
	defer s.mu.RUnlock()
	out := make([]*Collection, 0, len(s.data))
	for _, c := range s.data {
		cp := *c
		out = append(out, &cp)
	}
	return out
}

// Update 更新 (TTL/Schema).
func (s *Store) Update(name string, ttl int, schema map[string]string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	c, ok := s.data[name]
	if !ok {
		return ErrCollectionNotFound
	}
	c.TTL = ttl
	c.Schema = schema
	c.UpdatedAt = time.Now().Unix()
	return nil
}

// Delete 删除.
func (s *Store) Delete(name string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.data[name]; !ok {
		return ErrCollectionNotFound
	}
	delete(s.data, name)
	return nil
}

// ApplyWatchEvent 应用 etcd watch 事件到本地缓存.
// 用于多个 Meta 实例间同步.
func (s *Store) ApplyWatchEvent(typ string, c *Collection) {
	s.mu.Lock()
	defer s.mu.Unlock()
	switch typ {
	case "PUT":
		if c == nil {
			return
		}
		s.data[c.Name] = c
	case "DELETE":
		if c != nil {
			delete(s.data, c.Name)
		}
	}
}
