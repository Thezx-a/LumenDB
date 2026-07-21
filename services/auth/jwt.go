package auth

import (
	"errors"
	"fmt"
	"time"

	"github.com/golang-jwt/jwt/v5"
	"github.com/google/uuid"
)

// Claims JWT Claims, 与 gateway/middleware/auth.go Claims 结构一致.
type Claims struct {
	UserID string `json:"sub"`
	Role   string `json:"role"`
	jwt.RegisteredClaims
}

// JWTManager 管理 JWT 签发与解析.
type JWTManager struct {
	secret      []byte
	accessTTL   time.Duration
	refreshTTL  time.Duration
}

func NewJWTManager(secret string) *JWTManager {
	return &JWTManager{
		secret:     []byte(secret),
		accessTTL:  30 * time.Minute,
		refreshTTL: 7 * 24 * time.Hour,
	}
}

// SignAccess 签发 Access Token (短期, 30min).
func (m *JWTManager) SignAccess(userID, role string) (token, jti string, err error) {
	jti = uuid.NewString()
	claims := Claims{
		UserID: userID,
		Role:   role,
		RegisteredClaims: jwt.RegisteredClaims{
			ID:        jti,
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(m.accessTTL)),
			IssuedAt:  jwt.NewNumericDate(time.Now()),
			Issuer:    "titan-auth",
			Subject:   userID,
		},
	}
	t, err := jwt.NewWithClaims(jwt.SigningMethodHS256, claims).SignedString(m.secret)
	return t, jti, err
}

// SignRefresh 签发 Refresh Token (长期, 7d). Refresh Token 不带 role,
// 刷新时重新查角色, 避免角色变更后 Refresh 仍带旧角色.
func (m *JWTManager) SignRefresh(userID string) (token, jti string, err error) {
	jti = uuid.NewString()
	claims := Claims{
		UserID: userID,
		RegisteredClaims: jwt.RegisteredClaims{
			ID:        jti,
			ExpiresAt: jwt.NewNumericDate(time.Now().Add(m.refreshTTL)),
			IssuedAt:  jwt.NewNumericDate(time.Now()),
			Issuer:    "titan-auth-refresh",
			Subject:   userID,
		},
	}
	t, err := jwt.NewWithClaims(jwt.SigningMethodHS256, claims).SignedString(m.secret)
	return t, jti, err
}

// Parse 校验签名算法 (拒绝 alg:none 攻击) + 解析 claims.
func (m *JWTManager) Parse(tokenStr string) (*Claims, error) {
	claims := &Claims{}
	_, err := jwt.ParseWithClaims(tokenStr, claims, func(t *jwt.Token) (interface{}, error) {
		if _, ok := t.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, fmt.Errorf("unexpected signing method: %v", t.Header["alg"])
		}
		return m.secret, nil
	})
	if err != nil {
		return nil, err
	}
	return claims, nil
}

// AccessTTL / RefreshTTL 暴露 TTL, 用于 Redis 存储时设过期.
func (m *JWTManager) AccessTTL() time.Duration  { return m.accessTTL }
func (m *JWTManager) RefreshTTL() time.Duration { return m.refreshTTL }

// ErrInvalidToken 校验失败时返回 (避免泄漏细节).
var ErrInvalidToken = errors.New("invalid token")
