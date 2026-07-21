package handler

import (
	"net/http"
	"net/http/httputil"
	"net/url"
	"strings"

	"github.com/gin-gonic/gin"
)

// ReverseProxy 反向代理到后端服务.
// 用于 Gateway 把 /api/data/* 转发到 data-service, /api/meta/* 转发到 meta-service 等.
// 真实生产中也可基于服务发现 (etcd) 动态选目标.
type ReverseProxy struct {
	// targets 按前缀路由: "/api/data" → "http://localhost:8081"
	targets map[string]*url.URL
	proxies map[string]*httputil.ReverseProxy
}

func NewReverseProxy(targets map[string]string) (*ReverseProxy, error) {
	rp := &ReverseProxy{
		targets: make(map[string]*url.URL, len(targets)),
		proxies: make(map[string]*httputil.ReverseProxy, len(targets)),
	}
	for prefix, raw := range targets {
		u, err := url.Parse(raw)
		if err != nil {
			return nil, err
		}
		rp.targets[prefix] = u
		rp.proxies[prefix] = httputil.NewSingleHostReverseProxy(u)
	}
	return rp, nil
}

// Handle 找到匹配的前缀, 转发请求; 不匹配返回 404.
// 转发前注入 X-User-ID / X-Role 头, 让后端服务拿到身份.
func (rp *ReverseProxy) Handle(c *gin.Context) {
	path := c.Request.URL.Path
	for prefix, proxy := range rp.proxies {
		if strings.HasPrefix(path, prefix) {
			// 注入身份信息到上游
			if uid, ok := c.Get("user_id"); ok {
				c.Request.Header.Set("X-User-ID", uid.(string))
			}
			if role, ok := c.Get("role"); ok {
				c.Request.Header.Set("X-Role", role.(string))
			}
			c.Request.Header.Set("X-Request-ID", c.GetString("request_id"))
			proxy.ServeHTTP(c.Writer, c.Request)
			return
		}
	}
	c.JSON(http.StatusNotFound, gin.H{"error": "no upstream for path"})
}
