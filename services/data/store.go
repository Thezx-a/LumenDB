package data

import (
	"sort"
	"strings"
	"sync"
)

// Store 是 data 服务的内存 KV 存储 (MVP).
// 后续 Phase 2 会替换为 minikv LSM-Tree 引擎 (通过 cgo 桥接).
type Store struct {
	mu   sync.RWMutex
	data map[string]string
}

// NewStore 创建内存 KV 存储.
func NewStore() *Store {
	return &Store{data: make(map[string]string)}
}

// Put 写入 KV.
func (s *Store) Put(key, value string) {
	s.mu.Lock()
	defer s.mu.Unlock()
	s.data[key] = value
}

// Get 读取 KV. 返回 value 和是否存在.
func (s *Store) Get(key string) (string, bool) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	v, ok := s.data[key]
	return v, ok
}

// Delete 删除 KV. 返回是否删除了 (key 是否存在).
func (s *Store) Delete(key string) bool {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.data[key]; !ok {
		return false
	}
	delete(s.data, key)
	return true
}

// KVPair 单条 key-value (与 client-go/titan/types.go 对齐).
type KVPair struct {
	Key   string `json:"key"`
	Value string `json:"value"`
}

// Scan 范围扫描. start/end 均为前缀闭区间语义:
//   - start="" 表示从最小 key 开始
//   - end="" 表示扫到最大 key
//   - 否则扫描满足 start <= key < end 的所有 KV (按 key 升序).
//
// 注意: 这里采用 [start, end) 半开区间, 与 LevelDB / RocksDB 习惯一致.
func (s *Store) Scan(start, end string) []KVPair {
	s.mu.RLock()
	defer s.mu.RUnlock()

	out := make([]KVPair, 0, len(s.data))
	for k, v := range s.data {
		if start != "" && k < start {
			continue
		}
		if end != "" && k >= end {
			continue
		}
		out = append(out, KVPair{Key: k, Value: v})
	}
	// 按 key 升序, 便于客户端展示稳定.
	sort.Slice(out, func(i, j int) bool {
		return strings.Compare(out[i].Key, out[j].Key) < 0
	})
	return out
}

// Size 返回当前 KV 总数 (用于健康检查 / 指标).
func (s *Store) Size() int {
	s.mu.RLock()
	defer s.mu.RUnlock()
	return len(s.data)
}
