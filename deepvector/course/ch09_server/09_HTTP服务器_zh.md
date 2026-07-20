# 绗節绔狅細HTTP 鏈嶅姟鍣?
> Agent Server 鈥?FastAPI 鍜岀畝鍗?HTTP 鍙屾ā寮忋€?
## 鍓嶇疆鐭ヨ瘑

> 馃搸 **鍙傝€?*: [Python鐜](../prerequisites/02_Python鐜_zh.md) | [閰嶇疆绯荤粺](../ch03_config/03_閰嶇疆绯荤粺_zh.md)

---

## 瀛︿範鐩爣

- 鐞嗚В鍙屾ā寮忔湇鍔″櫒璁捐
- 鎺屾彙 API 绔偣璁捐
- 瀛︿細 FastAPI 鍜岀函 Python 鐨勯€夋嫨绛栫暐

---

## 9.1 鍙屾ā寮忔灦鏋?
```mermaid
flowchart TD
    subgraph Client["瀹㈡埛绔?]
        C1[curl]
        C2["Python SDK"]
        C3["娴忚鍣?]
    end
    
    subgraph Server["Agent Server"]
        F[FastAPI (鎺ㄨ崘)]
        S[Simple HTTP (鍥為€€)]
    end
    
    Client --> F
    Client --> S
    
    F --> LLM[LLM Router]
    S --> LLM
    
    LLM --> Engine[MultiRound Engine]
    Engine --> LDB[DeepVector HTTP API]
```

涓ょ妯″紡鐨勯€夋嫨锛?
| 妯″紡 | 渚濊禆 | 閫傜敤鍦烘櫙 | 鍔熻兘 |
|------|------|----------|------|
| FastAPI | fastapi + uvicorn | 鐢熶骇閮ㄧ讲 | OpenAPI 鏂囨。, 鑷姩鏍￠獙, WebSocket |
| Simple HTTP | 鏃犻澶栦緷璧?| 鏈€灏忓寲閮ㄧ讲, 宓屽叆寮?| 鍩虹璺敱, JSON 鍝嶅簲 |

---

## 9.2 API 绔偣

| 绔偣 | 鏂规硶 | 璇存槑 |
|------|------|------|
| `/health` | GET | 鍋ュ悍妫€鏌?|
| `/query` | POST | 瀹屾暣妫€绱?(瑙勫垝+鎵ц+鍥炵瓟) |
| `/ask` | POST | 绠€娲侀棶绛?|
| `/plan` | POST | 浠呯敓鎴愭绱㈣鍒?|

`/query` 璇锋眰绀轰緥:

```json
{
    "question": "瀵规瘮 HNSW 鍜?IVF 鐨勪紭缂虹偣",
    "collection": "default",
    "max_rounds": 3
}
```

`/query` 鍝嶅簲绀轰緥:

```json
{
    "answer": "HNSW 閫氳繃鍒嗗眰鍥剧粨鏋勫疄鐜?..",
    "documents": [
        {"id": 1, "distance": 0.12, "text": "HNSW search..."}
    ],
    "strategy": "multi_query",
    "rounds": 2,
    "quality_score": 0.85
}
```

---

## 9.3 閿欒澶勭悊

```python
try:
    if path == "/query" and method == "POST":
        resp_data = await handle_query(engine, body)
    else:
        resp_data = {"error": "not found"}
        status = 404
except Exception as e:
    resp_data = {"error": str(e)}
    status = 500
```

鎵€鏈夊紓甯搁兘浼氳鎹曡幏骞惰繑鍥?JSON 鏍煎紡鐨勯敊璇秷鎭紝涓嶄細鏆撮湶鍐呴儴鍫嗘爤銆?
---

## 鎬濊€冮

1. FastAPI 鐨勮嚜鍔ㄨ姹傛牎楠?(Pydantic) 鐩告瘮鎵嬪姩瑙ｆ瀽鏈変粈涔堜紭鍔匡紵
2. 濡傛灉璇锋眰 body 涓殑 `question` 闀垮害瓒呰繃 10000 瀛楃锛屽簲璇ユ€庝箞澶勭悊锛?3. 濡備綍缁?Agent Server 娣诲姞璇锋眰闄愭祦 (Rate Limiting)锛?
## 鍔ㄦ墜缁冧範

1. 鍦?Simple HTTP 妯″紡涓鍔?CORS 澶存敮鎸?2. 缁?`/query` 娣诲姞 `stream` 鍙傛暟锛屽疄鐜?SSE 娴佸紡杩斿洖
3. 瀹炵幇涓€涓?`/batch/query` 绔偣锛屼竴娆″鐞嗗涓棶棰?