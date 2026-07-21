package auth

// RBAC 角色 → 权限映射. 与 gateway/middleware/rbac.go 保持一致.
//
// 权限命名规范: resource:action (如 kv:put, collection:create).
// TitanKV 初期用 RBAC 足够, 后期如需 "Collection 级权限" / "IP 白名单"
// 可演进到 ABAC.

type Permission string

const (
	PermKVGet            Permission = "kv:get"
	PermKVPut            Permission = "kv:put"
	PermKVDelete         Permission = "kv:delete"
	PermCollectionCreate Permission = "collection:create"
	PermCollectionDelete Permission = "collection:delete"
	PermUserManage       Permission = "user:manage"
	PermAPIKeyIssue      Permission = "apikey:issue"
)

var rolePerms = map[string]map[Permission]bool{
	"admin": {
		PermKVGet: true, PermKVPut: true, PermKVDelete: true,
		PermCollectionCreate: true, PermCollectionDelete: true,
		PermUserManage: true, PermAPIKeyIssue: true,
	},
	"writer": {PermKVGet: true, PermKVPut: true, PermCollectionCreate: true},
	"reader": {PermKVGet: true},
}

// RoleHasPermission 判断角色是否拥有权限.
func RoleHasPermission(role string, perm Permission) bool {
	if m, ok := rolePerms[role]; ok {
		return m[perm]
	}
	return false
}

// AllRoles 返回所有角色 (用户管理界面用).
func AllRoles() []string {
	out := make([]string, 0, len(rolePerms))
	for r := range rolePerms {
		out = append(out, r)
	}
	return out
}
