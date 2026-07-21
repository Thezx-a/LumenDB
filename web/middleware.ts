import { NextResponse, type NextRequest } from "next/server";
import { jwtVerify, errors as joseErrors } from "jose";

/**
 * 路由守卫：校验 cookie titan_token 中的 JWT。
 * - publicPaths 下的路径直接放行
 * - 其它路径要求 token 有效（签名正确 + 未过期）
 * - 失败则 302 跳转到 /login?from=<原路径>
 *
 * 注意：中间件运行在 Edge Runtime，jose 是为数不多可在 Edge 上运行的 JWT 库。
 */
const JWT_SECRET = process.env.JWT_SECRET ?? "dev-secret-change-in-production";
const PUBLIC_PATHS = ["/login", "/api/auth"];

function getSecretKey(): Uint8Array {
  return new TextEncoder().encode(JWT_SECRET);
}

async function verifyToken(token: string): Promise<boolean> {
  try {
    await jwtVerify(token, getSecretKey(), {
      algorithms: ["HS256"],
    });
    return true;
  } catch (err) {
    if (
      err instanceof joseErrors.JWTExpired ||
      err instanceof joseErrors.JWTClaimValidationFailed ||
      err instanceof joseErrors.JWSInvalid ||
      err instanceof joseErrors.JWSSignatureVerificationFailed
    ) {
      return false;
    }
    // 未知错误，保守起见视为无效
    return false;
  }
}

export async function middleware(req: NextRequest) {
  const { pathname } = req.nextUrl;

  // 公共路径直接放行
  if (PUBLIC_PATHS.some((p) => pathname === p || pathname.startsWith(p + "/"))) {
    return NextResponse.next();
  }

  // 静态资源放行
  if (
    pathname.startsWith("/_next/") ||
    pathname.startsWith("/favicon") ||
    pathname.includes(".")
  ) {
    return NextResponse.next();
  }

  const token = req.cookies.get("titan_token")?.value;

  if (!token) {
    return redirectToLogin(req);
  }

  const ok = await verifyToken(token);
  if (!ok) {
    return redirectToLogin(req);
  }

  return NextResponse.next();
}

function redirectToLogin(req: NextRequest): NextResponse {
  const loginUrl = req.nextUrl.clone();
  loginUrl.pathname = "/login";
  loginUrl.search = "";
  loginUrl.searchParams.set("from", req.nextUrl.pathname + req.nextUrl.search);
  return NextResponse.redirect(loginUrl);
}

export const config = {
  // 匹配所有路径，但内部用 PUBLIC_PATHS 放行白名单
  matcher: ["/((?!_next/static|_next/image|favicon.ico).*)"],
};
