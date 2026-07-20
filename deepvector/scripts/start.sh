#!/bin/bash
# Start script for AgenticDB + DeepVector
# This script starts both the DeepVector C++ server and the Python agent server

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "============================================"
echo "  AgenticDB - Agent-Native Vector Database"
echo "============================================"

# Default settings
DEEPVECTOR_PORT=${DEEPVECTOR_PORT:-8080}
AGENT_PORT=${AGENT_PORT:-8090}
DATA_DIR=${DATA_DIR:-"$PROJECT_DIR/data"}
LLM_PROVIDER=${LLM_PROVIDER:-"ollama"}
DIM=${DIM:-768}
API_KEY=${API_KEY:-""}

# Check if DeepVector binary exists
DEEPVECTOR_BIN="$PROJECT_DIR/build/server/lumendb_server"
if [ ! -f "$DEEPVECTOR_BIN" ]; then
    echo "[WARN] DeepVector binary not found. Building..."
    cd "$PROJECT_DIR"
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-12
    cmake --build build --target lumendb_server
fi

# Start DeepVector server
echo "[INFO] Starting DeepVector server on port $DEEPVECTOR_PORT..."
"$DEEPVECTOR_BIN" \
    --port "$DEEPVECTOR_PORT" \
    --dim "$DIM" \
    --data-dir "$DATA_DIR" \
    --api-key "$API_KEY" &
DEEPVECTOR_PID=$!
echo "[INFO] DeepVector PID: $DEEPVECTOR_PID"

# Wait for DeepVector to start
sleep 2

# Check if DeepVector is up
if ! kill -0 $DEEPVECTOR_PID 2>/dev/null; then
    echo "[ERROR] DeepVector failed to start"
    exit 1
fi

# Start Python agent server
echo "[INFO] Starting AgenticDB agent on port $AGENT_PORT..."
cd "$PROJECT_DIR"
export LLM_PROVIDER="$LLM_PROVIDER"
export AGENTICDB_DEEPVECTOR_URL="http://localhost:$DEEPVECTOR_PORT"
export AGENTICDB_AGENT_PORT="$AGENT_PORT"

python -m agent.server.app &
AGENT_PID=$!
echo "[INFO] Agent PID: $AGENT_PID"

# Wait for agent to start
sleep 2

echo ""
echo "============================================"
echo "  AgenticDB is running!"
echo ""
echo "  DeepVector API:  http://localhost:$DEEPVECTOR_PORT"
echo "  Agent API:    http://localhost:$AGENT_PORT"
echo "  Data Dir:     $DATA_DIR"
echo "  LLM Provider: $LLM_PROVIDER"
echo ""
echo "  Quick test:"
echo "    curl http://localhost:$DEEPVECTOR_PORT/health"
echo ""
echo "  Stop with: kill $DEEPVECTOR_PID $AGENT_PID"
echo "============================================"

# Trap SIGINT/SIGTERM to clean up
trap "echo ''; echo 'Shutting down...'; kill $DEEPVECTOR_PID $AGENT_PID 2>/dev/null; exit 0" SIGINT SIGTERM

# Wait for both processes
wait $DEEPVECTOR_PID
