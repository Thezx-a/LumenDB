import type { ReactNode } from "react";

/**
 * shadcn/ui 风格的 Card 组件，纯 div + Tailwind，无外部依赖。
 * 提供 Card / CardHeader / CardTitle / CardDescription / CardContent / CardFooter。
 */

export function Card({ className = "", ...props }: React.HTMLAttributes<HTMLDivElement>) {
  return (
    <div
      className={
        "rounded-lg border bg-card text-card-foreground shadow-sm " + className
      }
      {...props}
    />
  );
}

export function CardHeader({
  className = "",
  ...props
}: React.HTMLAttributes<HTMLDivElement>) {
  return (
    <div className={"flex flex-col space-y-1.5 p-6 " + className} {...props} />
  );
}

export function CardTitle({
  className = "",
  ...props
}: React.HTMLAttributes<HTMLHeadingElement>) {
  return (
    <h3
      className={"text-sm font-medium leading-none tracking-tight " + className}
      {...props}
    />
  );
}

export function CardDescription({
  className = "",
  ...props
}: React.HTMLAttributes<HTMLParagraphElement>) {
  return (
    <p className={"text-xs text-muted-foreground " + className} {...props} />
  );
}

export function CardContent({
  className = "",
  ...props
}: React.HTMLAttributes<HTMLDivElement>) {
  return <div className={"p-6 pt-0 " + className} {...props} />;
}

export function CardFooter({
  className = "",
  ...props
}: React.HTMLAttributes<HTMLDivElement>) {
  return (
    <div className={"flex items-center p-6 pt-0 " + className} {...props} />
  );
}

/** 单个指标展示卡：标题 + 主数值 + 可选副文案/趋势 */
export function MetricCard({
  title,
  value,
  hint,
  children,
}: {
  title: string;
  value: ReactNode;
  hint?: string;
  children?: ReactNode;
}) {
  return (
    <Card>
      <CardHeader className="pb-2">
        <CardDescription>{title}</CardDescription>
        <CardTitle className="text-2xl font-semibold tabular-nums">{value}</CardTitle>
      </CardHeader>
      {(hint || children) && (
        <CardContent className="pt-0">
          {hint && <p className="text-xs text-muted-foreground">{hint}</p>}
          {children}
        </CardContent>
      )}
    </Card>
  );
}
