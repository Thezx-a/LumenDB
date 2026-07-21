package auth

import (
	"context"
	"crypto/rand"
	"crypto/sha256"
	"encoding/hex"
	"fmt"
	"sync"
	"time"

	"github.com/google/uuid"
)

const apiKeyPrefix = "tk_live_"

// APIKeyRecord API Key 元数据 (服务端存储).
type APIKeyRecord struct {
	ID        string // 主键
	UserID    string // 所属用户
	Hash      string // SHA256(plain) — 不存原值
	Name      string // 用户给的标签
	CreatedAt time.Time
	ExpiresAt time.Time // 0 表示不过期
	Revoked   bool
}

// APIKeyStore API Key 存储 (内存版; 生产用 Postgres).
type APIKeyStore struct {
	mu     sync.RWMutex
	byID   map[string]*APIKeyRecord // id → record
	byHash map[string]*APIKeyRecord // sha256 → record
}

func NewAPIKeyStore() *APIKeyStore {
	return &APIKeyStore{byID: make(map[string]*APIKeyRecord), byHash: make(map[string]*APIKeyRecord)}
}

// Issue 签发 API Key. 返回明文 (仅一次) 与 record (含 hash).
// 形如 tk_live_<random32bytes>, 前缀区分环境.
// 服务端只存 SHA256(key), 数据库泄露不暴露原 Key.
func (s *APIKeyStore) Issue(userID, name string, ttl time.Duration) (plain string, rec *APIKeyRecord, err error) {
	buf := make([]byte, 32)
	if _, err := rand.Read(buf); err != nil {
		return "", nil, err
	}
	plain = apiKeyPrefix + hex.EncodeToString(buf)
	sum := sha256.Sum256([]byte(plain))
	hash := hex.EncodeToString(sum[:])

	rec = &APIKeyRecord{
		ID:        uuidNew(),
		UserID:    userID,
		Hash:      hash,
		Name:      name,
		CreatedAt: time.Now(),
	}
	if ttl > 0 {
		rec.ExpiresAt = time.Now().Add(ttl)
	}
	s.mu.Lock()
	s.byID[rec.ID] = rec
	s.byHash[hash] = rec
	s.mu.Unlock()
	return plain, rec, nil
}

// Verify 校验 API Key. 返回 (userID, role, ok).
// 流程: plain → SHA256(plain) → 查 hash → 校验未吊销/未过期 → 查 user 拿 role.
func (s *APIKeyStore) Verify(ctx context.Context, plain string, userStore *UserStore) (userID, role string, ok bool) {
	sum := sha256.Sum256([]byte(plain))
	hash := hex.EncodeToString(sum[:])
	s.mu.RLock()
	rec := s.byHash[hash]
	s.mu.RUnlock()
	if rec == nil || rec.Revoked {
		return "", "", false
	}
	if !rec.ExpiresAt.IsZero() && time.Now().After(rec.ExpiresAt) {
		return "", "", false
	}
	u, err := userStore.FindByID(rec.UserID)
	if err != nil {
		return "", "", false
	}
	return u.ID, u.Role, true
}

// Revoke 吊销 API Key (设 Revoked=true, 不删记录).
func (s *APIKeyStore) Revoke(id string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	rec, ok := s.byID[id]
	if !ok {
		return fmt.Errorf("apikey not found: %s", id)
	}
	rec.Revoked = true
	return nil
}

// ListByUser 列出某用户的所有 API Key (不含 hash).
func (s *APIKeyStore) ListByUser(userID string) []*APIKeyRecord {
	s.mu.RLock()
	defer s.mu.RUnlock()
	var out []*APIKeyRecord
	for _, r := range s.byID {
		if r.UserID == userID {
			// 返回副本, 不暴露 Hash
			cp := *r
			cp.Hash = ""
			out = append(out, &cp)
		}
	}
	return out
}

// uuidNew 简单包装, 便于测试 mock.
var uuidNew = func() string {
	return uuid.NewString()
}
