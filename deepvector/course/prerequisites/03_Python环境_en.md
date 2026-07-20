# Python Environment Configuration

> This tutorial uses Python for bindings and scripts. This document covers Python environment management.

## Virtual Environments

### Why use virtual environments?

Different projects may depend on different library versions. Virtual environments create an isolated Python environment for each project, avoiding version conflicts.

```bash
# Create a virtual environment
python3 -m venv .venv

# Activate (Linux/Mac)
source .venv/bin/activate

# Activate (Windows)
.venv\Scripts\activate

# Deactivate
deactivate
```

## Common pip Commands

```bash
# Install a package
pip install numpy

# Install from requirements.txt
pip install -r requirements.txt

# Export current environment
pip freeze > requirements.txt

# Upgrade a package
pip install --upgrade numpy

# List installed packages
pip list
```

## Common Python Packages in This Tutorial

| Package | Purpose | Installation |
|---------|---------|-------------|
| numpy | Numerical computation | `pip install numpy` |
| pybind11 | C++ bindings | `pip install pybind11` |
| langchain | RAG framework | `pip install langchain` |
| requests | HTTP client | `pip install requests` |

## Building Python Bindings

```bash
# Method 1: pip install (recommended)
cd python/
pip install -e .

# Method 2: CMake build
cmake -B build -DENABLE_PYTHON=ON -DCMAKE_CXX_COMPILER=g++-12
cmake --build build
```

## Related Chapters

- Ch09 Python bindings: [09_Python绑定](../ch09_python_bindings/09_Python绑定_zh.md)
