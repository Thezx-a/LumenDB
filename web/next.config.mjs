/** @type {import('next').NextConfig} */
const nextConfig = {
  reactStrictMode: true,
  // 将后端 Gateway 的 API 代理到同源，避免浏览器跨域问题（可选）。
  // 这里保留直连 NEXT_PUBLIC_API_BASE 的方式，故不强制 rewrites。
  async rewrites() {
    return [];
  },
};

export default nextConfig;
