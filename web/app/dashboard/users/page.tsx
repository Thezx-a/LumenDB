"use client";

import { useState } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import { api, HttpError, API_BASE } from "@/lib/api";
import type { User, UserRole } from "@/lib/types";

const ROLE_LABELS: Record<UserRole, string> = {
  admin: "管理员",
  writer: "写者",
  reader: "只读",
};

/**
 * 用户管理页。
 * 后端 auth 服务目前只提供 /api/auth/register（注册），
 * 尚未提供 /api/users 列表接口，因此本页在加载失败时显示友好提示。
 */
interface RegisterBody {
  username: string;
  password: string;
  role: UserRole;
}

export default function UsersPage() {
  const queryClient = useQueryClient();
  const [showForm, setShowForm] = useState(false);
  const [form, setForm] = useState<RegisterBody>({
    username: "",
    password: "",
    role: "reader",
  });

  // 注意：后端目前没有 GET /api/users，这里会 404，页面会显示错误提示
  const { data, isLoading, error } = useQuery<User[]>({
    queryKey: ["users"],
    queryFn: () => api.get<User[]>("/api/users"),
    retry: 0,
  });

  // 通过 /api/auth/register 创建新用户
  const createMutation = useMutation({
    mutationFn: (input: RegisterBody) =>
      api.post<User>("/api/auth/register", input),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["users"] });
      setShowForm(false);
      setForm({ username: "", password: "", role: "reader" });
    },
  });

  function handleSubmit(e: React.FormEvent) {
    e.preventDefault();
    createMutation.mutate(form);
  }

  async function handleLogout() {
    try {
      await fetch(`${API_BASE}/api/auth/logout`, {
        method: "POST",
        credentials: "include",
      });
    } catch {
      // 忽略
    }
    document.cookie = "titan_token=; Max-Age=0; path=/";
    window.location.href = "/login";
  }

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-semibold tracking-tight">用户管理</h1>
          <p className="mt-1 text-sm text-muted-foreground">
            注册新用户（通过 /api/auth/register）。
          </p>
        </div>
        <button
          type="button"
          onClick={() => setShowForm((v) => !v)}
          className="inline-flex h-9 items-center justify-center rounded-md bg-primary px-4 text-sm font-medium text-primary-foreground shadow hover:bg-primary/90"
        >
          {showForm ? "取消" : "新建用户"}
        </button>
      </div>

      {showForm && (
        <form
          onSubmit={handleSubmit}
          className="space-y-4 rounded-lg border bg-card p-6 shadow-sm"
        >
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-1.5">
              <label className="text-sm font-medium">用户名</label>
              <input
                required
                minLength={3}
                maxLength={32}
                value={form.username}
                onChange={(e) => setForm({ ...form, username: e.target.value })}
                className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm"
              />
            </div>
            <div className="space-y-1.5">
              <label className="text-sm font-medium">密码（≥8 位）</label>
              <input
                type="password"
                required
                minLength={8}
                value={form.password}
                onChange={(e) => setForm({ ...form, password: e.target.value })}
                className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm"
              />
            </div>
            <div className="space-y-1.5">
              <label className="text-sm font-medium">角色</label>
              <select
                value={form.role}
                onChange={(e) => setForm({ ...form, role: e.target.value as UserRole })}
                className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm"
              >
                <option value="admin">管理员</option>
                <option value="writer">写者</option>
                <option value="reader">只读</option>
              </select>
            </div>
          </div>
          {createMutation.isError && (
            <p className="text-xs text-destructive">
              {createMutation.error instanceof HttpError
                ? createMutation.error.message
                : "创建失败"}
            </p>
          )}
          <button
            type="submit"
            disabled={createMutation.isPending}
            className="inline-flex h-9 items-center justify-center rounded-md bg-primary px-4 text-sm font-medium text-primary-foreground disabled:opacity-50"
          >
            {createMutation.isPending ? "创建中…" : "创建"}
          </button>
        </form>
      )}

      {error && (
        <div className="rounded-md bg-amber-100 px-3 py-2 text-xs text-amber-800">
          用户列表接口（GET /api/users）尚未由后端实现，仅支持新建用户。
          已注册用户可通过 <code className="font-mono">POST /api/auth/login</code> 登录。
        </div>
      )}

      {!error && (
        <div className="overflow-hidden rounded-lg border">
          <table className="w-full text-sm">
            <thead className="bg-muted/50">
              <tr>
                <th className="px-4 py-2 text-left font-medium">用户名</th>
                <th className="px-4 py-2 text-left font-medium">角色</th>
                <th className="px-4 py-2 text-left font-medium">创建时间</th>
              </tr>
            </thead>
            <tbody className="divide-y">
              {isLoading && (
                <tr>
                  <td colSpan={3} className="px-4 py-8 text-center text-muted-foreground">
                    加载中…
                  </td>
                </tr>
              )}
              {data?.map((u) => (
                <tr key={u.id} className="hover:bg-muted/30">
                  <td className="px-4 py-2 font-mono text-xs">{u.username}</td>
                  <td className="px-4 py-2">
                    <span className="rounded bg-muted px-1.5 py-0.5 text-xs">
                      {ROLE_LABELS[u.role]}
                    </span>
                  </td>
                  <td className="px-4 py-2 text-xs text-muted-foreground">
                    {new Date(u.created_at).toLocaleString("zh-CN")}
                  </td>
                </tr>
              ))}
            </tbody>
          </table>
        </div>
      )}

      <div className="border-t pt-4">
        <button
          type="button"
          onClick={handleLogout}
          className="rounded-md border px-3 py-2 text-sm text-muted-foreground transition-colors hover:bg-muted hover:text-foreground"
        >
          退出登录
        </button>
      </div>
    </div>
  );
}
