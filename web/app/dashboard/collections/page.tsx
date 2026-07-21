"use client";

import { useState } from "react";
import { useQuery, useMutation, useQueryClient } from "@tanstack/react-query";
import { api, HttpError } from "@/lib/api";
import type { Collection, CollectionInput } from "@/lib/types";

/**
 * Collection 管理页：CRUD /api/meta/collections。
 * 字段与后端 services/meta/store.go 对齐：name / ttl_seconds / schema。
 */
export default function CollectionsPage() {
  const queryClient = useQueryClient();
  const [showForm, setShowForm] = useState(false);
  const [form, setForm] = useState<CollectionInput>({
    name: "",
    ttl_seconds: 0,
    schema: {},
  });
  const [schemaText, setSchemaText] = useState("{}");

  const { data, isLoading, error } = useQuery<{ items: Collection[]; count: number }>({
    queryKey: ["collections"],
    queryFn: () => api.get("/api/meta/collections"),
  });

  const createMutation = useMutation({
    mutationFn: (input: CollectionInput) =>
      api.post<Collection>("/api/meta/collections", input),
    onSuccess: () => {
      queryClient.invalidateQueries({ queryKey: ["collections"] });
      setShowForm(false);
      setForm({ name: "", ttl_seconds: 0, schema: {} });
      setSchemaText("{}");
    },
  });

  const deleteMutation = useMutation({
    mutationFn: (name: string) => api.delete(`/api/meta/collections/${name}`),
    onSuccess: () => queryClient.invalidateQueries({ queryKey: ["collections"] }),
  });

  function handleSubmit(e: React.FormEvent) {
    e.preventDefault();
    let schema: Record<string, string> = {};
    try {
      schema = JSON.parse(schemaText || "{}");
    } catch {
      alert("Schema 不是合法 JSON");
      return;
    }
    createMutation.mutate({ ...form, schema });
  }

  const items = data?.items ?? [];

  return (
    <div className="space-y-6">
      <div className="flex items-center justify-between">
        <div>
          <h1 className="text-2xl font-semibold tracking-tight">Collection 管理</h1>
          <p className="mt-1 text-sm text-muted-foreground">
            管理键值集合的元数据（name / TTL / schema）。
          </p>
        </div>
        <button
          type="button"
          onClick={() => setShowForm((v) => !v)}
          className="inline-flex h-9 items-center justify-center rounded-md bg-primary px-4 text-sm font-medium text-primary-foreground shadow hover:bg-primary/90"
        >
          {showForm ? "取消" : "新建 Collection"}
        </button>
      </div>

      {showForm && (
        <form
          onSubmit={handleSubmit}
          className="space-y-4 rounded-lg border bg-card p-6 shadow-sm"
        >
          <div className="grid grid-cols-2 gap-4">
            <div className="space-y-1.5">
              <label className="text-sm font-medium">名称</label>
              <input
                required
                value={form.name}
                onChange={(e) => setForm({ ...form, name: e.target.value })}
                className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm"
                placeholder="my-collection"
              />
            </div>
            <div className="space-y-1.5">
              <label className="text-sm font-medium">TTL（秒，0=永久）</label>
              <input
                type="number"
                min={0}
                value={form.ttl_seconds ?? 0}
                onChange={(e) =>
                  setForm({ ...form, ttl_seconds: Number(e.target.value) })
                }
                className="flex h-9 w-full rounded-md border border-input bg-background px-3 py-1 text-sm"
              />
            </div>
          </div>
          <div className="space-y-1.5">
            <label className="text-sm font-medium">Schema（JSON）</label>
            <textarea
              value={schemaText}
              onChange={(e) => setSchemaText(e.target.value)}
              className="flex min-h-[80px] w-full rounded-md border border-input bg-background px-3 py-1 font-mono text-xs"
              placeholder='{"field": "string"}'
            />
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
        <p className="rounded-md bg-destructive/10 px-3 py-2 text-xs text-destructive">
          {error instanceof HttpError ? error.message : "加载失败"}
        </p>
      )}

      <div className="overflow-hidden rounded-lg border">
        <table className="w-full text-sm">
          <thead className="bg-muted/50">
            <tr>
              <th className="px-4 py-2 text-left font-medium">名称</th>
              <th className="px-4 py-2 text-right font-medium">TTL(秒)</th>
              <th className="px-4 py-2 text-left font-medium">创建时间</th>
              <th className="px-4 py-2 text-right font-medium">操作</th>
            </tr>
          </thead>
          <tbody className="divide-y">
            {isLoading && (
              <tr>
                <td colSpan={4} className="px-4 py-8 text-center text-muted-foreground">
                  加载中…
                </td>
              </tr>
            )}
            {!isLoading && items.length === 0 && (
              <tr>
                <td colSpan={4} className="px-4 py-8 text-center text-muted-foreground">
                  暂无 Collection
                </td>
              </tr>
            )}
            {items.map((c) => (
              <tr key={c.name} className="hover:bg-muted/30">
                <td className="px-4 py-2 font-mono text-xs">{c.name}</td>
                <td className="px-4 py-2 text-right tabular-nums text-muted-foreground">
                  {c.ttl_seconds || "∞"}
                </td>
                <td className="px-4 py-2 text-xs text-muted-foreground">
                  {new Date(c.created_at * 1000).toLocaleString("zh-CN")}
                </td>
                <td className="px-4 py-2 text-right">
                  <button
                    type="button"
                    onClick={() => {
                      if (confirm(`确定删除 ${c.name}？`)) {
                        deleteMutation.mutate(c.name);
                      }
                    }}
                    className="text-xs text-destructive hover:underline"
                  >
                    删除
                  </button>
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>
    </div>
  );
}
