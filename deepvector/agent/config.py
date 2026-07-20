"""
AgenticDB 鍏ㄥ眬閰嶇疆绠＄悊 / Global Configuration Management.

璁捐鍘熷垯 / Design Principles:
  - 鎵€鏈夐厤缃彲閫氳繃鐜鍙橀噺瑕嗙洊 / All settings overridable via env vars
  - 鍑哄巶榛樿鍊煎彲鐩存帴杩愯 Ollama 鏈湴妯″紡 / Sensible defaults for local Ollama
  - 閫氳繃 dataclass 瀹炵幇绫诲瀷瀹夊叏鍜?IDE 鑷姩琛ュ叏 / Type-safe with dataclass

鐜鍙橀噺鍓嶇紑 / Env Var Prefix: AGENTICDB_
"""

import os
from dataclasses import dataclass, field
from typing import Optional


@dataclass
class LLMConfig:
    """
    澶ц瑷€妯″瀷閰嶇疆 / Large Language Model Configuration.

    Attributes:
        provider: 妯″瀷鎻愪緵鍟? "openai" 鎴?"ollama" / LLM provider
        model: 妯″瀷鍚嶇О, Ollama 榛樿 qwen2.5:7b, OpenAI 榛樿 gpt-4o
        openai_api_key: OpenAI API 瀵嗛挜 (浠庣幆澧冨彉閲?OPENAI_API_KEY 璇诲彇)
        openai_base_url: OpenAI API 鑷畾涔夌鐐?(鐢ㄤ簬浠ｇ悊鎴栧吋瀹?API)
        ollama_host: Ollama 鏈嶅姟鍦板潃, 榛樿 http://localhost:11434
        temperature: 鐢熸垚娓╁害, 鍊艰秺浣庤秺纭畾 / Lower = more deterministic
        max_tokens: 鏈€澶х敓鎴?token 鏁?/ Max output tokens
        timeout: HTTP 璇锋眰瓒呮椂绉掓暟 / Request timeout in seconds
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
        """鍒濆鍖栧悗浠庣幆澧冨彉閲忚鐩栭粯璁ゅ€?/ Override defaults from env vars."""
        self._override_from_env("AGENTICDB_LLM_PROVIDER", "provider")
        self._override_from_env("AGENTICDB_LLM_MODEL", "model")
        self._override_from_env("OPENAI_API_KEY", "openai_api_key")
        self._override_from_env("OPENAI_BASE_URL", "openai_base_url")
        self._override_from_env("AGENTICDB_OLLAMA_HOST", "ollama_host")

    def _override_from_env(self, env_var: str, attr: str):
        """浠庣幆澧冨彉閲忚鍙栧苟瑕嗙洊灞炴€?/ Override attribute from env var if set."""
        val = os.getenv(env_var)
        if val is not None:
            # 澶勭悊绫诲瀷杞崲 / Handle type conversion
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
    宓屽叆鍚戦噺閰嶇疆 / Embedding Configuration.

    鏀寔鏈湴 sentence-transformers 鍜?OpenAI 涓ょ妯″紡銆?    鏈湴榛樿 all-MiniLM-L6-v2 (384缁?, 杞婚噺涓旀€ц兘鑹ソ銆?
    Attributes:
        provider: "local" 鎴?"openai"
        model: 鏈湴妯″瀷鍚嶇О / Local model name
        openai_model: OpenAI 宓屽叆妯″瀷 / OpenAI embedding model
        dimension: 鏈湴妯″瀷杈撳嚭缁村害 (384) / Output dimension
        openai_dimension: OpenAI 妯″瀷杈撳嚭缁村害 (1536)
        batch_size: 鎵归噺缂栫爜澶у皬 / Batch size for encoding
        cache_enabled: 鏄惁鍚敤宓屽叆缂撳瓨 / Enable embedding result cache
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
        """鍒濆鍖栧悗浠庣幆澧冨彉閲忚鐩?/ Override defaults from env vars."""
        self.provider = os.getenv("AGENTICDB_EMBEDDING_PROVIDER", self.provider)
        self.model = os.getenv("AGENTICDB_EMBEDDING_MODEL", self.model)
        self.openai_api_key = os.getenv("OPENAI_API_KEY", self.openai_api_key)


@dataclass
class AgenticDBConfig:
    """
    AgenticDB 椤跺眰閰嶇疆 / Top-level Configuration.

    鍖呭惈 LLM銆丒mbedding銆佹湇鍔″湴鍧€銆佹绱㈠弬鏁扮瓑鎵€鏈夐厤缃€?    鍙€氳繃鐜鍙橀噺缁熶竴绠＄悊锛岄€傚悎 Docker/K8s 閮ㄧ讲銆?
    Attributes:
        llm: LLM 瀛愰厤缃?        embedding: Embedding 瀛愰厤缃?        lumendb_url: DeepVector C++ Server 鍦板潃
        agent_port: Agent HTTP Server 绔彛
        agent_host: Agent HTTP Server 鐩戝惉鍦板潃
        max_rounds: 鏈€澶ф绱㈣疆鏁?/ Max retrieval rounds
        quality_threshold: 璐ㄩ噺闃堝€?(0-1), 杈惧埌鍗冲仠姝?        max_results_per_round: 姣忚疆鏈€澶氳繑鍥炵粨鏋滄暟
        top_k_final: 鏈€缁堣繑鍥炵粰鐢ㄦ埛鐨勬渶澶氱粨鏋滄暟
        default_collection: 榛樿闆嗗悎鍚嶇О
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
        """鍒濆鍖栧悗浠庣幆澧冨彉閲忚鐩?/ Override defaults from env vars."""
        self.lumendb_url = os.getenv("AGENTICDB_DEEPVECTOR_URL", self.lumendb_url)
        self.agent_port = int(os.getenv("AGENTICDB_AGENT_PORT", str(self.agent_port)))
        self.default_collection = os.getenv(
            "AGENTICDB_DEFAULT_COLLECTION", self.default_collection
        )


def load_config() -> AgenticDBConfig:
    """
    鍔犺浇閰嶇疆 (宸ュ巶鍑芥暟) / Load configuration with sensible defaults.

    鐢ㄦ硶 / Usage:
        config = load_config()
        config.llm.provider  # "ollama" | "openai"

    Returns:
        宸叉敞鍏ョ幆澧冨彉閲忚鐩栧€肩殑 AgenticDBConfig 瀹炰緥
    """
    return AgenticDBConfig()
