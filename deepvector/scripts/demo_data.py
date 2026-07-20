"""
Demo data for AgenticDB 鈥?sample documents for testing.
"""

SAMPLE_DOCUMENTS = [
    {
        "text": "RAG (Retrieval-Augmented Generation) 鏄竴绉嶅皢妫€绱㈢郴缁熶笌澶ц瑷€妯″瀷缁撳悎鐨勬妧鏈灦鏋勩€傞€氳繃浠庡閮ㄧ煡璇嗗簱妫€绱㈢浉鍏虫枃妗ｏ紝LLM鑳藉鐢熸垚鏇村噯纭€佹洿鏈変緷鎹殑鍥炵瓟銆俁AG鐨勬牳蹇冩祦绋嬪寘鎷細鏌ヨ鐞嗚В銆佹枃妗ｆ绱€佷笂涓嬫枃缁勮鍜岀瓟妗堢敓鎴愩€?,
        "tags": "topic:RAG,type:overview,lang:zh",
    },
    {
        "text": "HNSW (Hierarchical Navigable Small World) 鏄竴绉嶉珮鏁堢殑杩戜技鏈€杩戦偦鎼滅储绠楁硶銆傚畠閫氳繃鏋勫缓澶氬眰鍥剧粨鏋勫疄鐜板揩閫熸悳绱細椤跺眰鍖呭惈灏戦噺鑺傜偣鐢ㄤ簬绮楃矑搴﹀鑸紝搴曞眰鍖呭惈鎵€鏈夎妭鐐圭敤浜庣簿纭悳绱€侶NSW鐨勬悳绱㈡椂闂村鏉傚害涓篛(log n)銆?,
        "tags": "topic:HNSW,type:theory,lang:zh",
    },
    {
        "text": "Product Quantization (PQ) 鏄竴绉嶅悜閲忓帇缂╂妧鏈紝閫氳繃灏嗛珮缁村悜閲忓垎鍓叉垚澶氫釜瀛愮┖闂村苟瀵规瘡涓瓙绌洪棿杩涜鑱氱被閲忓寲锛屽疄鐜板ぇ骞呭帇缂┿€備緥濡傦紝涓€涓?68缁寸殑float32鍚戦噺鍙互閫氳繃PQ鍘嬬缉鍒?4瀛楄妭锛屽帇缂╂瘮杈惧埌48鍊嶃€?,
        "tags": "topic:quantization,type:theory,lang:zh",
    },
    {
        "text": "mmap (memory-mapped file) 鏄竴绉嶅皢鏂囦欢鏄犲皠鍒拌繘绋嬪湴鍧€绌洪棿鐨勬妧鏈€備娇鐢╩map鍙互瀹炵幇闆舵嫹璐濊鍙栵紝鍥犱负鎿嶄綔绯荤粺浼氭寜闇€灏嗘枃浠堕〉闈㈠姞杞藉埌鍐呭瓨涓€傝繖瀵逛簬鍚戦噺鏁版嵁搴撶殑瀛樺偍寮曟搸闈炲父鏈夌敤锛屽彲浠ラ伩鍏嶉澶栫殑鏁版嵁澶嶅埗銆?,
        "tags": "topic:storage,type:theory,lang:zh",
    },
    {
        "text": "鍚戦噺鏁版嵁搴撶殑鏍稿績鎬ц兘鎸囨爣鍖呮嫭锛氬彫鍥炵巼(Recall)銆佹煡璇㈠欢杩?Latency)鍜屽悶鍚愰噺(Throughput)銆傚湪鐧句竾绾ф暟鎹泦涓婏紝浼樼鐨勫悜閲忔暟鎹簱搴旇杈惧埌95%浠ヤ笂鐨勫彫鍥炵巼锛屽悓鏃朵繚鎸佹绉掔骇鐨勬煡璇㈠欢杩熴€?,
        "tags": "topic:performance,type:benchmark,lang:zh",
    },
    {
        "text": "C++20 寮曞叆浜嗗崗绋?coroutine)鏀寔锛屼娇寰楀紓姝ョ紪绋嬫洿鍔犵畝娲併€俢o_await銆乧o_return鍜宑o_yield鏄笁涓牳蹇冨叧閿瓧銆傚崗绋嬪彲浠ュ疄鐜伴浂寮€閿€鐨勫紓姝ユ搷浣滐紝閬垮厤浜嗗洖璋冨湴鐙卞拰绾跨▼寮€閿€銆?,
        "tags": "topic:cpp,type:theory,lang:zh",
    },
    {
        "text": "LSM-Tree (Log-Structured Merge-Tree) 鏄竴绉嶅啓浼樺寲鐨勫瓨鍌ㄥ紩鎿庛€傚畠灏嗗啓鎿嶄綔棣栧厛璁板綍鍒癢AL鍜孧emTable涓紝鐒跺悗瀹氭湡鍚堝苟鍒癝STable鏂囦欢涓€侺SM-Tree鐨勫啓鍏ュ悶鍚愰噺寰堥珮锛屼絾璇诲彇鏃堕渶瑕佹鏌ュ涓眰绾с€?,
        "tags": "topic:storage,type:theory,lang:zh",
    },
    {
        "text": "鍏冩暟鎹繃婊ゆ槸鍚戦噺鎼滅储鐨勯噸瑕佸姛鑳姐€傜敤鎴峰彲浠ラ€氳繃鏍囩銆佹椂闂存埑銆佺被鍒瓑缁村害瀵规悳绱㈢粨鏋滆繘琛岃繃婊ゃ€備緥濡傦紝鍙悳绱㈢壒瀹氱被鍒笅鐨勬枃妗ｏ紝鎴栬€呭彧杩斿洖鏈€杩戞坊鍔犵殑缁撴灉銆傝繃婊ゅ彲浠ュ湪鎼滅储鍓?pre-filter)鎴栨悳绱㈠悗(post-filter)鎵ц銆?,
        "tags": "topic:filtering,type:overview,lang:zh",
    },
    {
        "text": "Pybind11 鏄竴涓交閲忕骇鐨凜++/Python缁戝畾搴擄紝鍙互杞绘澗鍦板皢C++鍑芥暟鍜岀被鏆撮湶缁橮ython銆傚畠浣跨敤绾ご鏂囦欢瀹炵幇锛屾敮鎸佺幇浠++鐗规€э紝鐢熸垚鐨凱ython妯″潡鎬ц兘鎺ヨ繎鍘熺敓C++浠ｇ爜銆?,
        "tags": "topic:python,type:theory,lang:zh",
    },
    {
        "text": "MCP (Model Context Protocol) 鏄竴绉嶆爣鍑嗗寲鐨勫崗璁紝鐢ㄤ簬杩炴帴AI妯″瀷涓庡閮ㄥ伐鍏峰拰鏁版嵁婧愩€傞€氳繃MCP锛屼换浣旳I浠ｇ悊妗嗘灦閮藉彲浠ヤ互缁熶竴鐨勬柟寮忚皟鐢ㄥ閮ㄦ湇鍔★紝瀹炵幇宸ュ叿璋冪敤鍜屾暟鎹闂€?,
        "tags": "topic:MCP,type:overview,lang:zh",
    },
    {
        "text": "鍚戦噺鎼滅储鐨勫簲鐢ㄥ満鏅寘鎷細璇箟鎼滅储銆佹帹鑽愮郴缁熴€佸浘鍍忔绱€佷唬鐮佹悳绱€佸紓甯告娴嬪拰闂瓟绯荤粺銆傚湪鐢熶骇鐜涓紝閫氬父闇€瑕佺粨鍚堝厓鏁版嵁杩囨护銆佸垎灞傜储寮曞拰缂撳瓨绛夋妧鏈潵婊¤冻涓嶅悓鐨勬€ц兘闇€姹傘€?,
        "tags": "topic:applications,type:overview,lang:zh",
    },
    {
        "text": "Agent (鏅鸿兘浣? 鏄竴绉嶈兘澶熻嚜涓昏鍒掋€佹墽琛屼换鍔″苟鏍规嵁鍙嶉璋冩暣绛栫暐鐨凙I绯荤粺銆侫gentic AI鐨勬牳蹇冭兘鍔涘寘鎷細浠诲姟鍒嗚В銆佸伐鍏蜂娇鐢ㄣ€佽蹇嗙鐞嗗拰鑷垜鍙嶆€濄€俁eAct (Reasoning + Acting) 鏄渶缁忓吀鐨凙gent鑼冨紡涔嬩竴銆?,
        "tags": "topic:agent,type:overview,lang:zh",
    },
    {
        "text": "Inner Product (鍐呯Н) 鍜?Cosine Similarity (浣欏鸡鐩镐技搴? 鏄袱绉嶅父鐢ㄧ殑鍚戦噺鐩镐技搴﹀害閲忋€傚唴绉绠椾袱涓悜閲忕殑鐐圭Н锛岃€屼綑寮︾浉浼煎害璁＄畻涓や釜鍚戦噺澶硅鐨勪綑寮﹀€笺€傚綋鍚戦噺宸插綊涓€鍖栨椂锛屼袱鑰呯瓑浠枫€?,
        "tags": "topic:distance,type:theory,lang:zh",
    },
    {
        "text": "AVX2 (Advanced Vector Extensions 2) 鏄疘ntel/AMD澶勭悊鍣ㄧ殑SIMD鎸囦护闆嗭紝鍙互鍚屾椂澶勭悊8涓?2浣嶆诞鐐规暟銆傚埄鐢ˋVX2鍙互灏嗗悜閲忚窛绂昏绠楃殑鎬ц兘鎻愬崌4-8鍊嶃€傚悜閲忔暟鎹簱鐨勬€ц兘浼樺寲閫氬父浠嶴IMD鎸囦护闆嗗紑濮嬨€?,
        "tags": "topic:SIMD,type:optimization,lang:zh",
    },
    {
        "text": "澶氳疆妫€绱?Multi-round Retrieval)鏄竴绉嶈凯浠ｅ紡鐨勬悳绱㈢瓥鐣ャ€傜涓€杞繘琛屽垵姝ユ悳绱紝鐒跺悗璇勪及缁撴灉璐ㄩ噺銆傚鏋滅粨鏋滀笉澶熷ソ锛岀郴缁熶細鑷姩鐢熸垚鏀硅繘鐨勬煡璇㈣繘琛岀浜岃疆鎼滅储锛岀洿鍒扮粨鏋滄弧瓒宠川閲忚姹傘€?,
        "tags": "topic:retrieval,type:advanced,lang:zh",
    },
]


async def insert_demo_data(lumendb_url: str = "http://localhost:8080"):
    """Insert sample documents into DeepVector."""
    import httpx

    print(f"Inserting {len(SAMPLE_DOCUMENTS)} documents into DeepVector...")

    async with httpx.AsyncClient() as client:
        for i, doc in enumerate(SAMPLE_DOCUMENTS):
            try:
                # First embed the text
                embed_resp = await client.post(
                    f"{lumendb_url}/embed",
                    json={"text": doc["text"]},
                )
                if embed_resp.status_code == 200:
                    vector = embed_resp.json()["vector"]
                else:
                    print(f"  Embed failed for doc {i}, using random vector")
                    import random
                    vector = [random.random() for _ in range(768)]

                # Then insert
                insert_resp = await client.post(
                    f"{lumendb_url}/insert",
                    json={"vector": vector},
                )
                if insert_resp.status_code == 200:
                    doc_id = insert_resp.json()["ids"][0]
                    print(f"  [{i+1}/{len(SAMPLE_DOCUMENTS)}] Inserted doc {doc_id}")
                else:
                    print(f"  [{i+1}/{len(SAMPLE_DOCUMENTS)}] Insert failed: {insert_resp.text}")

            except Exception as e:
                print(f"  [{i+1}/{len(SAMPLE_DOCUMENTS)}] Error: {e}")

    print("Done!")


if __name__ == "__main__":
    import asyncio
    asyncio.run(insert_demo_data())
