"use client";

import { useState } from "react";
import { API_BASE } from "@/lib/api";
import type { KVPair } from "@/lib/types";

/**
 * 数据浏览页：Scan KV。
 * 后端 /api/data/scan 返回 SSE 流（每条 data: {key,value}\n\n，结尾 event: end），
 * 这里用 fetch + ReadableStream 解析，逐条追加到列表。
 */
export default function DataPage() {
  const [start, setStart] = useState("");
  const [end, setEnd] = useState("");
  const [items, setItems] = useState<KVPair[]>([]);
  const [count, setCount] = useState<number | null>(null);
  const [loading, setLoading] = useState(false);
  const [error, setError] = useState<string | null>(null);

  async function handleScan(e: React.FormEvent) {
    e.preventDefault();
    setLoading(true);
    setError(null);
    setItems([]);
    setCount(null);

    try {
      const url = new URL(`${API_BASE}/api/data/scan`);
      if (start) url.searchParams.set("start", start);
      if (end) url.searchParams.set("end", end);

      const res = await fetch(url.toString(), {
        method: "GET",
        credentials: "include",
      });
      if (!res.ok) {
        throw new Error(`扫描失败 (${res.status})`);
      }
      if (!res.body) {
        throw new Error("无响应流");
      }

      const reader = res.body.getReader();
      const decoder = new TextDecoder();
      let buf = "";
      let collected: KVPair[] = [];

      while (true) {
        const { done, value } = await reader.read();
        if (done) break;
        buf += decoder.decode(value, { stream: true });

        // 按 "\n\n" 切分 SSE 帧
        let idx: number;
        while ((idx = buf.indexOf("\n\n")) >= 0) {
          const frame = buf.slice(0, idx);
          buf = buf.slice(idx + 2);
          // 解析 frame：可能含 event: xxx 和 data: xxx
          const lines = frame.split("\n");
          let eventName = "message";
          let dataLine = "";
          for (const line of lines) {
            if (line.startsWith("event:")) {
              eventName = line.slice(6).trim();
            } else if (line.startsWith("data:")) {
              dataLine = line.slice(5).trim();
            }
          }
          if (eventName === "end") {
            try {
              const payload = JSON.parse(dataLine) as { count: number };
              setCount(payload.count);
            } catch {
              // 忽略
            }
          } else if (dataLine) {
            try {
              const pair = JSON.parse(dataLine) as KVPair;
              collected.push(pair);
              // 每 50 条刷新一次，避免频繁 re-render
              if (collected.length % 50 === 0) {
                setItems([...collected]);
              }
            } catch {
              // 忽略解析失败
            }
          }
        }
      }
      setItems([...collected]);
    } catch (err) {
      setError(err instanceof Error ? err.message : "扫描失败");
    } finally {
      setLoading(false);
    }
  }

  return (
    <div className="space-y-6">
      <div>
        <h1 className="text-2xl font-semibold tracking-tight">数据浏览</h1>
        <p className="mt-1 text-sm text-muted-foreground">
          按区间扫描键值对（SSE 流式返回）。
        </p>
      </div>

      <form onSubmit={handleScan} className="flex gap-2">
        <input
          type="text"
          value={start}
          onChange={(e) => setStart(e.target.value)}
          placeholder="起始 key（含）"
          className="flex h-9 w-40 rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
        />
        <input
          type="text"
          value={end}
          onChange={(e) => setEnd(e.target.value)}
          placeholder="结束 key（不含）"
          className="flex h-9 w-40 rounded-md border border-input bg-background px-3 py-1 text-sm shadow-sm focus-visible:outline-none focus-visible:ring-1 focus-visible:ring-ring"
        />
        <button
          type="submit"
          disabled={loading}
          className="inline-flex h-9 items-center justify-center rounded-md bg-primary px-4 text-sm font-medium text-primary-foreground shadow hover:bg-primary/90 disabled:opacity-50"
        >
          {loading ? "扫描中…" : "扫描"}
        </button>
      </form>

      {error && (
        <p className="rounded-md bg-destructive/10 px-3 py-2 text-xs text-destructive">
          {error}
        </p>
      )}

      <div className="overflow-hidden rounded-lg border">
        <table className="w-full text-sm">
          <thead className="bg-muted/50">
            <tr>
              <th className="px-4 py-2 text-left font-medium">Key</th>
              <th className="px-4 py-2 text-left font-medium">Value</th>
            </tr>
          </thead>
          <tbody className="divide-y">
            {items.length === 0 && !loading && (
              <tr>
                <td colSpan={2} className="px-4 py-8 text-center text-muted-foreground">
                  点击「扫描」加载数据
                </td>
              </tr>
            )}
            {items.map((kv, i) => (
              <tr key={`${kv.key}-${i}`} className="hover:bg-muted/30">
                <td className="px-4 py-2 font-mono text-xs">{kv.key}</td>
                <td className="max-w-md truncate px-4 py-2 font-mono text-xs text-muted-foreground">
                  {kv.value}
                </td>
              </tr>
            ))}
          </tbody>
        </table>
      </div>

      {count !== null && (
        <p className="text-xs text-muted-foreground">共扫描 {count} 条</p>
      )}
    </div>
  );
}
