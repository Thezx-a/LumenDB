import type { ReactNode } from "react";
import { Nav } from "@/components/nav";

/**
 * Dashboard 布局：左侧侧边栏 + 右侧内容区。
 * 中间件已确保进入此布局的用户都已通过 JWT 校验。
 */
export default function DashboardLayout({ children }: { children: ReactNode }) {
  return (
    <div className="flex min-h-screen bg-background">
      <Nav />
      <main className="flex-1 overflow-x-hidden">
        <div className="mx-auto max-w-7xl px-8 py-8">{children}</div>
      </main>
    </div>
  );
}
