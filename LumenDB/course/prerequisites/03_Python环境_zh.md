# Python 环境配置

> 本教程使用 Python 进行绑定和脚本。本文档覆盖 Python 环境管理。

## 虚拟环境

### 为什么需要虚拟环境？

不同项目可能依赖不同版本的库。虚拟环境为每个项目创建独立的 Python 环境，避免版本冲突。

```bash
# 创建虚拟环境
python3 -m venv .venv

# 激活（Linux/Mac）
source .venv/bin/activate

# 激活（Windows）
.venv\Scripts\activate

# 退出
deactivate
```

## pip 常用命令

```bash
# 安装包
pip install numpy

# 从 requirements.txt 安装
pip install -r requirements.txt

# 导出当前环境
pip freeze > requirements.txt

# 升级包
pip install --upgrade numpy

# 查看已安装
pip list
```

## 本教程常用 Python 包

| 包 | 用途 | 安装 |
|----|------|------|
| numpy | 数值计算 | `pip install numpy` |
| pybind11 | C++ 绑定 | `pip install pybind11` |
| langchain | RAG 框架 | `pip install langchain` |
| requests | HTTP 客户端 | `pip install requests` |

## 构建 Python 绑定

```bash
# 方式 1：pip 安装（推荐）
cd python/
pip install -e .

# 方式 2：CMake 构建
cmake -B build -DENABLE_PYTHON=ON -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
```

## 相关章节

- Ch09 Python 绑定：[09_Python绑定](../ch09_python_bindings/09_Python绑定_zh.md)
