"""
检索策略定义 / Retrieval Strategy Definitions.

定义 AgenticDB 支持的四种搜索策略和计划数据结构。

策略对比 / Strategy Comparison:
  Strategy      | 使用场景               | 轮数 | 查询数
  DIRECT        | 简单事实性问题         | 1    | 1
  FILTERED      | 带条件约束的查询       | 1    | 1 + filter
  MULTI_QUERY   | 多角度复杂问题         | 1-2  | 2-5
  HIERARCHICAL  | 开放性探索问题         | 2-5  | 每轮1-2

设计原则 / Design Principles:
  - dataclass 确保类型安全和不可变性 / Type-safe immutable data structures
  - strategy 枚举保证策略名称一致性 / Enum for strategy names consistency
  - RetrievalPlan 可以被序列化为 JSON (用于日志/监控)
"""

import json
from dataclasses import dataclass, field
from enum import Enum
from typing import Any, Dict, List, Optional


class SearchStrategy(Enum):
    """
    检索策略枚举 / Supported retrieval strategies.

    每种策略对应不同的查询行为:
    - DIRECT: 单轮语义搜索, 最快但最简单
    - FILTERED: 基于元数据的过滤搜索
    - MULTI_QUERY: 多查询并行 + 结果融合
    - HIERARCHICAL: 渐进式搜索, 逐步精化
    """
    DIRECT = "direct"           # 直接搜索 / One-shot semantic search
    FILTERED = "filtered"       # 带过滤 / Search with metadata constraints
    MULTI_QUERY = "multi_query" # 多查询 / Multiple sub-queries
    HIERARCHICAL = "hierarchical"  # 层次化 / Progressive refinement


@dataclass
class SearchStep:
    """
    单次搜索操作 / A single search operation.

    Attributes:
        query: 搜索查询文本
        filter: 可选元数据过滤条件 (FilterNode JSON 格式)
        k: 返回结果数量
        collection: 集合名称
        round_num: 所属轮次 (由引擎自动设置)
    """
    query: str
    filter: Optional[Dict[str, Any]] = None
    k: int = 10
    collection: str = "default"
    round_num: int = 0


@dataclass
class RetrievalPlan:
    """
    检索计划 / A complete retrieval plan.

    由 QueryPlanner 生成, 包含策略选择、步骤列表和预期轮数。
    可以被引擎执行, 也可以序列化为 JSON 用于日志。

    Attributes:
        strategy: 选中的搜索策略
        reasoning: LLM 选择该策略的原因 (用于调试和面试展示)
        steps: 要执行的具体搜索步骤列表
        expected_rounds: 预期执行轮数
        original_question: 原始用户问题 (用于回溯)
    """
    strategy: SearchStrategy
    reasoning: str
    steps: List[SearchStep] = field(default_factory=list)
    expected_rounds: int = 1
    original_question: str = ""

    def summary(self) -> str:
        """
        生成人类可读的计划摘要 / Generate human-readable plan summary.

        用于日志记录和调试输出。

        Returns:
            多行字符串, 包含策略、原因和步骤详情
        """
        parts = [f"Strategy: {self.strategy.value}"]
        parts.append(f"Reasoning: {self.reasoning}")
        parts.append(f"Steps: {len(self.steps)}, Expected rounds: {self.expected_rounds}")
        for i, step in enumerate(self.steps):
            f_str = json.dumps(step.filter) if step.filter else "none"
            parts.append(f"  Step {i+1}: query='{step.query}' filter={f_str} k={step.k}")
        return "\n".join(parts)
