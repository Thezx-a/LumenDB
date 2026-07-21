"use client";

import { Suspense, useState, type FormEvent } from "react";
import { useRouter, useSearchParams } from "next/navigation";
import type { LoginResponse } from "@/lib/types";
import { HttpError, API_BASE } from "@/lib/api";

/**
 * 登录页：用户名密码表单，POST /api/auth/login。
 * 成功后写入 cookie titan_token 并 router.push 到 from（默认 /dashboard）。
 *
 * 注意：useSearchParams 必须被 Suspense 包裹，否则 next build 会报错。
 */
export default function LoginPage() {
  return (
    <Suspense fallback={<LoginFallback />}>
      <LoginForm />
    </Suspense>
  );
}

function LoginFallback() {
  return (
    <main className="flex min-h-screen items-center justify-center bg-gradient-to-br from-background to-muted px-4">
      <div className="w-full max-w-sm animate-pulse rounded-lg border bg-card p-6 shadow-sm">
        <div className="h-6 w-1/2 rounded bg-muted" />
      </div>
    </main>
  );
}

function LoginForm() {
  const router = useRouter();
  const searchParams = useSearchParams();
  const from = searchParams.get("from") ?? "/dashboard";

  const [username, setUsername] = useState("");
  const [password, setPassword] = useState("");
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function handleSubmit(e: FormEvent) {
    e.preventDefault();
    setError(null);
    setLoading(true);

    try {
      const res = await fetch(`${API_BASE}/api/auth/login`, {
        method: "POST",
        headers: { "Content-Type": "application/json" },
        credentials: "include",
        body: JSON.stringify({ username, password }),
      });

      const payload = await res.json().catch(() => null);

      if (!res.ok) {
        const message =
          (payload && (payload as { error?: string }).error) ??
          `登录失败 (${res.status})`;
        throw new HttpError({
          code: "LOGIN_FAILED",
          message,
          status: res.status,
        });
      }

      const data = payload as LoginResponse;
      // 写入 cookie；httpOnly=false 以便浏览器端 fetch 时由 middleware 读取
      // 生产环境务必在 HTTPS 下设置 secure=true
      const maxAge = Math.min(data.expires_in, 7 * 24 * 3600);
      document.cookie = `titan_token=${encodeURIComponent(
        data.access_token,
      )}; Path=/; Max-Age=${maxAge}; SameSite=Lax`;

      router.push(from);
      router.refresh();
    } catch (err) {
      setError(err instanceof HttpError ? err.message : "登录失败，请重试");
    } finally {
      setLoading(false);
    }
  }

  return (
    <main className="flex min-h-screen items-center justify-center bg-gradient-to-br from-background to-muted px-4">
      <div className="w-full max-w-sm">
        <div className="mb-8 flex items-center gap-3">
          <div className="flex h-10 w-10 items-center justify-center rounded-lg bg-primary text-sm font-bold text-primary-foreground">
            TK
          </div>
          <div>
            <h1 className="text-xl font-semibold tracking-tight">TitanKV Console</h1>
            <p className="text-xs text-muted-foreground">分布式键值存储统一控制台</p>
          </div>
        </div>

        <form
          onSubmit={handleSubmit}
          className="space-y-4 rounded-lg border bg-card p-6 shadow-sm"
        >
          <div className="space-y-1.5">
            <label htmlFor="username" className="text-sm font-medium">
              用户名
            </label>
            <input
              id="username"
              name="username"
              type="text"
              autoComplete="username"
              required
              value={username}
              onChange={(e) => setUsername(e.target.value)}
              className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              placeholder="admin"
            />
          </div>

          <div className="space-y-1.5">
            <label htmlFor="password" className="text-sm font-medium">
              密码
            </label>
            <input
              id="password"
              name="password"
              type="password"
              autoComplete="current-password"
              required
              value={password}
              onChange={(e) => setPassword(e.target.value)}
              className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm transition-colors focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
              placeholder="••••••••"
            />
          </div>

          {error && (
            <p className="rounded-md bg-destructive/10 px-3 py-2 text-xs text-destructive">
              {error}
            </p>
          )}

          <button
            type="submit"
            disabled={loading || !username || !password}
            className="inline-flex h-9 w-full items-center justify-center rounded-md bg-primary px-4 text-sm font-medium text-primary-foreground shadow transition-colors hover:bg-primary/90 disabled:cursor-not-allowed disabled:opacity-50"
          >
            {loading ? "登录中…" : "登录"}
          </button>

          <p className="text-center text-xs text-muted-foreground">
            首次使用请先调用 /api/auth/register 注册账号
          </p>
        </form>
      </div>
    </main>
  );
}
