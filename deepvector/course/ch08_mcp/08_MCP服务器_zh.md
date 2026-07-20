# 绗叓绔狅細MCP 鏈嶅姟鍣?
> MCP (Model Context Protocol) 鈥?璁?Agent 妗嗘灦鍗虫彃鍗崇敤 DeepVector銆?
## 鍓嶇疆鐭ヨ瘑

> 馃搸 **鍙傝€?*: [Python鐜](../prerequisites/02_Python鐜_zh.md)

---

## 瀛︿範鐩爣

- 鐞嗚В MCP 鍗忚鐨勬牳蹇冩蹇?- 鎺屾彙 MCP Server 鐨勫疄鐜?- 瀛︿細閫氳繃 MCP 闆嗘垚 Agent 妗嗘灦

---

## 8.1 浠€涔堟槸 MCP锛?
MCP (Model Context Protocol) 鏄?AI Agent 棰嗗煙鐨?USB 鎺ュ彛"鏍囧噯鍗忚銆?
```
浼犵粺鏂瑰紡:
  Agent Framework 鈫?閫傞厤鍣?鈫?DeepVector SDK 鈫?DeepVector
                      鈫?                  姣忎釜妗嗘灦閮借鍐?
MCP 鏂瑰紡:
  Agent Framework 鈫?MCP Client 鈫?MCP Server 鈫?DeepVector
                                    鈫?                              涓€娆″疄鐜帮紝鍒板浣跨敤
```

---

## 8.2 MCP 宸ュ叿瀹氫箟

AgenticDB 閫氳繃 MCP 鏆撮湶 6 涓伐鍏凤細

| 宸ュ叿鍚?| 鍔熻兘 | 鍙傛暟 |
|--------|------|------|
| vector_search | 璇箟鎼滅储 | query, k, collection |
| filtered_search | 甯﹁繃婊ゆ悳绱?| query, filter, k, collection |
| add_documents | 鎵归噺娣诲姞 | texts, metadatas, collection |
| get_collection_info | 闆嗗悎淇℃伅 | collection |
| list_collections | 鍒楀嚭闆嗗悎 | - |
| delete_document | 鍒犻櫎鏂囨。 | id, collection |

---

## 8.3 鏍稿績瀹炵幇

```python
# MCP 宸ュ叿瀹氫箟 (JSON Schema 鏍煎紡)
MCP_TOOLS = [
    {
        "name": "vector_search",
        "description": "Search DeepVector for semantically similar documents",
        "inputSchema": {
            "type": "object",
            "properties": {
                "query": {"type": "string"},
                "k": {"type": "integer", "default": 10},
                "collection": {"type": "string", "default": "default"},
            },
            "required": ["query"],
        },
    },
]

# 宸ュ叿璋冪敤澶勭悊
async def call_tool(name: str, arguments: dict):
    if name == "vector_search":
        result = await tools.vector_search(
            arguments["query"],
            arguments.get("k", 10),
            arguments.get("collection", "default"),
        )
        return [TextContent(type="text", text=result)]
```

---

## 8.4 闆嗘垚绀轰緥

```python
# LangChain 闆嗘垚
from langchain.agents import AgentExecutor, create_openai_functions_agent
from langchain_community.tools import Tool

tools = [
    Tool(
        name="vector_search",
        func=lambda q: call_mcp("vector_search", {"query": q}),
        description="Search documents",
    )
]

agent = create_openai_functions_agent(llm, tools, prompt)
agent_executor = AgentExecutor(agent=agent, tools=tools)
```

---

## 鎬濊€冮

1. MCP 鐨?stdio 浼犺緭妯″紡鍜?SSE 浼犺緭妯″紡鍚勬湁浠€涔堥€傜敤鍦烘櫙锛?2. 濡傛灉 MCP Server 杩斿洖閿欒锛孉gent 妗嗘灦搴旇鎬庝箞澶勭悊锛?3. MCP 宸ュ叿鐨勫弬鏁版牎楠屽簲璇ョ敱璋佸仛锛烻erver 绔繕鏄?Client 绔紵

## 鍔ㄦ墜缁冧範

1. 鍚姩 MCP Server锛岀敤 `mcp` CLI 宸ュ叿璋冪敤 `vector_search`
2. 娣诲姞涓€涓柊鐨?MCP 宸ュ叿 `batch_search`锛屾敮鎸佷竴娆℃悳绱㈠涓?query
3. 鍦?LangChain 涓垱寤轰竴涓娇鐢?DeepVector MCP 宸ュ叿鐨?Agent
