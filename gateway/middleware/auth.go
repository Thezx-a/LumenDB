package middleware

import (
	"context"
	"crypto/sha256"
	"encoding/hex"
	"net/http"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/golang-jwt/jwt/v5"
	"github.com/redis/go-redis/v9"
)

// Claims 自定义 JWT Claims, 包含 user_id / role / jti.
type Claims struct {
	UserID string `json:"sub"`
	Role   string `json:"role"`
	jwt.RegisteredClaims
}

// Auth 校验 JWT 或 API Key. 失败返回 401.
// 优先级: Bearer token (JWT) > X-API-Key (API Key).
//
// 在 RBAC 之前注册: 先识别 "你是谁", 再判断 "你能干啥".
type AuthConfig struct {
	JWTSecret    string
	Redis        *redis.Client
	APIKeyVerify func(ctx context.Context, key string) (userID, role string, ok bool)
}

func Auth(cfg AuthConfig) gin.HandlerFunc {
	return func(c *gin.Context) {
		// 1) 优先 Bearer JWT
		authz := c.GetHeader("Authorization")
		if strings.HasPrefix(authz, "Bearer ") {
			tokenStr := strings.TrimPrefix(authz, "Bearer ")
			claims, err := parseJWT(tokenStr, cfg.JWTSecret)
			if err == nil {
				// 检查 jti 黑名单 (登出后吊销)
				if claims.ID != "" && cfg.Redis != nil {
					if isRevoked(c, cfg.Redis, claims.ID) {
						c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{"error": "token revoked"})
						return
					}
				}
				c.Set("user_id", claims.UserID)
				c.Set("role", claims.Role)
				c.Set("auth_method", "jwt")
				c.Next()
				return
			}
		}

		// 2) 其次 X-API-Key
		apiKey := c.GetHeader("X-API-Key")
		if apiKey != "" && cfg.APIKeyVerify != nil {
			uid, role, ok := cfg.APIKeyVerify(c.Request.Context(), apiKey)
			if ok {
				c.Set("user_id", uid)
				c.Set("role", role)
				c.Set("auth_method", "apikey")
				c.Next()
				return
			}
		}

		c.AbortWithStatusJSON(http.StatusUnauthorized, gin.H{
			"error":      "unauthorized",
			"request_id": GetRequestID(c),
		})
	}
}

// parseJWT 校验签名算法 (拒绝 alg:none 攻击) + 解析 claims.
func parseJWT(tokenStr, secret string) (*Claims, error) {
	claims := &Claims{}
	_, err := jwt.ParseWithClaims(tokenStr, claims, func(t *jwt.Token) (interface{}, error) {
		if _, ok := t.Method.(*jwt.SigningMethodHMAC); !ok {
			return nil, jwt.ErrTokenSignatureInvalid
		}
		return []byte(secret), nil
	})
	if err != nil {
		return nil, err
	}
	return claims, nil
}

// isRevoked 检查 jti 是否在黑名单 (Redis key: revoked:jti:<id>, TTL = token 剩余有效期).
func isRevoked(c *gin.Context, rdb *redis.Client, jti string) bool {
	ctx, cancel := context.WithTimeout(c.Request.Context(), 100*time.Millisecond)
	defer cancel()
	n, err := rdb.Exists(ctx, "revoked:jti:"+jti).Result()
	if err != nil {
		return false // Redis 故障时放行
	}
	return n > 0
}

// HashAPIKey 服务端只存 SHA256(key), 数据库泄露不暴露原 Key.
// 校验时对入参做 SHA256, 比对 DB 中存储的 hash.
func HashAPIKey(plain string) string {
	sum := sha256.Sum256([]byte(plain))
	return hex.EncodeToString(sum[:])
}
