package auth

import (
	"context"
	"encoding/json"
	"net/http"
	"strings"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/google/uuid"
	"github.com/redis/go-redis/v9"
)

// Service 是 Auth 服务核心, 持有所有依赖.
type Service struct {
	users      *UserStore
	apiKeys    *APIKeyStore
	jwt        *JWTManager
	rdb        *redis.Client // 存 refresh token / jti 黑名单
}

func NewService(jwtSecret string, rdb *redis.Client) *Service {
	return &Service{
		users:   NewUserStore(),
		apiKeys: NewAPIKeyStore(),
		jwt:     NewJWTManager(jwtSecret),
		rdb:     rdb,
	}
}

// RegisterRequest 注册请求.
type RegisterRequest struct {
	Username string `json:"username" binding:"required,min=3,max=32"`
	Password string `json:"password" binding:"required,min=8,max=128"`
	Role     string `json:"role" binding:"required,oneof=admin writer reader"`
}

// Register 处理注册. 默认角色 reader, 防止越权.
func (s *Service) Register(c *gin.Context) {
	var req RegisterRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	hash, err := HashPassword(req.Password)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "hash failed"})
		return
	}
	u := &User{
		ID:           uuid.NewString(),
		Username:     req.Username,
		PasswordHash: hash,
		Role:         req.Role,
		CreatedAt:    time.Now(),
		UpdatedAt:    time.Now(),
	}
	if err := s.users.Create(u); err != nil {
		c.JSON(http.StatusConflict, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusCreated, gin.H{
		"id":       u.ID,
		"username": u.Username,
		"role":     u.Role,
	})
}

// LoginRequest 登录请求.
type LoginRequest struct {
	Username string `json:"username" binding:"required"`
	Password string `json:"password" binding:"required"`
}

// LoginResponse 登录响应.
type LoginResponse struct {
	AccessToken  string `json:"access_token"`
	RefreshToken string `json:"refresh_token"`
	ExpiresIn    int64  `json:"expires_in"` // 秒
	TokenType    string `json:"token_type"`
	UserID       string `json:"user_id"`
	Role         string `json:"role"`
}

// Login 处理登录. 成功返回 access + refresh token.
func (s *Service) Login(c *gin.Context) {
	var req LoginRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	u, err := s.users.FindByUsername(req.Username)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
		return
	}
	if err := VerifyPassword(req.Password, u.PasswordHash); err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid credentials"})
		return
	}
	access, accessJTI, err := s.jwt.SignAccess(u.ID, u.Role)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "sign failed"})
		return
	}
	refresh, refreshJTI, err := s.jwt.SignRefresh(u.ID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "sign failed"})
		return
	}
	// 把 refresh jti 存 Redis (TTL = refresh TTL), 用于:
	// 1) 校验 refresh 是否有效 (未吊销)
	// 2) Refresh Token 轮换: 用一次就吊销
	if s.rdb != nil {
		ctx, cancel := context.WithTimeout(c.Request.Context(), 200*time.Millisecond)
		defer cancel()
		s.rdb.Set(ctx, "refresh:"+refreshJTI, u.ID, s.jwt.RefreshTTL())
	}
	c.JSON(http.StatusOK, LoginResponse{
		AccessToken:  access,
		RefreshToken: refresh,
		ExpiresIn:    int64(s.jwt.AccessTTL().Seconds()),
		TokenType:    "Bearer",
		UserID:       u.ID,
		Role:         u.Role,
	})
	_ = accessJTI // access token 不存 Redis, 靠 jti 黑名单管理
}

// RefreshRequest 刷新请求.
type RefreshRequest struct {
	RefreshToken string `json:"refresh_token" binding:"required"`
}

// Refresh 用 refresh token 换新的 access token.
// Refresh Token 轮换: 用一次就吊销, 防重放.
func (s *Service) Refresh(c *gin.Context) {
	var req RefreshRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	claims, err := s.jwt.Parse(req.RefreshToken)
	if err != nil || claims.Issuer != "titan-auth-refresh" {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid refresh token"})
		return
	}
	// 校验 refresh jti 在 Redis 中 (未吊销)
	if s.rdb != nil {
		ctx, cancel := context.WithTimeout(c.Request.Context(), 200*time.Millisecond)
		defer cancel()
		n, err := s.rdb.Exists(ctx, "refresh:"+claims.ID).Result()
		if err != nil || n == 0 {
			c.JSON(http.StatusUnauthorized, gin.H{"error": "refresh token revoked"})
			return
		}
		// 一次性: 立即吊销
		s.rdb.Del(ctx, "refresh:"+claims.ID)
	}
	// 重新查角色 (角色可能已变更)
	u, err := s.users.FindByID(claims.UserID)
	if err != nil {
		c.JSON(http.StatusUnauthorized, gin.H{"error": "user not found"})
		return
	}
	access, _, err := s.jwt.SignAccess(u.ID, u.Role)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "sign failed"})
		return
	}
	newRefresh, _, err := s.jwt.SignRefresh(u.ID)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": "sign failed"})
		return
	}
	c.JSON(http.StatusOK, gin.H{
		"access_token":  access,
		"refresh_token": newRefresh,
		"expires_in":    int64(s.jwt.AccessTTL().Seconds()),
	})
}

// Logout 把当前 access token 的 jti 加入黑名单 (TTL = 剩余有效期).
func (s *Service) Logout(c *gin.Context) {
	authz := c.GetHeader("Authorization")
	tokenStr := strings.TrimPrefix(authz, "Bearer ")
	claims, err := s.jwt.Parse(tokenStr)
	if err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": "invalid token"})
		return
	}
	if s.rdb != nil {
		ctx, cancel := context.WithTimeout(c.Request.Context(), 200*time.Millisecond)
		defer cancel()
		// TTL = token 剩余有效期, 过期后自动清出黑名单
		ttl := time.Until(claims.ExpiresAt.Time)
		if ttl > 0 {
			s.rdb.Set(ctx, "revoked:jti:"+claims.ID, "1", ttl)
		}
	}
	c.JSON(http.StatusOK, gin.H{"ok": true})
}

// IssueAPIKeyRequest 签发 API Key 请求.
type IssueAPIKeyRequest struct {
	Name string `json:"name" binding:"required"`
	TTL  int    `json:"ttl_seconds"` // 0 = 永不过期
}

// IssueAPIKey 签发 API Key (需 apikey:issue 权限).
// 明文只返回一次, 服务端只存 SHA256.
func (s *Service) IssueAPIKey(c *gin.Context) {
	uid, _ := c.Get("user_id")
	userID, _ := uid.(string)

	var req IssueAPIKeyRequest
	if err := c.ShouldBindJSON(&req); err != nil {
		c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
		return
	}
	ttl := time.Duration(req.TTL) * time.Second
	plain, rec, err := s.apiKeys.Issue(userID, req.Name, ttl)
	if err != nil {
		c.JSON(http.StatusInternalServerError, gin.H{"error": err.Error()})
		return
	}
	c.JSON(http.StatusCreated, gin.H{
		"api_key":    plain, // 仅一次
		"id":         rec.ID,
		"name":       rec.Name,
		"expires_at": rec.ExpiresAt,
	})
}

// VerifyAPIKey 给 Gateway 调用, 校验 API Key 并返回身份.
func (s *Service) VerifyAPIKey(ctx context.Context, plain string) (userID, role string, ok bool) {
	return s.apiKeys.Verify(ctx, plain, s.users)
}

// RegisterRoutes 在 Gin Router 上注册 Auth 路由 (供 Auth 服务独立启动用).
func (s *Service) RegisterRoutes(r *gin.Engine) {
	r.POST("/api/auth/register", s.Register)
	r.POST("/api/auth/login", s.Login)
	r.POST("/api/auth/refresh", s.Refresh)
	r.POST("/api/auth/verify-apikey", func(c *gin.Context) {
		var req struct {
			APIKey string `json:"api_key" binding:"required"`
		}
		if err := c.ShouldBindJSON(&req); err != nil {
			c.JSON(http.StatusBadRequest, gin.H{"error": err.Error()})
			return
		}
		uid, role, ok := s.VerifyAPIKey(c.Request.Context(), req.APIKey)
		if !ok {
			c.JSON(http.StatusUnauthorized, gin.H{"error": "invalid api key"})
			return
		}
		c.JSON(http.StatusOK, gin.H{"user_id": uid, "role": role})
	})
	r.GET("/healthz", func(c *gin.Context) {
		c.JSON(http.StatusOK, gin.H{"status": "ok", "service": "auth"})
	})
}

// json decode helper (内部用)
func decodeJSON(r *http.Request, v interface{}) error {
	dec := json.NewDecoder(r.Body)
	dec.DisallowUnknownFields()
	return dec.Decode(v)
}
