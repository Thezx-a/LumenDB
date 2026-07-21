import { redirect } from "next/navigation";

/**
 * 首页：直接重定向到 /dashboard。
 * 中间件会在未登录时把 /dashboard 的访问重定向到 /login，
 * 因此这里无需重复鉴权逻辑。
 */
export default function HomePage() {
  redirect("/dashboard");
}
