# 绗?0绔狅細HTTP鏈嶅姟鍣ㄨ璁?
## 鍓嶇疆鐭ヨ瘑

> 馃搸 **鍙傝€?*: [鏋勫缓鐜閰嶇疆](../prerequisites/01_鏋勫缓鐜閰嶇疆.md) 鈥?C++ 缂栬瘧鐜涓庡伐鍏烽摼

---

## 鐩綍
1. [HTTP绠€鍙瞉(#1-http绠€鍙?
2. [浠€涔堟槸Socket锛焆(#2-浠€涔堟槸socket)
3. [鏂囦欢鎻忚堪绗︼細閫氱敤鍙ユ焺](#3-鏂囦欢鎻忚堪绗﹂€氱敤鍙ユ焺)
4. [TCP Socket鍒嗘璇﹁В](#4-tcp-socket鍒嗘璇﹁В)
5. [闃诲涓庨潪闃诲I/O](#5-闃诲涓庨潪闃诲io)
6. [I/O澶氳矾澶嶇敤锛欳10K闈╁懡](#6-io澶氳矾澶嶇敤c10k闈╁懡)
7. [HTTP/1.1鍗忚娣卞叆鍓栨瀽](#7-http11鍗忚娣卞叆鍓栨瀽)
8. [REST API璁捐](#8-rest-api璁捐)
9. [璁よ瘉涓庢巿鏉僝(#9-璁よ瘉涓庢巿鏉?
10. [浼橀泤鍏抽棴](#10-浼橀泤鍏抽棴)
11. [鐢熶骇鐜HTTP鏈嶅姟鍣╙(#11-鐢熶骇鐜http鏈嶅姟鍣?
12. [璁ㄨ闂](#12-璁ㄨ闂)
13. [缁冧範棰榏(#13-缁冧範棰?

---

## 1. HTTP绠€鍙?
### 1989-1991锛氫竾缁寸綉鐨勮癁鐢?
1989骞?鏈堬紝**Tim Berners-Lee**鈥斺€斾竴浣嶅湪**CERN**锛圕onseil Europ茅en pour la Recherche Nucl茅aire锛屾娲叉牳瀛愮爺绌剁粍缁囷紝浣嶄簬鏃ュ唴鐡︼級宸ヤ綔鐨勮嫳鍥界瀛﹀鈥斺€旀彁浜や簡涓€浠介涓?淇℃伅绠＄悊锛氫竴浠芥彁妗?鐨勬姤鍛娿€備粬鐨勭洰鏍囨槸锛氳鐗╃悊瀛﹀浠兘澶熷湪CERN鍙婁箣澶栫殑璁＄畻鏈虹綉缁滀笂鍏变韩鐮旂┒鏂囨。銆?
鍒?991骞村簳锛孊erners-Lee宸茬粡鍒涘缓浜嗕笁椤瑰熀纭€鎶€鏈細

- **HTTP**锛堣秴鏂囨湰浼犺緭鍗忚锛夆€斺€斾竴绉嶇畝鍗曠殑璇锋眰-鍝嶅簲鍗忚锛岀敤浜庤幏鍙栨枃妗?- **HTML**锛堣秴鏂囨湰鏍囪璇█锛夆€斺€斾竴绉嶅甫鏈夎秴閾炬帴鐨勬枃妗ｆ牸寮忚瑷€
- **URL**锛堢粺涓€璧勬簮瀹氫綅绗︼級鈥斺€斾竴绉嶅湪缃戠粶涓畾浣嶆枃妗ｇ殑鍛藉悕鏂规

绗竴涓増鏈?*HTTP/0.9**锛?991骞达級鎸夌幇浠ｆ爣鍑嗘潵鐪嬫瀬鍏剁畝鍗曘€傚畠鍙湁涓€涓柟娉曗€斺€擿GET`鈥斺€斿搷搴旀槸娌℃湁澶撮儴鐨勫師濮婬TML锛?
```
GET /index.html\r\n
\r\n
<html>A page of text</html>
```

娌℃湁鐘舵€佺爜锛屾病鏈夊ご閮紝娌℃湁鐗堟湰鎺у埗銆傚鎴风杩炴帴銆佸彂閫佷竴琛屾暟鎹€佹帴鏀禜TML锛岀劧鍚庢湇鍔″櫒鍏抽棴杩炴帴銆傚氨杩欐牱銆傚崗璁殑瀹屾暣瑙勮寖鐢ㄤ竴娈佃瘽灏辫兘姒傛嫭銆?
### 1993-1996锛欻TTP/1.0涓嶮osaic鐨勭垎鍙?
1993骞?*Mosaic**鈥斺€旂涓€涓浘褰㈠寲缃戦〉娴忚鍣ㄢ€斺€旂殑鍙戝竷鐐圭噧浜嗕竾缁寸綉銆傜獊鐒堕棿姣忎釜浜洪兘鎯充笂缃戙€侶TTP闇€瑕佹垚闀夸簡銆?
**HTTP/1.0**锛?996骞存寮忓寲涓篟FC 1945锛夊鍔犱簡锛?- **璇锋眰鍜屽搷搴斿ご閮?*鈥斺€斿`Content-Type`銆乣Content-Length`銆乣Date`绛夊厓鏁版嵁
- **鐘舵€佺爜**鈥斺€斿`200 OK`銆乣404 Not Found`绛夋暟瀛楃粨鏋滅爜
- **澶氱鏂规硶**鈥斺€擿GET`銆乣POST`銆乣HEAD`
- **鐗堟湰瀛楃涓?*鈥斺€旇姹傝涓殑`HTTP/1.0`

浣咹TTP/1.0鏈変竴涓嚧鍛界己闄凤細**姣忎釜璇锋眰閮介渶瑕佷竴涓柊鐨凾CP杩炴帴**銆備竴涓姞杞?0寮犲浘鐗囩殑椤甸潰闇€瑕?1涓嫭绔嬬殑TCP杩炴帴銆傛瘡娆¤繛鎺ラ兘闇€瑕乀CP涓夋鎻℃墜锛圫YN 鈫?SYN-ACK 鈫?ACK锛夛紝鍦ㄦ暟鎹紶杈撲箣鍓嶅ぇ绾︽秷鑰?*1.5涓線杩旀椂寤讹紙RTT锛?*銆傚湪100ms鐨勮繛鎺ヤ笂锛屾瘡娆¤姹備粎杩炴帴寤虹珛灏辫娴垂150ms銆?
### 1997锛欻TTP/1.1涓庢寔涔呰繛鎺?
**HTTP/1.1**锛圧FC 2068锛屽悗鐢盧FC 2616鍜孯FC 7230鏇存柊锛夐€氳繃**鎸佷箙杩炴帴**锛堥粯璁Connection: keep-alive`锛夎В鍐充簡杩欎釜闂銆傜幇鍦ㄤ竴涓猅CP杩炴帴鍙互鎵胯浇鏁板崄涓『搴忚姹傘€傝繖涓増鏈湪鎺ヤ笅鏉ョ殑浜屽崄骞翠腑涓诲浜嗕竾缁寸綉銆?
HTTP/1.1杩樺鍔犱簡锛?- **Host澶撮儴**鈥斺€旀瘡涓姹備腑蹇呴』鍖呭惈锛屾敮鎸佽櫄鎷熶富鏈猴紙涓€涓狪P涓婂涓煙鍚嶏級
- **鍒嗗潡浼犺緭缂栫爜**鈥斺€旀棤闇€棰勫厛鐭ラ亾鎬诲ぇ灏忓嵆鍙祦寮忎紶杈撳搷搴?- **鍐呭鍗忓晢**鈥斺€斿鎴风鍜屾湇鍔″櫒灏辨牸寮忥紙璇█銆佺紪鐮併€佸獟浣撶被鍨嬶級杈炬垚涓€鑷?- **缂撳瓨鎺у埗**鈥斺€擿Cache-Control`銆乣ETag`銆乣If-None-Match`澶撮儴

### 2000锛歊EST涓嶢PI闈╁懡

2000骞达紝**Roy Fielding**鈥斺€擧TTP/1.1鐨勪富瑕佷綔鑰呬箣涓€鈥斺€斿彂琛ㄤ簡鍗氬＋璁烘枃"鏋舵瀯椋庢牸涓庡熀浜庣綉缁滅殑杞欢鏋舵瀯璁捐"銆備粬鍦ㄨ鏂囦腑姝ｅ紡鎻忚堪浜?*REST**锛堣〃杩版€х姸鎬佽浆绉伙級锛屼竴绉嶄粬浠庣爺绌朵竾缁寸綉鏈韩涓彁鐐煎嚭鏉ョ殑鏋舵瀯椋庢牸銆?
REST鎴愪负Web API鐨勪富娴佽寖寮忥紝鍙栦唬浜嗕笓鏈塕PC鍗忚锛圫OAP銆乆ML-RPC銆丆ORBA锛夌殑娣蜂贡灞€闈€?
### 2015-2022锛欻TTP/2鍜孒TTP/3

**HTTP/2**锛圧FC 7540锛?015骞达級浠庢枃鏈崗璁浆鍚戜簡浜岃繘鍒跺抚锛屽鍔犱簡澶氳矾澶嶇敤锛堜竴涓繛鎺ヤ笂鍚屾椂杩涜澶氫釜璇锋眰锛夈€佸ご閮ㄥ帇缂╋紙HPACK锛夊拰鏈嶅姟鍣ㄦ帹閫併€?
**HTTP/3**锛圧FC 9114锛?022骞达級瀹屽叏鐢?*QUIC**鈥斺€斾竴绉嶇敱Google寮€鍙戠殑鍗忚鈥斺€斿彇浠ｄ簡TCP銆?
---

## 2. 浠€涔堟槸Socket锛?
### 鐢佃瘽绫绘瘮

**Socket**鏄竴涓綉缁滈€氫俊绔偣鈥斺€斾竴涓涓ゅ彴涓嶅悓鏈哄櫒涓婄殑绋嬪簭浜ゆ崲鏁版嵁鐨勬娊璞°€?
| 鐢佃瘽绫绘瘮 | Socket鎿嶄綔 | 绯荤粺璋冪敤 |
|---------|-----------|---------|
| 涔颁竴閮ㄧ數璇?| 鍒涘缓绔偣 | `socket()` |
| 鑾峰緱鐢佃瘽鍙风爜 | 鍒嗛厤鍦板潃 | `bind()` |
| 鎵撳紑鍝嶉搩锛岀瓑寰呮潵鐢?| 鏍囪涓鸿鍔紝鎺掗槦浼犲叆杩炴帴 | `listen()` |
| 鎺ュ惉鐢佃瘽 | 鎺ュ彈杩炴帴 | `accept()` |
| 涓庢潵鐢佃€呬氦璋?| 浜ゆ崲鏁版嵁 | `send()` / `recv()` |
| 鎸傛柇鐢佃瘽 | 鍏抽棴杩炴帴 | `close()` |

### Socket绫诲瀷

- **SOCK_STREAM**锛圱CP锛夆€斺€斿彲闈犵殑銆佹湁搴忕殑銆侀潰鍚戣繛鎺ョ殑瀛楄妭娴?- **SOCK_DGRAM**锛圲DP锛夆€斺€斾笉鍙潬鐨勩€佹棤搴忕殑銆佹棤杩炴帴鐨勬暟鎹姤

### 鍦板潃鏃?
- **AF_INET**鈥斺€擨Pv4鍦板潃锛?2浣嶏級
- **AF_INET6**鈥斺€擨Pv6鍦板潃锛?28浣嶏級
- **AF_UNIX**鈥斺€擴nix鍩熷鎺ュ瓧锛屽悓涓€鍙版満鍣ㄤ笂鐨勮繘绋嬮棿閫氫俊

---

## 3. 鏂囦欢鎻忚堪绗︼細閫氱敤鍙ユ焺

### "涓€鍒囩殕鏂囦欢"

Unix鏈変竴涓憲鍚嶇殑鍝插鍘熷垯锛?*"涓€鍒囩殕鏂囦欢"**銆?
缁熶竴瀹冧滑鐨勬娊璞℃槸**鏂囦欢鎻忚堪绗?*锛堥€氬父缂╁啓涓?*fd**锛夛細涓€涓〃绀鸿繘绋嬩腑宸叉墦寮€鐨処/O璧勬簮鐨勫皬鐨勯潪璐熸暣鏁般€?
```
鏂囦欢鎻忚堪绗﹁〃锛堟瘡杩涚▼锛夛細
鈹屸攢鈹€鈹€鈹€鈹€鈹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?鈹? 0  鈹? stdin  (鏍囧噯杈撳叆)     鈹?  鈫?鐢盋杩愯鏃惰嚜鍔ㄦ墦寮€
鈹? 1  鈹? stdout (鏍囧噯杈撳嚭)     鈹?鈹? 2  鈹? stderr (鏍囧噯閿欒)     鈹?鈹? 3  鈹? 鏈嶅姟鍣ㄥ鎺ュ瓧 (绔彛8080) 鈹?  鈫?socket()杩斿洖鏈€浣庡彲鐢ㄧ殑fd
鈹? 4  鈹? 瀹㈡埛绔繛鎺?           鈹?鈹? 5  鈹? 瀹㈡埛绔繛鎺?           鈹?鈹?... 鈹? ...                 鈹?鈹斺攢鈹€鈹€鈹€鈹€鈹粹攢鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹?```

---

## 4. TCP Socket鍒嗘璇﹁В

### 娴忚鍣ㄨ繛鎺ユ椂鍙戠敓浜嗕粈涔?
#### 姝ラ1锛氭湇鍔″櫒璁剧疆

1. 鍒涘缓浜嗗鎺ュ瓧锛歚socket(AF_INET, SOCK_STREAM, 0)`鈥斺€旇繑鍥瀎d 3
2. 璁剧疆浜哷SO_REUSEADDR`鈥斺€斿厑璁搁噸鍚悗绔嬪嵆閲嶆柊缁戝畾绔彛
3. 缁戝畾鍒板湴鍧€锛歚bind(fd, "0.0.0.0:8080")`
4. 寮€濮嬬洃鍚細`listen(fd, 128)`

#### 姝ラ2锛歍CP涓夋鎻℃墜

```
娴忚鍣?                           鏈嶅姟鍣?  鈹?                               鈹?  鈹傗攢鈹€鈹€鈹€ SYN (seq=1000) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤
  鈹?                               鈹?  鈹傗梽鈹€鈹€鈹€ SYN-ACK (seq=3000, ack=1001) 鈹€鈹€鈹?  鈹?                               鈹?  鈹傗攢鈹€鈹€鈹€ ACK (ack=3001) 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈻衡攤
  鈹?                               鈹?  鈹?    杩炴帴宸插缓绔?                 鈹?```

#### 姝ラ3锛欻TTP璇锋眰

```
GET /api/health HTTP/1.1\r\n
Host: localhost:8080\r\n
User-Agent: Mozilla/5.0\r\n
Accept: */*\r\n
\r\n
```

#### 姝ラ4锛欻TTP鍝嶅簲

```
HTTP/1.1 200 OK\r\n
Content-Type: application/json\r\n
Content-Length: 15\r\n
Connection: close\r\n
\r\n
{"status":"ok"}
```

#### 姝ラ5锛氳繛鎺ュ叧闂?
```mermaid
sequenceDiagram
    participant Browser as 娴忚鍣?    participant Server as 鏈嶅姟鍣?
    Note over Browser,Server: TCP涓夋鎻℃墜 (1.5 RTT)
    Browser->>Server: SYN (seq=1000)
    Server-->>Browser: SYN-ACK (seq=3000, ack=1001)
    Browser->>Server: ACK (ack=3001)

    Note over Browser,Server: HTTP璇锋眰/鍝嶅簲
    Browser->>Server: GET /api/health HTTP/1.1\r\nHost: ...
    Server-->>Browser: HTTP/1.1 200 OK\r\nContent-Type: ...\r\n{"status":"ok"}

    Note over Browser,Server: TCP鍥涙鎸ユ墜
    Server-->>Browser: FIN
    Browser->>Server: ACK
    Browser->>Server: FIN
    Server-->>Browser: ACK
```

### SO_REUSEADDR锛氫负浠€涔堜綘闇€瑕佸畠

褰撲竴涓猅CP杩炴帴鍏抽棴鏃讹紝瀹冧細杩涘叆**TIME_WAIT**鐘舵€侊紝鎸佺画鏃堕棿涓?*鏈€澶ф鐢熷瓨鏈燂紙MSL锛?*鐨勪袱鍊嶃€俙SO_REUSEADDR`琛ㄧず锛?鎴戠煡閬撴垜鍦ㄥ仛浠€涔堬紝鍗充娇鏈塗IME_WAIT涔熻鎴戠粦瀹氥€?

---

## 5. 闃诲涓庨潪闃诲I/O

### 闃诲I/O锛氶粯璁よ涓?
**闃诲I/O**鏄渶绠€鍗曠殑妯″瀷銆傚綋浣犺皟鐢╜recv()`涓旀病鏈夋暟鎹彲鐢ㄦ椂锛屾搷浣滅郴缁熻浣犵殑绾跨▼杩涘叆鐫＄湢鐘舵€併€?
**闃诲I/O鐨勬牳蹇冮棶棰?*锛氫竴涓闃诲鍦ㄤ竴涓繛鎺ヤ笂鐨勭嚎绋嬫棤娉曞鐞嗕换浣曞叾浠栬繛鎺ャ€傚鏋滀綘鏈?0,000涓鎴风锛屼綘闇€瑕?0,000涓嚎绋嬨€?
### 闈為樆濉濱/O锛氫笉瑕佺潯鐪狅紝绔嬪嵆鍛婅瘔鎴?
```cpp
#include <fcntl.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}
```

### 瀵规瘮锛氫笁绉岻/O妯″瀷

| 妯″瀷 | 绾跨▼琛屼负 | CPU浣跨敤 | 鍙墿灞曟€?| 澶嶆潅搴?|
|------|---------|---------|---------|--------|
| **闃诲I/O** | 鐫＄湢鐩村埌鏁版嵁鍒拌揪 | 浣庯紙绛夊緟鏃剁┖闂诧級 | 宸細姣忎釜杩炴帴涓€涓嚎绋?| 绠€鍗?|
| **闈為樆濉濱/O + 蹇欒疆璇?* | 鍦ㄥ惊鐜腑杞鎵€鏈塮d | 100%锛堝缁堝湪鑷棆锛?| 宸細姣忔杩唬O(n) | 涓瓑 |
| **浜嬩欢椹卞姩锛堝璺鐢級** | 鍦╜epoll_wait()`涓潯鐪犵洿鍒颁簨浠惰Е鍙?| 浣庯紙浠呭湪鏈夊伐浣滄椂娲昏穬锛?| 浼樼锛氫竴涓嚎绋嬶紝鏁板崈涓繛鎺?| 澶嶆潅 |

---

## 6. I/O澶氳矾澶嶇敤锛欳10K闈╁懡

### C10K闂

1999骞达紝杞欢宸ョ▼甯?*Dan Kegel**鍙戣〃浜嗕竴绡囬噷绋嬬寮忕殑鏂囩珷"C10K闂"銆備粬闂亾锛?*濡備綍鏋勫缓涓€涓鐞?0,000涓苟鍙戣繛鎺ョ殑鏈嶅姟鍣紵**

### 澶氳矾澶嶇敤鐨勬紨杩?
| 绯荤粺璋冪敤 | 骞翠唤 | 鏈€澶D鏁?| 鏌ユ壘澶嶆潅搴?| 涓昏闄愬埗 |
|---------|------|---------|-----------|---------|
| **select()** | 1983 (4.2BSD) | 1024 (`FD_SETSIZE`) | O(n)鎵弿 | 纭紪鐮侀檺鍒?|
| **poll()** | 1987 (SVR3 Unix) | 鏃犻檺鍒?| O(n)鎵弿 | 浠嶇劧鏄疧(n) |
| **epoll()** | 2002 (Linux 2.5) | 鏃犻檺鍒?| O(1)浠呰繑鍥炲氨缁殑fd | Linux涓撳睘 |
| **kqueue** | 2000 (FreeBSD 4.1) | 鏃犻檺鍒?| O(1) | BSD/macOS涓撳睘 |
| **io_uring** | 2019 (Linux 5.1) | 鏃犻檺鍒?| O(1) | 寮傛锛涘墠娌挎妧鏈?|

### epoll()锛歀inux鐨勯潻鍛?
**epoll**鐨勬牳蹇冩礊瀵熸槸锛?*娉ㄥ唽涓€娆★紝姘镐箙鐩戞帶銆?*

```mermaid
flowchart TD
    A[浜嬩欢寰幆] --> B{epoll_wait}
    B -->|浜嬩欢灏辩华| C{鏄痵erver_fd鍚?}
    B -->|瓒呮椂| D[澶勭悊瀹氭椂鍣╙

    C -->|鏄瘄 E[鎺ュ彈鏂拌繛鎺
    C -->|鍚 F[recv瀹㈡埛绔暟鎹甝

    E --> G[add_fd鍒癳poll]
    G --> B

    F --> H{鏁版嵁璇诲彇瀹屾瘯?}
    H -->|鏄瘄 I[鍏抽棴fd]
    H -->|鍚 J[handle_data]
    J --> B

    I --> B
    D --> B

    style A fill:#e8f5e9,stroke:#2e7d32
    style B fill:#fff3e0,stroke:#f57c00
    style E fill:#e1f5fe,stroke:#0288d1
    style F fill:#fce4ec,stroke:#c62828
```

```cpp
#include <sys/epoll.h>

class EpollEventLoop {
    int epoll_fd_;
    static constexpr int MAX_EVENTS = 1024;

public:
    EpollEventLoop() {
        epoll_fd_ = epoll_create1(0);
    }

    void add_fd(int fd, uint32_t events) {
        struct epoll_event ev{};
        ev.events = events;
        ev.data.fd = fd;
        epoll_ctl(epoll_fd_, EPOLL_CTL_ADD, fd, &ev);
    }

    void run(int server_fd) {
        add_fd(server_fd, EPOLLIN);
        struct epoll_event events[MAX_EVENTS];

        while (true) {
            int nfds = epoll_wait(epoll_fd_, events, MAX_EVENTS, -1);

            for (int i = 0; i < nfds; i++) {
                int fd = events[i].data.fd;

                if (fd == server_fd) {
                    while (true) {
                        int client = accept4(server_fd, nullptr, nullptr,
                                            SOCK_NONBLOCK);
                        if (client < 0) break;
                        add_fd(client, EPOLLIN | EPOLLET);
                    }
                } else {
                    char buf[4096];
                    while (true) {
                        ssize_t n = recv(fd, buf, sizeof(buf), 0);
                        if (n > 0) {
                            handle_data(fd, buf, n);
                        } else if (n == 0) {
                            close(fd);
                            break;
                        } else if (errno == EAGAIN) {
                            break;
                        } else {
                            close(fd);
                            break;
                        }
                    }
                }
            }
        }
    }
};
```

### LT涓嶦T锛氫袱绉嶈Е鍙戞ā寮?
| 妯″紡 | 鍚嶇О | 琛屼负 | 浣曟椂浣跨敤 |
|------|------|------|---------|
| **LT** | 姘村钩瑙﹀彂锛堥粯璁わ級 | 鍙缂撳啿鍖烘湁鏁版嵁灏遍€氱煡浣?| 绠€鍗曞畨鍏?|
| **ET** | 杈圭紭瑙﹀彂 | 鍙湪鐘舵€佹敼鍙樻椂閫氱煡浣犱竴娆?| 楂樻€ц兘锛屾洿澶嶆潅 |

---

## 7. HTTP/1.1鍗忚娣卞叆鍓栨瀽

### HTTP璇锋眰缁撴瀯

```
POST /api/v1/search HTTP/1.1\r\n           鈫?璇锋眰琛岋細鏂规硶  璺緞  鐗堟湰
Host: localhost:8080\r\n                    鈫?澶撮儴锛堥敭鍊煎锛?Content-Type: application/json\r\n
Content-Length: 45\r\n
Authorization: Bearer token123\r\n
\r\n                                        鈫?绌鸿 = 澶撮儴缁撴潫
{"query": [0.1, 0.2, 0.3], "top_k": 10}    鈫?涓讳綋
```

### HTTP鍝嶅簲缁撴瀯

```
HTTP/1.1 200 OK\r\n                         鈫?鐘舵€佽锛氱増鏈? 鐘舵€佺爜  鍘熷洜鐭
Content-Type: application/json\r\n          鈫?澶撮儴
Content-Length: 62\r\n
Connection: keep-alive\r\n
\r\n                                        鈫?绌鸿 = 澶撮儴缁撴潫
{"results": [{"id": 42, "score": 0.95}]}   鈫?涓讳綋
```

### 鐘舵€佺爜锛氱粨鏋滅殑璇█

```
1xx 淇℃伅鎬?  100 Continue          鏈嶅姟鍣ㄥ凡鎺ユ敹璇锋眰澶?  101 Switching Protocols  鏈嶅姟鍣ㄥ悓鎰忓崌绾?
2xx 鎴愬姛
  200 OK                璇锋眰鎴愬姛
  201 Created           鏂拌祫婧愬凡鍒涘缓
  204 No Content        鎴愬姛锛屼絾娌℃湁涓讳綋杩斿洖

3xx 閲嶅畾鍚?  301 Moved Permanently  璧勬簮姘镐箙绉诲姩鍒版柊URL
  304 Not Modified      缂撳瓨鍝嶅簲浠嶇劧鏈夋晥

4xx 瀹㈡埛绔敊璇?  400 Bad Request       鏍煎紡閿欒鐨勮姹?  401 Unauthorized      鏈彁渚涜璇佸嚟鎹?  403 Forbidden         宸茶璇佷絾鏈巿鏉冭闂璧勬簮
  404 Not Found         姝RI澶勪笉瀛樺湪璧勬簮
  405 Method Not Allowed  涓嶆敮鎸佺殑HTTP鏂规硶
  422 Unprocessable Entity  璇硶鏈夋晥浣嗚涔夐敊璇?  429 Too Many Requests   瓒呭嚭閫熺巼闄愬埗

5xx 鏈嶅姟鍣ㄩ敊璇?  500 Internal Server Error  鏈嶅姟鍣ㄤ笂鍑轰簡闂
  502 Bad Gateway           涓婃父鏈嶅姟鍣ㄨ繑鍥炰簡鏃犳晥鍝嶅簲
  503 Service Unavailable   鏈嶅姟鍣ㄦ殏鏃惰繃杞芥垨鍋滄満缁存姢
```

### 澶撮儴锛氬厓鏁版嵁灞?
| 澶撮儴 | 鐢ㄩ€?| 绀轰緥 |
|------|------|------|
| `Host` | 姝よ姹傞拡瀵瑰摢涓煙鍚?| `Host: example.com` |
| `Content-Type` | 涓讳綋鐨凪IME绫诲瀷 | `Content-Type: application/json` |
| `Content-Length` | 涓讳綋鐨勫瓧鑺傞暱搴?| `Content-Length: 45` |
| `Authorization` | 璁よ瘉鍑嵁 | `Authorization: Bearer sk-...` |
| `Connection` | 杩炴帴绠＄悊 | `Connection: keep-alive` |

### JSON锛氶€氱敤浜ゆ崲鏍煎紡

```json
{
    "id": 42,
    "vector": [0.1, 0.2, 0.3],
    "metadata": {
        "source": "document.pdf",
        "page": 5
    }
}
```

---

## 8. REST API璁捐

### 浠€涔堟槸REST锛?
**REST**锛堣〃杩版€х姸鎬佽浆绉伙級鏄?*Roy Fielding**鍦ㄥ叾2000骞村崥澹鏂囦腑鎻忚堪鐨勬灦鏋勯鏍笺€俁EST鐨勬牳蹇冪害鏉燂細

1. **璧勬簮閫氳繃URI鏍囪瘑**
2. **缁熶竴鎺ュ彛**锛氭搷浣滀娇鐢℉TTP鏂规硶
3. **鏃犵姸鎬?*锛氭瘡涓姹傚寘鍚湇鍔″櫒闇€瑕佺殑鎵€鏈変俊鎭?4. **琛ㄨ堪**锛氳祫婧愪互琛ㄨ堪锛圝SON銆乆ML绛夛級鐨勫舰寮忎紶杈?5. **鍒嗗眰绯荤粺**锛氫腑闂翠欢鍙互浣嶄簬瀹㈡埛绔拰鏈嶅姟鍣ㄤ箣闂?
### DeepVector鐨凴ESTful璺敱

```
POST   /api/v1/vectors         鎻掑叆涓€涓悜閲?GET    /api/v1/vectors/{id}    妫€绱竴涓悜閲?DELETE /api/v1/vectors/{id}    鍒犻櫎涓€涓悜閲?POST   /api/v1/search          鐩镐技鎬ф悳绱?GET    /api/v1/health          鍋ュ悍妫€鏌?GET    /api/v1/stats           缁熻淇℃伅
```

### 缁熶竴JSON鍝嶅簲鏍煎紡

```json
// 鎴愬姛
{
    "status": "ok",
    "data": { "id": 42, "results": [{"id": 7, "score": 0.95}] }
}

// 閿欒
{
    "status": "error",
    "error": {
        "code": "DIMENSION_MISMATCH",
        "message": "Expected dimension 768, got 1024"
    }
}
```

### 璺敱鍣細浜ら€氳瀵?
```cpp
class Router {
    std::unordered_map<std::string,
        std::function<HttpResponse(HttpRequest&)>> routes_;

public:
    void get(const std::string& path,
             std::function<HttpResponse(HttpRequest&)> handler) {
        routes_["GET " + path] = handler;
    }
    void post(const std::string& path,
              std::function<HttpResponse(HttpRequest&)> handler) {
        routes_["POST " + path] = handler;
    }

    HttpResponse dispatch(HttpRequest& req) {
        std::string key = req.method + " " + req.path;
        auto it = routes_.find(key);
        if (it != routes_.end()) {
            return it->second(req);
        }
        return HttpResponse(404,
            R"({"status":"error","error":{"code":"NOT_FOUND"}})");
    }
};
```

### CORS锛氳法婧愯祫婧愬叡浜?
**CORS**锛堣法婧愯祫婧愬叡浜級鏄竴绉嶆帶鍒跺摢浜涚綉椤靛彲浠ヤ粠涓嶅悓鍩熷悕璇锋眰璧勬簮鐨勫畨鍏ㄦ満鍒躲€?
---

## 9. 璁よ瘉涓庢巿鏉?
### Bearer Token璁よ瘉

```
Authorization: Bearer sk-lumen-test-token
```

### JWT锛氳嚜鍖呭惈浠ょ墝

**JWT**锛圝SON Web Token锛夋槸RFC 7519涓畾涔夌殑鑷寘鍚€佸彲楠岃瘉鐨勪护鐗屾牸寮忋€?
```
eyJhbGciOiJIUzI1NiJ9.eyJzdWIiOiJ1c2VyMTIzIiwiZXhwIjoxNjkwMDg2NDAwfQ.abc123def456
鈹溾攢鈹€ 澶撮儴 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹も敎鈹€鈹€ 杞借嵎 鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹€鈹も敎鈹€鈹€ 绛惧悕 鈹€鈹€鈹€鈹?```

- **澶撮儴**锛氱鍚嶇畻娉?- **杞借嵎**锛氬０鏄庘€斺€旂敤鎴稩D銆佽繃鏈熸椂闂淬€佹潈闄?- **绛惧悕**锛氫娇鐢ㄥ瘑閽ョ殑HMAC鎴朢SA绛惧悕

### 鍒锋柊浠ょ墝+璁块棶浠ょ墝妯″紡

- **璁块棶浠ょ墝**锛氱煭瀵垮懡锛?-15鍒嗛挓锛夛紝鐢ㄤ簬API璋冪敤
- **鍒锋柊浠ょ墝**锛氶暱瀵垮懡锛堟暟澶?鏁板懆锛夛紝浠呯敤浜庤幏鍙栨柊鐨勮闂护鐗?
---

## 10. 浼橀泤鍏抽棴

### 浼橀泤鍏抽棴锛氫簲涓楠?
```
1. 鎺ユ敹SIGTERM 鈹€鈹€鈻?璁剧疆鏍囧織锛歡_running = false
2. close(listen_fd) 鈹€鈹€鈻?鍋滄鎺ュ彈鏂拌繛鎺?3. 鎺掔┖杩涜涓殑璇锋眰 鈹€鈹€鈻?璁╁綋鍓嶈姹傚畬鎴愶紙瀹介檺鏈燂紝閫氬父10-30绉掞級
4. 鎸佷箙鍖栫姸鎬?鈹€鈹€鈻?灏嗙储寮曞埛鏂板埌纾佺洏锛屽啓鍏AL
5. exit(0) 鈹€鈹€鈻?骞插噣閫€鍑?```

### 瀹屾暣瀹炵幇

```cpp
#include <csignal>

volatile sig_atomic_t g_running = 1;

void signal_handler(int sig) {
    g_running = 0;
}

int main() {
    signal(SIGINT, signal_handler);
    signal(SIGTERM, signal_handler);

    int server_fd = create_server(8080);

    while (g_running) {
        epoll_wait(...);
    }

    printf("Shutting down gracefully...\n");
    close(server_fd);

    auto deadline = std::chrono::steady_clock::now() + std::chrono::seconds(30);
    while (!active_connections.empty() &&
           std::chrono::steady_clock::now() < deadline) {
        poll_remaining_connections(100);
    }

    for (int fd : active_connections) {
        shutdown(fd, SHUT_RDWR);
        close(fd);
    }

    index_.flush();
    printf("Shutdown complete.\n");
    return 0;
}
```

---

## 11. 鐢熶骇鐜HTTP鏈嶅姟鍣?
### nginx

**nginx** 涓虹害 33-34% 鐨勭綉绔欐彁渚涙湇鍔★紙W3Techs, 2025锛夈€傚畠浣跨敤浜嬩欢椹卞姩锛坋poll/kqueue锛夋ā鍨嬪鐞嗘墍鏈塈/O銆?
### 涓轰粈涔堜簨浠堕┍鍔ㄦā鍨嬪崰鎹富瀵煎湴浣?
1. **鏃犵嚎绋嬪紑閿€**锛氫竴涓嚎绋嬪鐞嗘暟鍗冧釜杩炴帴
2. **鏃犻攣**锛氬崟绾跨▼ = 娌℃湁浜掓枼閿併€佹病鏈夌珵鎬佹潯浠躲€佹病鏈夋閿?3. **鍙娴嬬殑寤惰繜**锛氭病鏈変笂涓嬫枃鍒囨崲銆佹病鏈夐攣浜夌敤
4. **楂樻晥鐨勫唴瀛樹娇鐢?*锛氫竴涓爤浠ｆ浛鏁板崈涓爤

浠ｄ环锛氫唬鐮佹洿澶嶆潅銆備綘涓嶈兘鍦ㄤ簨浠跺惊鐜腑杩涜闃诲璋冪敤銆?
---

## 12. 璁ㄨ闂

1. 涓轰粈涔坄select()`鐨刞FD_SETSIZE`琚檺鍒朵负1024锛熻繖鏄妧鏈檺鍒惰繕鏄璁￠€夋嫨锛?2. 鍦╡poll鐨凟T妯″紡涓嬶紝濡傛灉浣犲湪鍒拌揪`EAGAIN`涔嬪墠璺冲嚭`recv`寰幆锛屽墿浣欐暟鎹粈涔堟椂鍊欎細琚鐞嗭紵
3. HTTP/1.1鐨刞Connection: keep-alive`鍜孒TTP/2鐨勫璺鐢ㄤ箣闂寸殑鏍规湰鍖哄埆鏄粈涔堬紵
4. 濡傛灉瀹㈡埛绔彂閫佺殑`Content-Length`涓庡疄闄呬富浣撻暱搴︿笉鍖归厤锛屾湇鍔″櫒搴斿浣曢槻寰★紵
5. 璁捐涓€涓粦鍔ㄧ獥鍙ｉ€熺巼闄愬埗鍣?6. 鏃犵姸鎬丣WT璁よ瘉濡備綍澶勭悊"鐢ㄦ埛琚皝绂佷絾浠ょ墝灏氭湭杩囨湡"锛?7. `SO_REUSEADDR`涓巂SO_REUSEPORT`鍒嗗埆瑙ｅ喅浠€涔堥棶棰橈紵
8. 鍦╜close(fd)`涔嬪悗锛宖d缂栧彿绔嬪嵆琚玚accept()`閲嶇敤銆傛柊杩炴帴鏄惁浼氭剰澶栬鍙栨棫杩炴帴鐨勮繃鏈熸暟鎹紵

---

## 13. 缁冧範棰?
### 缁冧範1锛氬崟绾跨▼epoll HTTP鏈嶅姟鍣紙35鍒嗛挓锛?
浣跨敤epoll锛圗T妯″紡锛変粠闆舵瀯寤轰竴涓狧TTP鏈嶅姟鍣紝鏀寔锛?- `GET /health` 鈫?`200 OK {"status":"ok"}`
- `POST /echo` 鈫?杩斿洖璇锋眰涓讳綋
- 鎵嬪姩HTTP璇锋眰瑙ｆ瀽

### 缁冧範2锛氬畬鏁碦EST API锛?0鍒嗛挓锛?
涓哄悜閲忔暟鎹簱瀹炵幇CRUD REST API銆?
### 缁冧範3锛氳璇佷腑闂翠欢锛?0鍒嗛挓锛?
瀹炵幇Bearer token璁よ瘉涓棿浠躲€?
### 缁冧範4锛氫紭闆呭叧闂?Keep-Alive锛?5鍒嗛挓锛?
### 缁冧範5锛氬帇鍔涙祴璇曪紙20鍒嗛挓锛?
```bash
wrk -t 4 -c 100 -d 30s --latency http://localhost:8080/api/v1/health
```

---

## 绔犺妭鎬荤粨

| 灞?| 鏍稿績鎶€鏈?| 鍏抽敭姒傚康 |
|----|---------|---------|
| **浼犺緭灞?* | Berkeley Sockets API | fd, socket() 鈫?bind() 鈫?listen() 鈫?accept() 鈫?send()/recv() 鈫?close() |
| **澶氳矾澶嶇敤** | select 鈫?poll 鈫?epoll/kqueue | C10K闂锛汱T涓嶦T锛汷(1)浜嬩欢閫氱煡 |
| **鍗忚灞?* | HTTP/1.1 | 璇锋眰琛?+ 澶撮儴 + 涓讳綋锛汯eep-Alive锛汣ontent-Length涓巆hunked |
| **搴旂敤灞?* | REST API | 璧勬簮URI + HTTP鏂规硶锛圕RUD锛夛紱缁熶竴JSON鍝嶅簲锛涚姸鎬佺爜 |
| **瀹夊叏灞?* | 璁よ瘉 | Bearer Token锛汮WT涓夐儴鍒嗙粨鏋勶紱璁よ瘉涓庢巿鏉?|
| **杩愮淮灞?* | 淇″彿澶勭悊 | SIGTERM 鈫?鎺掔┖璇锋眰 鈫?鎸佷箙鍖?鈫?閫€鍑猴紱瀹介檺鏈?|

> 涓嬩竴绔狅細[绗?1绔狅細C++20鍗忕▼涓嶴kyNet](../ch11_coroutines/README.md)