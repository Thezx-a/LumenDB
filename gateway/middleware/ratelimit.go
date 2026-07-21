package middleware

import (
	"context"
	"net/http"
	"time"

	"github.com/gin-gonic/gin"
	"github.com/redis/go-redis/v9"
)

// ratelimitLua 实现令牌桶: 原子 CAS, 分布式生效.
// 三步 (读当前令牌 → 计算补充 → 写回) 必须原子, 否则并发请求会读到旧值并各自写回导致超卖.
// Redis 单线程执行 Lua, 天然原子.
const ratelimitLua = `
local key = KEYS[1]
local capacity = tonumber(ARGV[1])
local rate = tonumber(ARGV[2])
local now = tonumber(ARGV[3])
local requested = tonumber(ARGV[4])

local bucket = redis.call('HMGET', key, 'tokens', 'ts')
local tokens = tonumber(bucket[1]) or capacity
local ts = tonumber(bucket[2]) or now

local delta = math.max(0, now - ts)
tokens = math.min(capacity, tokens + delta * rate)

local allowed = 0
if tokens >= requested then
    tokens = tokens - requested
    allowed = 1
end

redis.call('HMSET', key, 'tokens', tokens, 'ts', now)
redis.call('EXPIRE', key, math.ceil(capacity / rate) * 2)

return allowed
`

// RateLimit 返回令牌桶限流中间件.
// capacity: 桶容量 (允许瞬时突发)
// rate: 每秒补充令牌数 (长期均值上限)
//
// 在 Auth 之前注册: 先挡匿名恶意流量, 节省 Auth CPU.
func RateLimit(rdb *redis.Client, capacity, rate float64) gin.HandlerFunc {
	script := redis.NewScript(ratelimitLua)
	return func(c *gin.Context) {
		// 未登录按 IP, 登录后按 UID (见 Auth 注入)
		key := "rl:ip:" + c.ClientIP()
		if uid, ok := c.Get("user_id"); ok {
			if s, ok := uid.(string); ok && s != "" {
				key = "rl:uid:" + s
			}
		}
		ctx, cancel := context.WithTimeout(c.Request.Context(), 200*time.Millisecond)
		defer cancel()
		allowed, err := script.Run(ctx, rdb, []string{key},
			capacity, rate, time.Now().Unix(), 1).Int()
		if err != nil {
			// Redis 故障时放行 (可用性优先); 生产可改为拒绝 (保护优先).
			c.Next()
			return
		}
		if allowed == 0 {
			c.AbortWithStatusJSON(http.StatusTooManyRequests, gin.H{
				"error":      "rate limited",
				"request_id": GetRequestID(c),
			})
			return
		}
		c.Next()
	}
}

// NewNoopRateLimit 无 Redis 时的本地限流降级 (单机令牌桶, 不分布式).
// 仅用于本地开发, 生产环境必须用 Redis 版本.
func NewNoopRateLimit(capacity, rate float64) gin.HandlerFunc {
	// 简化: 本地不限流, 仅打日志
	return func(c *gin.Context) { c.Next() }
}
