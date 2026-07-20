"""
AgenticDB 全局配置管理 / Global Configuration Management.

设计原则 / Design Principles:
  - 所有配置可通过环境变量覆盖 / All settings overridable via env vars
  - 出厂默认值可直接运行 Ollama 本地模式 / Sensible defaults for local Ollama
  - 通过 dataclass 实现类型安全和 IDE 自动补全 / Type-safe with dataclass

环境变量前缀 / Env Var Prefix: AGENTICDB_
"""

import os
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class LLMConfig:
    """
    大语言模型配置 / Large Language Model Configuration.

    Attributes:
        provider: 模型提供商, "openai" 或 "ollama" / LLM provider
        model: 模型名称, Ollama 默认 qwen2.5:7b, OpenAI 默认 gpt-4o
        openai_api_key: OpenAI API 密钥 (从环境变量 OPENAI_API_KEY 读取)
        openai_base_url: OpenAI API 自定义端点 (用于代理或兼容 API)
        ollama_host: Ollama 服务地址, 默认 http://localhost:11434
        temperature: 生成温度, 值越低越确定 / Lower = more deterministic
        max_tokens: 最大生成 token 数 / Max output tokens
        timeout: HTTP 请求超时秒数 / Request timeout in seconds
    """
    provider: str = "ollama"  # "openai" | "ollama"
    model: str = "qwen2.5:7b"
    openai_api_key: str = ""
    openai_base_url: Optional[str] = None
    ollama_host: str = "http://localhost:11434"
    temperature: float = 0.1
    max_tokens: int = 2048
    timeout: float = 60.0

    def __post_init__(self):
        """初始化后从环境变量覆盖默认值 / Override defaults from env vars."""
        self._override_from_env("AGENTICDB_LLM_PROVIDER", "provider")
        self._override_from_env("AGENTICDB_LLM_MODEL", "model")
        self._override_from_env("OPENAI_API_KEY", "openai_api_key")
        self._override_from_env("OPENAI_BASE_URL", "openai_base_url")
        self._override_from_env("AGENTICDB_OLLAMA_HOST", "ollama_host")

    def _override_from_env(self, env_var: str, attr: str):
        """从环境变量读取并覆盖属性 / Override attribute from env var if set."""
        val = os.getenv(env_var)
        if val is not None:
            # 处理类型转换 / Handle type conversion
            current = getattr(self, attr)
            if isinstance(current, bool):
                setattr(self, attr, val.lower() in ("1", "true", "yes"))
            elif isinstance(current, int):
                setattr(self, attr, int(val))
            elif isinstance(current, float):
                setattr(self, attr, float(val))
            else:
                setattr(self, attr, val)


@dataclass
class EmbeddingConfig:
    """
    嵌入向量配置 / Embedding Configuration.

    支持本地 sentence-transformers 和 OpenAI 两种模式。
    本地默认 all-MiniLM-L6-v2 (384维), 轻量且性能良好。

    Attributes:
        provider: "local" 或 "openai"
        model: 本地模型名称 / Local model name
        openai_model: OpenAI 嵌入模型 / OpenAI embedding model
        dimension: 本地模型输出维度 (384) / Output dimension
        openai_dimension: OpenAI 模型输出维度 (1536)
        batch_size: 批量编码大小 / Batch size for encoding
        cache_enabled: 是否启用嵌入缓存 / Enable embedding result cache
    """
    provider: str = "local"  # "local" | "openai"
    model: str = "all-MiniLM-L6-v2"
    openai_model: str = "text-embedding-3-small"
    openai_api_key: str = ""
    dimension: int = 384
    openai_dimension: int = 1536
    batch_size: int = 64
    cache_enabled: bool = True

    def __post_init__(self):
        """初始化后从环境变量覆盖 / Override defaults from env vars."""
        self.provider = os.getenv("AGENTICDB_EMBEDDING_PROVIDER", self.provider)
        self.model = os.getenv("AGENTICDB_EMBEDDING_MODEL", self.model)
        self.openai_api_key = os.getenv("OPENAI_API_KEY", self.openai_api_key)


@dataclass
class AgenticDBConfig:
    """
    AgenticDB 顶层配置 / Top-level Configuration.

    包含 LLM、Embedding、服务地址、检索参数等所有配置。
    可通过环境变量统一管理，适合 Docker/K8s 部署。

    Attributes:
        llm: LLM 子配置
        embedding: Embedding 子配置
        lumendb_url: LumenDB C++ Server 地址
        agent_port: Agent HTTP Server 端口
        agent_host: Agent HTTP Server 监听地址
        max_rounds: 最大检索轮数 / Max retrieval rounds
        quality_threshold: 质量阈值 (0-1), 达到即停止
        max_results_per_round: 每轮最多返回结果数
        top_k_final: 最终返回给用户的最多结果数
        default_collection: 默认集合名称
    """
    llm: LLMConfig = field(default_factory=LLMConfig)
    embedding: EmbeddingConfig = field(default_factory=EmbeddingConfig)

    lumendb_url: str = "http://localhost:8080"
    agent_port: int = 8090
    agent_host: str = "0.0.0.0"

    max_rounds: int = 5
    quality_threshold: float = 0.7
    max_results_per_round: int = 20
    top_k_final: int = 10

    default_collection: str = "default"

    def __post_init__(self):
        """初始化后从环境变量覆盖 / Override defaults from env vars."""
        self.lumendb_url = os.getenv("AGENTICDB_LUMENDB_URL", self.lumendb_url)
        self.agent_port = int(os.getenv("AGENTICDB_AGENT_PORT", str(self.agent_port)))
        self.default_collection = os.getenv(
            "AGENTICDB_DEFAULT_COLLECTION", self.default_collection
        )


def load_config() -> AgenticDBConfig:
    """
    加载配置 (工厂函数) / Load configuration with sensible defaults.

    用法 / Usage:
        config = load_config()
        config.llm.provider  # "ollama" | "openai"

    Returns:
        已注入环境变量覆盖值的 AgenticDBConfig 实例
    """
    return AgenticDBConfig()
