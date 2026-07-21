// Package auth 实现 TitanKV 的 Auth 服务 (Phase 3).
//
// 功能:
//   - 用户注册/登录 (bcrypt + JWT)
//   - Refresh Token (Redis 存储, 可吊销)
//   - API Key 签发/校验 (机器对机器)
//   - RBAC 角色 → 权限映射
//
// 启动: go run ./services/auth
// 端口: 8082 (默认)
package auth

import (
	"golang.org/x/crypto/bcrypt"
)

const bcryptCost = 12 // 2^12 ≈ 4096 轮, 约 250ms

// HashPassword 用 bcrypt 哈希密码. 盐内置, 抗彩虹表与暴力破解.
func HashPassword(plain string) (string, error) {
	h, err := bcrypt.GenerateFromPassword([]byte(plain), bcryptCost)
	return string(h), err
}

// VerifyPassword 校验密码. 不匹配返回 error.
func VerifyPassword(plain, hash string) error {
	return bcrypt.CompareHashAndPassword([]byte(hash), []byte(plain))
}
