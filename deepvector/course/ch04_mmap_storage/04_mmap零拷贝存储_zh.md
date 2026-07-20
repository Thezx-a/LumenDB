# 绗洓绔狅細闆舵嫹璐濆瓨鍌?鈥?铏氭嫙鍐呭瓨涓?mmap 鎸佷箙鍖?
## 鍓嶇疆鐭ヨ瘑

> 馃搸 **鍙傝€?*: [鏋勫缓鐜閰嶇疆](../prerequisites/01_鏋勫缓鐜閰嶇疆.md)

---

## 瀛︿範鐩爣
- 鐞嗚В铏氭嫙鍐呭瓨鐨勬牳蹇冩蹇碉細鍦板潃绌洪棿銆侀〉銆侀〉琛ㄣ€乀LB銆佺己椤靛紓甯?- 璁よ瘑 mmap 鐨勫伐浣滃師鐞嗗強"闆舵嫹璐?鐨勭湡姝ｅ惈涔?- 鎺屾彙鎿嶄綔绯荤粺椤电紦瀛橈紙page cache锛夊強鑴忛〉鍥炲啓鏈哄埗
- 鐞嗚В MAP_SHARED vs MAP_PRIVATE 鐨勫樊寮傚強鍐欐椂澶嶅埗锛圕oW锛?- 璁捐鍙寔涔呭寲銆佸彲鎵╁睍鐨勭鐩樻暟鎹牸寮?- 瀹夊叏鍦版墿灞?mmap 鏄犲皠鍖哄煙锛坒truncate + remap锛?- 鐞嗚В msync 涓庢暟鎹寔涔呮€т繚闅?- 鍒濇浜嗚В WAL锛圵rite-Ahead Log锛変笌宕╂簝鎭㈠

---

## 4.0 闂鐨勮捣鐐癸細璇讳竴涓?1GB 鏂囦欢锛屼负浠€涔堝繀椤诲鍒?1GB 鏁版嵁锛?
鏈€鐩磋鐨勫仛娉曟槸 `read()` 绯荤粺璋冪敤銆備綘璋冪敤 `read(fd, buf, size)`锛屾搷浣滅郴缁熸妸鏂囦欢鍐呭浠庣鐩樻惉杩愬埌浣犳彁渚涚殑缂撳啿鍖恒€?
浣嗚繖寮曞嚭涓€涓叧閿棶棰橈細**鏁版嵁琚鍒朵簡涓ゆ**銆?
```
绗竴娆″鍒讹細纾佺洏 鈫?鎿嶄綔绯荤粺鍐呮牳鐨?椤电紦瀛?锛坧age cache锛?绗簩娆″鍒讹細椤电紦瀛?鈫?浣犵殑 buf锛堢敤鎴风┖闂寸紦鍐插尯锛?```

濡傛灉浣犵殑鏁版嵁搴撴枃浠舵湁 1GB锛岄偅涔?`read()` 灏卞繀椤诲湪鍐呭瓨涓惉杩?1GB 鐨勬暟鎹袱娆°€傚湪 DDR4-3200 鍐呭瓨锛堢悊璁哄甫瀹界害 25 GB/s锛変笂锛岃繖娴垂浜嗗ぇ绾?80ms 鐨勭函鎷疯礉鏃堕棿銆?
**鏍稿績娲炲療**锛氬鏋滄搷浣滅郴缁熻兘璁╀綘鐨勭▼搴?鐩存帴鐪嬪埌"椤电紦瀛樹腑鐨勬暟鎹紝鑰屼笉闇€瑕佹妸鏁版嵁鍐嶅鍒朵竴浠藉埌浣犵殑 buf 涓紝灏辫兘鍚屾椂鐪佹帀 1GB 鐨勫唴瀛樻氮璐瑰拰 1GB 鐨勬嫹璐濆紑閿€銆傝繖灏辨槸 mmap 瑕佽В鍐崇殑闂銆?
---

## 4.1 铏氭嫙鍐呭瓨锛氭搷浣滅郴缁熸渶浼熷ぇ鐨勫彂鏄?
### 4.1.1 濡傛灉鐗╃悊鍐呭瓨鐩存帴鏆撮湶缁欑▼搴忎細鎬庢牱锛?
鍦ㄦ棭鏈熻绠楁満绯荤粺涓紙1970 骞翠唬浠ュ墠锛夛紝璁＄畻鏈虹▼搴忕洿鎺ヤ娇鐢?*鐗╃悊鍐呭瓨鍦板潃**銆傝繖绉嶈璁＄畝鍗曚絾甯︽潵浜嗕笁涓弗閲嶉棶棰橈細

**闂 1锛氳繘绋嬮殧绂讳笉瀛樺湪銆?* 杩涚▼ A 鍐欏叆浜嗗湴鍧€ `0x00100000`锛岃繘绋?B 涔熻杩欎釜鍦板潃鈥斺€擝 鍙互鍋风湅 A 鐨勬暟鎹€?
**闂 2锛氬閮ㄧ鐗囧寲銆?* 鐗╃悊鍐呭瓨琚垏鎴愮鐗囷紝娌℃湁杩炵画鐨勫尯鍩熸弧瓒冲ぇ鍧楀垎閰嶈姹傘€?
**闂 3锛氭棤娉曞仛"鎳掑姞杞?銆?* 杩涚▼鍚戞搷浣滅郴缁熻 1GB 鍐呭瓨锛屼絾杩?1GB 涓?99% 鐨勫尯鍩熷彲鑳芥案杩滀笉浼氳瀹為檯璇诲啓銆?
### 4.1.2 铏氭嫙鍐呭瓨鐨勮В鍐虫柟妗?
**铏氭嫙鍐呭瓨**锛圴irtual Memory锛変负姣忎釜杩涚▼鍒涘缓涓€涓?骞昏"鈥斺€斾竴涓嫭绔嬬殑銆佸法澶х殑**铏氭嫙鍦板潃绌洪棿**銆傝繘绋嬪湪杩欎釜铏氭嫙绌洪棿涓换鎰忓垎閰嶅唴瀛橈紝CPU 鐨?**MMU**锛圡emory Management Unit锛屽唴瀛樼鐞嗗崟鍏冿級璐熻矗鍦ㄥ箷鍚庡皢铏氭嫙鍦板潃缈昏瘧鎴愮墿鐞嗗湴鍧€銆?
### 4.1.3 浠€涔堟槸椤碉紙Page锛夛紵

**椤?*锛坧age锛夋槸铏氭嫙鍐呭瓨绯荤粺鐨勬渶灏忕鐞嗗崟鍏冦€倄86-64 鏍囧噯椤靛ぇ灏忎负 4 KB銆?
### 4.1.4 椤佃〃锛氳櫄鎷熷湴鍧€鍒扮墿鐞嗗湴鍧€鐨勫瓧鍏?
**椤佃〃**锛圥age Table锛夋湰璐ㄤ笂鏄竴涓垎灞傛暟鎹粨鏋勶紙4 鎴?5 绾э級锛岀敱鎿嶄綔绯荤粺缁存姢銆侰PU 姣忔墽琛屼竴鏉″唴瀛樿闂寚浠わ紝閮藉繀椤诲皢铏氭嫙鍦板潃杞崲涓虹墿鐞嗗湴鍧€銆?
### 4.1.5 TLB锛欳PU 鍐呴儴鐨勭炕璇戠紦瀛?
**TLB**锛圱ranslation Lookaside Buffer锛夋槸 MMU 鍐呴儴鐨勭‖浠剁紦瀛橈紝缂撳瓨鏈€杩戜娇鐢ㄧ殑铏氭嫙鈫掔墿鐞嗗湴鍧€缈昏瘧缁撴灉銆傛病鏈?TLB锛屾瘡娆″唴瀛樿闂兘瑕佸厛鍋?4 娆￠〉琛ㄩ亶鍘嗏€斺€旀€ц兘鐏鹃毦銆?
### 4.1.6 缂洪〉寮傚父锛歊AM 涓嶅鐢ㄦ椂鎬庝箞鍔?
### 缂洪〉寮傚父澶勭悊娴佺▼

```mermaid
flowchart TD
    A[绋嬪簭璁块棶 mmap 鍦板潃] --> B{MMU 鏌?TLB}
    B -->|鍛戒腑| C[鐩存帴鑾峰彇鐗╃悊鍦板潃]
    B -->|鏈懡涓瓅 D[閬嶅巻椤佃〃]
    D --> E{PTE 瀛樺湪浣?= 1?}
    E -->|鏄瘄 F[缈昏瘧瀹屾垚]
    E -->|鍚 G[瑙﹀彂缂洪〉寮傚父]
    G --> H{鍦板潃鏄惁鍚堟硶?}
    H -->|鍚 I[SIGSEGV 宕╂簝]
    H -->|鏄瘄 J{椤甸潰鍦ㄩ〉缂撳瓨涓?}
    J -->|鏄瘄 K[Minor Page Fault]
    J -->|鍚 L[Major Page Fault]
    K --> M[鏇存柊椤佃〃]
    L --> M
    M --> N[CPU 閲嶆柊鎵ц鎸囦护]
    F --> O[璁块棶瀹屾垚]
    C --> O

    style G fill:#f44336,color:#fff
    style I fill:#d32f2f,color:#fff
    style K fill:#FF9800,color:#fff
    style L fill:#f44336,color:#fff
```

| 绫诲瀷 | 鍙戠敓鏉′欢 | 寤惰繜 |
|---|---|---|
| Minor | 椤甸潰宸插湪鐗╃悊 RAM 涓紝鍙渶鏇存柊椤佃〃 | ~1-10 碌s |
| Major | 椤甸潰蹇呴』浠庣鐩樿鍙?| ~5-20 ms锛圫SD锛?|
| Invalid | 璁块棶鏈槧灏勭殑鍦板潃 | 杩涚▼缁堟 |

---

## 4.2 mmap锛氳鏂囦欢鍍忓唴瀛樹竴鏍风洿鎺ヨ闂?
### 4.2.1 浠€涔堟槸 mmap锛?
**mmap**锛坢emory map锛屽唴瀛樻槧灏勶級鏄?POSIX 绯荤粺璋冪敤锛岀敤浜庡湪杩涚▼鐨勮櫄鎷熷湴鍧€绌洪棿涓垱寤轰竴娈?*鍐呭瓨鏄犲皠鍖哄煙**銆傝繖娈靛尯鍩熷彲浠ョ洿鎺ラ€氳繃鎸囬拡璁块棶锛屽簳灞傜殑鏁版嵁鐢卞唴鏍歌嚜鍔ㄧ鐞嗐€?
### read() vs mmap() 鏁版嵁娴佸姣?
```mermaid
sequenceDiagram
    participant App as 搴旂敤绋嬪簭
    participant Kernel as 鍐呮牳椤电紦瀛?    participant Disk as 纾佺洏

    rect rgb(255, 230, 230)
    Note over App,Disk: 浼犵粺 read() - 涓ゆ鎷疯礉
    App->>Kernel: read(fd, buf, size)
    Disk->>Kernel: DMA 浼犺緭
    Note over Kernel: 绗竴娆¤惤鐐? 椤电紦瀛?    Kernel->>App: memcpy 鍒扮敤鎴风紦鍐插尯
    Note over App: 绗簩娆¤惤鐐? 鐢ㄦ埛 buf
    end

    rect rgb(230, 255, 230)
    Note over App,Disk: mmap() - 闆?CPU 鎷疯礉
    App->>Kernel: mmap(NULL, size, ...)
    Kernel-->>App: 杩斿洖铏氭嫙鍦板潃鎸囬拡
    App->>App: 鐩存帴璇诲啓鎸囬拡
    Disk->>Kernel: DMA 浼犺緭 (鎸夐渶)
    Note over Kernel,App: 鍚屼竴鐗╃悊椤垫锛屾棤闇€鎷疯礉
    end
```

### 4.2.2 涓轰粈涔堝彨"闆舵嫹璐?锛?
**"闆舵嫹璐?锛坺ero-copy锛?* 杩欎釜鏈鏈変簺澶稿紶鈥斺€旀暟鎹粛鐒堕渶瑕佷粠纾佺洏璇诲彇鍒?RAM锛堟湁鎷疯礉锛夛紝浣?*鐪佸幓浜嗕粠鍐呮牳缂撳啿鍖哄埌鐢ㄦ埛缂撳啿鍖虹殑绗簩娆℃嫹璐?*銆傚浜?3 GB 鐨勫悜閲忔暟鎹簱鏂囦欢锛岀渷鍘讳竴娆?3 GB 鐨?memcpy 鎰忓懗鐫€锛?- 鐪佸幓 3 GB 鐨勫唴瀛樺甫瀹?- 涓嶆氮璐圭墿鐞嗗唴瀛?- 鍑忓皯 CPU 寮€閿€

### 4.2.3 浠€涔堟槸"鎿嶄綔绯荤粺椤电紦瀛?锛?
**鎿嶄綔绯荤粺椤电紦瀛?*锛圥age Cache锛夋槸 Linux 鍐呮牳鍦ㄧ墿鐞?RAM 涓淮鎶ょ殑銆佷笌纾佺洏鏂囦欢鍏宠仈鐨勭紦瀛樸€傛牳蹇冨睘鎬э細

1. 鎸夋枃浠?鍋忕Щ缁勭粐
2. 鑷姩棰勮锛圧eadahead锛?3. 鍥炲啓锛圵riteback锛夛細瀵?MAP_SHARED 鏄犲皠鐨勫啓鍏ヤ細鏍囪椤甸潰涓?鑴?
4. 鍏ㄥ眬鍏变韩锛氬悓涓€涓枃浠惰涓や釜杩涚▼鍒嗗埆 mmap 鏃跺叡浜悓涓€浠介〉缂撳瓨

### 鎿嶄綔绯荤粺鍐呭瓨浣跨敤妯″紡

```mermaid
pie title 鍏稿瀷鏈嶅姟鍣ㄧ殑鐗╃悊鍐呭瓨鍒嗛厤
    搴旂敤杩涚▼鍫嗗拰鏍?: 15
    椤电紦瀛?鏂囦欢鏁版嵁 : 60
    鍐呮牳鑷韩 : 10
    绌洪棽 鍙敤浜庨〉缂撳瓨 : 15
```

### 4.2.4 MAP_SHARED vs MAP_PRIVATE vs MAP_ANONYMOUS

```
MAP_SHARED锛堝叡浜槧灏勶級锛?  鍐欏叆鐩存帴浣滅敤浜庨〉缂撳瓨涓殑椤甸潰锛岃剰椤垫渶缁堣鍐欏洖纾佺洏
  鍏朵粬 mmap 鍚屼竴鏂囦欢鐨勮繘绋嬬湅鍒板啓鍏?  杩欐槸鏈€甯哥敤浜庢寔涔呭寲瀛樺偍鐨勬ā寮?
MAP_PRIVATE锛堢鏈夋槧灏勶級锛?  璇诲彇锛氫笌 SHARED 鐩稿悓
  鍐欏叆锛氳Е鍙戝啓鏃跺鍒讹紙CoW锛夆啋 鍐呮牳鍒嗛厤鏂扮墿鐞嗛〉妗?  鍘熷鏂囦欢涓嶅彉

MAP_ANONYMOUS锛堝尶鍚嶆槧灏勶級锛?  涓嶄笌浠讳綍鏂囦欢鍏宠仈锛岀瓑浠蜂簬 malloc 鐨勫簳灞傚疄鐜?```

### 4.2.5 鍐欐椂澶嶅埗锛欳opy-on-Write锛圕oW锛?
**鍐欐椂澶嶅埗**锛圕oW锛夋槸 MAP_PRIVATE 鑳屽悗鐨勫叧閿妧鏈€傚畠涔熸槸 `fork()` 绯荤粺璋冪敤鐨勫熀纭€鈥斺€斿綋鐖惰繘绋?fork 鍑哄瓙杩涚▼鏃讹紝瀛愯繘绋嬬殑椤佃〃鎸囧悜涓庣埗杩涚▼鐩稿悓鐨勭墿鐞嗛〉闈紝浣嗚繖浜涢〉闈㈠叏閮ㄨ鏍囪涓哄彧璇汇€傚彧鏈夊綋绗竴娆″啓鍏ユ煇涓〉闈㈡椂锛屽唴鏍告墠鐪熸澶嶅埗璇ラ〉銆?
---

## 4.3 鏁版嵁鎸佷箙鎬э細msync 涓庤剰椤电鐞?
### 4.3.1 msync锛?鎴戠幇鍦ㄥ氨瑕佷繚璇佹暟鎹埌浜嗙鐩?

```c
int msync(void* addr, size_t length, int flags);

// flags:
//   MS_ASYNC      鈥?鍙戣捣鍐欏洖璇锋眰锛屽嚱鏁扮珛鍗宠繑鍥?//   MS_SYNC       鈥?闃诲鐩村埌鑴忛〉琚啓鍥炲瓨鍌ㄨ澶?//   MS_INVALIDATE 鈥?浣垮綋鍓嶆槧灏勭殑缂撳瓨澶辨晥
```

**鍏抽敭璀﹀憡**锛歚close(fd)` **涓嶄繚璇佽Е鍙戝啓鍥?*銆傛纭殑鍏抽棴娴佺▼锛?```cpp
msync(ptr, len, MS_SYNC);   // 1. 寮哄埗鍒风洏
munmap(addr, len);           // 2. 瑙ｉ櫎鏄犲皠
close(fd);                   // 3. 鍏抽棴鏂囦欢鎻忚堪绗?```

---

## 4.4 纾佺洏鏍煎紡璁捐锛氬浣曟妸鍚戦噺绱㈠紩鎸佷箙鍖栧埌鏂囦欢涓?
### 4.4.1 DeepVector 鐨勭鐩樺竷灞€

```
鍋忕Щ 0 (offset 0)
+=====================+
| Magic Number (4B)   |  0x4C4D4442 = "LMDB" in ASCII
+---------------------+
| Version (4B)        |  1锛堟枃浠舵牸寮忕増鏈彿锛?+---------------------+
| Dimension (4B)      |  768锛堟瘡涓悜閲忕殑缁村害锛?+---------------------+
| Vector Count (8B)   |  N锛堟暟鎹簱涓瓨鍌ㄧ殑鍚戦噺鎬绘暟锛?+---------------------+
| Metric Type (4B)    |  0=L2, 1=IP, 2=Cosine
+---------------------+
| Flags (4B)          |  bit0: 褰掍竴鍖栨爣蹇? bit1: AVX-512 鍏煎鏍囧織
+---------------------+
| Reserved (40B)      |  鐣欑┖锛屼娇澶撮儴鎬昏涓?64 瀛楄妭锛堜笌缂撳瓨琛屽榻愶級
+=====================+  鍋忕Щ 64
| Index Header (64B)  |  HNSW 鍏冩暟鎹細M, ef_construction, max_level, entry_point_id
+=====================+  鍋忕Щ 128
| Index Graph (鍙橀暱)   |  閭绘帴琛?[node_id][neighbor_count][neighbor_ids 脳 4B]
+=====================+  鍋忕Щ瀵归綈鍒伴〉杈圭晫
| Vector Data         |  [id(8B)][vec_0(4B)]...[vec_D-1(4B)] 脳 N
+=====================+
```

---

## 4.5 瀹夊叏鍦版墿灞?mmap 鏂囦欢

### 璺ㄥ钩鍙版柟妗堬細munmap + ftruncate + mmap

```cpp
class GrowableMmapFile {
    int fd;
    void* ptr;
    size_t mapped_size;

public:
    void grow(size_t new_size) {
        if (new_size <= mapped_size) return;
        new_size = align_up(new_size, 4096);

        msync(ptr, mapped_size, MS_SYNC);     // 1. 鍏堝埛鐩樿剰鏁版嵁
        munmap(ptr, mapped_size);             // 2. 瑙ｉ櫎鏃ф槧灏?        ftruncate(fd, new_size);              // 3. 鎵╁睍鏂囦欢
        ptr = mmap(NULL, new_size, PROT_READ | PROT_WRITE,
                   MAP_SHARED, fd, 0);        // 4. 閲嶆柊鏄犲皠
        mapped_size = new_size;
        // 鈿狅笍 ptr 鍙兘鍙樹簡锛佷笉鑳戒緷璧栨棫鐨勬寚閽?    }
};
```

### Linux 鐨勯珮鏁堟浛浠ｆ柟妗堬細mremap

```c
#define _GNU_SOURCE
#include <sys/mman.h>

void* new_ptr = mremap(old_ptr, old_sz, new_sz, MREMAP_MAYMOVE);
```

mremap 鐨勪紭鍔匡細涓嶈Е鍙?TLB 鍒锋柊锛屼笉閲嶅缓 VMA锛屾瘮 munmap+mmap 蹇緢澶氥€?
---

## 4.6 宕╂簝瀹夊叏锛歐AL锛圵rite-Ahead Log锛?
mmap 鐨勭洿鎺ュ啓鍏ュ湪宕╂簝鏃舵槸涓棶棰橈細鍐欏叆鐨勬槸鍐呭瓨锛岃€屽唴瀛樹腑鐨勬暟鎹彲鑳借繕娌″埛鍒扮鐩樸€?
**Write-Ahead Log锛圵AL锛屽啓鍓嶆棩蹇楋級** 鏄暟鎹簱绯荤粺瑙ｅ喅杩欎釜闂鐨勬爣鍑嗘柟娉曪細

```
姝ｅ父鎿嶄綔娴佺▼锛?  1. 灏?鎴戣鍋氫粈涔?璁板綍鍒?WAL 鏂囦欢锛堣拷鍔犲啓鍏ワ紝鐢?fdatasync 纭繚鎸佷箙鎬э級
  2. 鍦ㄥ唴瀛橈紙mmap锛変腑鎵ц瀹為檯淇敼
  3. 瀹氭湡鍋?checkpoint锛氬皢 WAL 涓褰曠殑鎵€鏈変慨鏀瑰悓姝ュ埌涓绘暟鎹枃浠?
宕╂簝鎭㈠娴佺▼锛?  1. 閲嶆柊鎵撳紑鏁版嵁搴撴椂妫€鏌?WAL 鏂囦欢
  2. 浠庢渶鍚庝竴涓?checkpoint 浣嶇疆寮€濮嬶紝閲嶆斁 WAL 涓褰曠殑鎵€鏈夋搷浣?  3. 鎴柇 WAL锛屾暟鎹仮澶嶄竴鑷?```

---

## 4.7 鍔ㄦ墜瀹炵幇锛氬彲鎸佷箙鍖栫殑娴偣鏁扮粍

鍦?`ch04_mmap_storage/code/mmap_array.cpp` 瀹炵幇涓€涓畬鏁寸殑绀轰緥锛?
```cpp
#include <sys/mman.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <cstdint>
#include <iostream>
#include <vector>
#include <cassert>

struct Header {
    uint32_t magic;
    uint32_t version;
    uint64_t element_size;
    uint64_t count;
    uint64_t capacity;
    uint8_t reserved[40];

    static constexpr uint32_t MAGIC = 0x4C4D4442;
    static constexpr uint32_t VERSION = 1;
};

class MmapFloatArray {
    int fd;
    void* ptr;
    size_t file_size;
    Header* header;

    size_t data_offset() const {
        return (sizeof(Header) + 63) & ~63ULL;
    }

    void init_file(size_t capacity) {
        file_size = data_offset() + capacity * sizeof(float);
        ftruncate(fd, file_size);
        ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) { perror("mmap init"); exit(1); }
        header = reinterpret_cast<Header*>(ptr);
        header->magic = Header::MAGIC;
        header->version = Header::VERSION;
        header->element_size = sizeof(float);
        header->count = 0;
        header->capacity = capacity;
    }

    void load_existing() {
        struct stat st;
        fstat(fd, &st);
        file_size = st.st_size;
        ptr = mmap(NULL, file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) { perror("mmap load"); exit(1); }
        header = reinterpret_cast<Header*>(ptr);
        if (header->magic != Header::MAGIC) {
            std::cerr << "Error: Bad magic number!" << std::endl;
            exit(1);
        }
    }

public:
    MmapFloatArray(const char* path, size_t capacity = 1024) {
        fd = open(path, O_RDWR | O_CREAT, 0644);
        if (fd < 0) { perror("open"); exit(1); }
        struct stat st;
        fstat(fd, &st);
        if (st.st_size == 0) init_file(capacity);
        else load_existing();
    }

    ~MmapFloatArray() {
        msync(ptr, file_size, MS_SYNC);
        munmap(ptr, file_size);
        close(fd);
    }

    void push_back(float val) {
        if (header->count >= header->capacity)
            grow(header->capacity * 2);
        float* data = reinterpret_cast<float*>(
            reinterpret_cast<char*>(ptr) + data_offset());
        data[header->count++] = val;
    }

    float at(size_t i) const {
        float* data = reinterpret_cast<float*>(
            reinterpret_cast<char*>(ptr) + data_offset());
        return data[i];
    }

    size_t size() const { return header->count; }
    size_t capacity() const { return header->capacity; }

    void grow(size_t new_capacity) {
        size_t new_file_size = data_offset() + new_capacity * sizeof(float);
        msync(ptr, file_size, MS_SYNC);
        munmap(ptr, file_size);
        ftruncate(fd, new_file_size);
        ptr = mmap(NULL, new_file_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
        if (ptr == MAP_FAILED) { perror("mmap grow"); exit(1); }
        header = reinterpret_cast<Header*>(ptr);
        header->capacity = new_capacity;
        file_size = new_file_size;
    }
};

int main() {
    const char* path = "test_float_array.bin";

    {
        std::cout << "=== Round 1: Writing ===" << std::endl;
        MmapFloatArray arr(path, 8);
        for (int i = 0; i < 10; i++) arr.push_back(i * 1.5f);
        std::cout << "Size: " << arr.size() << "  Capacity: " << arr.capacity() << std::endl;
    }

    {
        std::cout << "\n=== Round 2: Re-reading ===" << std::endl;
        MmapFloatArray arr(path);
        assert(arr.size() == 10);
        assert(arr.capacity() == 16);
        for (size_t i = 0; i < arr.size(); i++) {
            assert(arr.at(i) == i * 1.5f);
            std::cout << "  [" << i << "] = " << arr.at(i) << std::endl;
        }
    }

    unlink(path);
    std::cout << "\nAll tests passed!" << std::endl;
    return 0;
}
```

缂栬瘧杩愯锛?```bash
g++-12 -O3 -std=c++17 mmap_array.cpp -o mmap_array
./mmap_array
```

---

## 鎬濊€冮

1. 涓轰粈涔?`close(fd)` 涓嶄細瑙﹀彂 msync锛?2. MAP_SHARED 鐨?mmap 涓庡悓涓€鏂囦欢涓婂叾浠栬繘绋嬬殑 `read()` 涔嬮棿濡備綍淇濊瘉涓€鑷存€э紵
3. 涓轰粈涔堢綉缁滄枃浠剁郴缁燂紙NFS锛変笂鐨?mmap 琛屼负澶嶆潅锛?4. 璁捐涓€绉嶆柟妗堬紝浣?mmap 鏁扮粍鏀寔澶氱嚎绋嬪苟鍙戣鍐欍€傞渶瑕佽€冭檻鍝簺绔炴€佹潯浠讹紵
5. Arrow/Feather 鏍煎紡涓轰粈涔堥€夋嫨鍒楀紡瀛樺偍锛熻繖瀵?mmap 鐨?page fault 妯″紡鏈変粈涔堝奖鍝嶏紵

---

## 鍔ㄦ墜缁冧範

1. 淇敼 `MmapFloatArray`锛屾坊鍔?閫昏緫鍒犻櫎"鎿嶄綔銆?2. 瀹炵幇涓€涓甫鍩虹 WAL 鐨?mmap 瀛樺偍銆傛祦绋嬶細鍐欏叆鍓嶅厛杩藉姞鍒?WAL 鈫?fdatasync WAL 鈫?淇敼 mmap銆?3. 瀵规瘮 mmap 鍜屼紶缁?`pread`/`pwrite` 鍦ㄥぇ鏂囦欢闅忔満璇诲満鏅笅鐨勬€ц兘宸紓銆?4. 娴嬭瘯浣跨敤 2MB 澶ч〉锛圚uge Pages, `MAP_HUGETLB`锛夊鍚戦噺鎼滅储鎬ц兘鐨勫奖鍝嶃€?5. 鐢?mmap 璇诲彇涓€涓?Arrow Feather 鏂囦欢锛坄.arrow`锛夛紝楠岃瘉鍏堕浂鎷疯礉鐗规€с€?