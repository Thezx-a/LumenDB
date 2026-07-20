# Chapter 3 (Track A): HNSW — Wiring Distance into a Graph Line

> HNSW is the production ANN workhorse (MongoDB Atlas, Milvus, Qdrant, pgvector, …).  
> This chapter turns the distance **point** into a navigable graph **line**.

## Prerequisites

> 📎 [Vectors & Distance](../ch02_vectors_distance/02_向量与距离度量_en.md) · [INTERVIEW_BANK §B](../INTERVIEW_BANK.md)

## Objectives

- [ ] Explain sparse upper layers vs dense layer 0  
- [ ] Write the level-assignment formula and role of `M`  
- [ ] Describe search: greedy → descend → ef beam  
- [ ] Locate `insert` / `searchLayer` in source  

---

## Surface Context

`Collection.search` → `HNSWIndex` → distance kernels + `VectorStore.get`.

---

## 1. Point

Level: \(\lfloor -\ln U / \ln M \rfloor\).  
Candidates often live in a `priority_queue` of `(distance, id)`.

**Code:** `include/dv/index/hnsw.h`, `src/index/hnsw.cpp`.

---

## 2. Line

`add` appends vector then `index.insert(id)`.  
`searchWithFilter` post-filters HNSW candidates via metadata AST.

---

## 3. Surface

Tune `M`, `ef_construction`, `ef_search` for recall/latency/memory.

---

## 4. Hands-on

Annotate `searchLayer`.  
Optional: plot recall vs ef_search.

---

## 5. Reflection

Why `2*M` on layer 0? Soft-delete pitfalls?

---

## 6. Interview

Drill **Q-B1/B2/B3** in `INTERVIEW_BANK.md`. Whiteboard a 3-layer query.

---

## 7. References

1. Malkov & Yashunin HNSW paper  
2. MongoDB HNSW overview  
3. Faiss HNSW notes  
4. Repo tests: `tests/test_hnsw.cpp`  
5. `ARCHITECTURE.md`

**Next:** [mmap storage](../ch04_mmap_storage/)

---

## Appendix: Interview Bank Mapping

After this chapter, drill the matching section in [INTERVIEW_BANK.md](../INTERVIEW_BANK.md) and self-check against [_CHAPTER_TEMPLATE.md](../_CHAPTER_TEMPLATE.md).

**Architecture:** [ARCHITECTURE.md](../../ARCHITECTURE.md) · **Tech:** [TECH.md](../../../TECH.md) · **Run:** [RUN.md](../../../RUN.md)
