package auth

import (
	"errors"
	"sync"
	"time"
)

// User 用户实体.
type User struct {
	ID           string
	Username     string
	PasswordHash string
	Role         string
	CreatedAt    time.Time
	UpdatedAt    time.Time
}

// UserStore 用户存储 (内存版, 生产用 Postgres).
type UserStore struct {
	mu      sync.RWMutex
	byID    map[string]*User
	byName  map[string]*User
}

func NewUserStore() *UserStore {
	return &UserStore{byID: make(map[string]*User), byName: make(map[string]*User)}
}

var (
	ErrUserNotFound      = errors.New("user not found")
	ErrUserAlreadyExists = errors.New("user already exists")
)

// Create 创建用户. 用户名唯一.
func (s *UserStore) Create(u *User) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	if _, ok := s.byName[u.Username]; ok {
		return ErrUserAlreadyExists
	}
	s.byID[u.ID] = u
	s.byName[u.Username] = u
	return nil
}

// FindByID 按 ID 查找.
func (s *UserStore) FindByID(id string) (*User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	u, ok := s.byID[id]
	if !ok {
		return nil, ErrUserNotFound
	}
	return u, nil
}

// FindByUsername 按用户名查找.
func (s *UserStore) FindByUsername(name string) (*User, error) {
	s.mu.RLock()
	defer s.mu.RUnlock()
	u, ok := s.byName[name]
	if !ok {
		return nil, ErrUserNotFound
	}
	return u, nil
}

// UpdateRole 更新用户角色.
func (s *UserStore) UpdateRole(id, role string) error {
	s.mu.Lock()
	defer s.mu.Unlock()
	u, ok := s.byID[id]
	if !ok {
		return ErrUserNotFound
	}
	u.Role = role
	u.UpdatedAt = time.Now()
	return nil
}

// List 列出所有用户 (用于用户管理界面).
func (s *UserStore) List() []*User {
	s.mu.RLock()
	defer s.mu.RUnlock()
	out := make([]*User, 0, len(s.byID))
	for _, u := range s.byID {
		cp := *u
		cp.PasswordHash = "" // 不暴露密码 hash
		out = append(out, &cp)
	}
	return out
}
