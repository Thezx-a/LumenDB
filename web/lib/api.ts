import type { ApiError } from "./types";

/**
 * 后端 Gateway 基础地址。
 * 通过 NEXT_PUBLIC_API_BASE 注入到浏览器端（NEXT_PUBLIC_ 前缀）。
 */
export const API_BASE =
  process.env.NEXT_PUBLIC_API_BASE ?? "http://localhost:8080";

/** 浏览器端读取 titan_token cookie（middleware 校验同源 cookie） */
function readCookie(name: string): string | null {
  if (typeof document === "undefined") return null;
  const match = document.cookie.match(
    new RegExp("(?:^|; )" + name.replace(/([.$?*|{}()[\]\\/+^])/g, "\\$1") + "=([^;]*)"),
  );
  return match ? decodeURIComponent(match[1]) : null;
}

/** 业务错误类，承载后端返回的结构化错误信息 */
export class HttpError extends Error {
  readonly status: number;
  readonly code: string;
  readonly details?: unknown;

  constructor(error: ApiError) {
    super(error.message);
    this.name = "HttpError";
    this.status = error.status;
    this.code = error.code;
    this.details = error.details;
  }
}

interface FetchOptions extends RequestInit {
  /** 是否携带 Authorization 头，默认 true */
  auth?: boolean;
  /** 查询参数，会拼接到 URL */
  query?: Record<string, string | number | boolean | undefined | null>;
}

/** 将 query 对象拼到 URL 上 */
function withQuery(url: string, query?: FetchOptions["query"]): string {
  if (!query) return url;
  const params = new URLSearchParams();
  for (const [key, value] of Object.entries(query)) {
    if (value === undefined || value === null) continue;
    params.append(key, String(value));
  }
  const qs = params.toString();
  return qs ? `${url}?${qs}` : url;
}

/**
 * 统一的 fetch 封装：
 * - 自动拼接 baseURL
 * - 自动注入 Authorization: Bearer <token>
 * - 统一 JSON 解析与错误处理
 * - 401 时清掉本地 cookie 并跳登录
 */
export async function apiFetch<T>(
  path: string,
  options: FetchOptions = {},
): Promise<T> {
  const { auth = true, query, headers, ...rest } = options;
  const url = withQuery(`${API_BASE}${path}`, query);

  const finalHeaders: Record<string, string> = {
    Accept: "application/json",
    ...(headers as Record<string, string>),
  };

  if (auth) {
    const token = readCookie("titan_token");
    if (token) {
      finalHeaders.Authorization = `Bearer ${token}`;
    }
  }

  if (rest.body && typeof rest.body === "string" && !finalHeaders["Content-Type"]) {
    finalHeaders["Content-Type"] = "application/json";
  }

  let res: Response;
  try {
    res = await fetch(url, {
      ...rest,
      headers: finalHeaders,
      credentials: "include",
    });
  } catch (err) {
    throw new HttpError({
      code: "NETWORK_ERROR",
      message: err instanceof Error ? err.message : "网络请求失败",
      status: 0,
    });
  }

  if (res.status === 204) {
    return undefined as T;
  }

  const isJson = (res.headers.get("content-type") ?? "").includes("application/json");
  const payload = isJson ? await res.json().catch(() => null) : null;

  if (!res.ok) {
    const apiError: ApiError =
      payload && typeof payload === "object"
        ? {
            code: (payload as { code?: string }).code ?? "UNKNOWN",
            message: (payload as { message?: string }).message ?? res.statusText,
            status: res.status,
            details: (payload as { details?: unknown }).details,
          }
        : {
            code: "HTTP_ERROR",
            message: res.statusText || `请求失败 (${res.status})`,
            status: res.status,
          };

    if (res.status === 401 && typeof window !== "undefined") {
      // 清掉过期 token，跳登录页
      document.cookie = "titan_token=; Max-Age=0; path=/";
      if (window.location.pathname !== "/login") {
        window.location.href = "/login";
      }
    }

    throw new HttpError(apiError);
  }

  return payload as T;
}

/** 便捷方法 */
export const api = {
  get: <T>(path: string, options?: FetchOptions) =>
    apiFetch<T>(path, { ...options, method: "GET" }),
  post: <T>(path: string, body?: unknown, options?: FetchOptions) =>
    apiFetch<T>(path, {
      ...options,
      method: "POST",
      body: body === undefined ? undefined : JSON.stringify(body),
    }),
  put: <T>(path: string, body?: unknown, options?: FetchOptions) =>
    apiFetch<T>(path, {
      ...options,
      method: "PUT",
      body: body === undefined ? undefined : JSON.stringify(body),
    }),
  delete: <T>(path: string, options?: FetchOptions) =>
    apiFetch<T>(path, { ...options, method: "DELETE" }),
};
