// TitanKV 控制台涉及的 TypeScript 类型定义
// 字段名与后端 Go struct 的 json tag 保持一致 (snake_case)

/** 实时指标快照（对应 SSE /api/metrics/stream 推送与 /api/observability/metrics 首屏） */
export interface Metrics {
  /** 当前 QPS（每秒请求数） */
  qps: number;
  /** P50 延迟，单位毫秒 */
  p50_ms: number;
  /** P99 延迟，单位毫秒 */
  p99_ms: number;
  /** 已用存储空间，单位 GB */
  storage_gb: number;
  /** 集群节点数 */
  node_count: number;
  /** Raft Leader 数 */
  leader_count: number;
  /** 采样时间戳（Unix 秒） */
  timestamp: number;
}

/** 用户 */
export interface User {
  id: string;
  username: string;
  /** 角色名：admin / writer / reader */
  role: UserRole;
  created_at: string;
  updated_at: string;
}

export type UserRole = "admin" | "writer" | "reader";

/** Collection（键值集合的元数据） */
export interface Collection {
  name: string;
  /** TTL，单位秒；0 表示永久 */
  ttl_seconds: number;
  /** Schema 定义 */
  schema: Record<string, string>;
  created_at: number;
  updated_at: number;
}

export interface CollectionInput {
  name: string;
  ttl_seconds?: number;
  schema?: Record<string, string>;
}

/** 键值对 */
export interface KVPair {
  key: string;
  value: string;
}

/** Scan 请求参数 */
export interface ScanParams {
  start?: string;
  end?: string;
}

/** 登录请求 */
export interface LoginRequest {
  username: string;
  password: string;
}

/** 登录响应（与后端 services/auth/handler.go LoginResponse 对齐） */
export interface LoginResponse {
  access_token: string;
  refresh_token: string;
  expires_in: number;
  token_type: string;
  user_id: string;
  role: UserRole;
}

/** 通用 API 错误 */
export interface ApiError {
  code: string;
  message: string;
  status: number;
  details?: unknown;
}
