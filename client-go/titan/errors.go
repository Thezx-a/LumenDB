// Package titan 是 TitanKV 的 Go SDK (Phase 4).
//
// 设计要点:
//   - typed errors: 用户用 errors.Is(err, ErrNotFound) 判断, 而非字符串匹配
//   - 重试: 指数退避 + jitter, 仅对幂等操作 (GET) 重试
//   - context 透传: 所有方法接受 ctx, 支持超时/取消
//
// 用法:
//
//	client := titan.New("http://localhost:8080", "tk_live_xxx")
//	val, err := client.Get(context.Background(), "foo")
//	if errors.Is(err, titan.ErrNotFound) { ... }
package titan

import "errors"

var (
	// ErrNotFound key 不存在.
	ErrNotFound = errors.New("titan: not found")
	// ErrConflict 冲突 (如创建已存在的资源).
	ErrConflict = errors.New("titan: conflict")
	// ErrUnauthorized 未授权 (401).
	ErrUnauthorized = errors.New("titan: unauthorized")
	// ErrForbidden 权限不足 (403).
	ErrForbidden = errors.New("titan: forbidden")
	// ErrRateLimited 被限流 (429).
	ErrRateLimited = errors.New("titan: rate limited")
	// ErrInternal 服务端错误 (5xx).
	ErrInternal = errors.New("titan: internal")
	// ErrInvalidArgument 客户端参数错误 (4xx).
	ErrInvalidArgument = errors.New("titan: invalid argument")
)
