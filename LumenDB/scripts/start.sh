#!/bin/bash
# Start script for AgenticDB + LumenDB
# This script starts both the LumenDB C++ server and the Python agent server

set -e

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "$SCRIPT_DIR")"

echo "============================================"
echo "  AgenticDB - Agent-Native Vector Database"
echo "============================================"

# Default settings
LUMENDB_PORT=${LUMENDB_PORT:-8080}
AGENT_PORT=${AGENT_PORT:-8090}
DATA_DIR=${DATA_DIR:-"$PROJECT_DIR/data"}
LLM_PROVIDER=${LLM_PROVIDER:-"ollama"}
DIM=${DIM:-768}
API_KEY=${API_KEY:-""}

# Check if LumenDB binary exists
LUMENDB_BIN="$PROJECT_DIR/build/server/lumendb_server"
if [ ! -f "$LUMENDB_BIN" ]; then
    echo "[WARN] LumenDB binary not found. Building..."
    cd "$PROJECT_DIR"
    cmake -B build -G Ninja -DCMAKE_BUILD_TYPE=Release -DCMAKE_CXX_COMPILER=g++-12
    cmake --build build --target lumendb_server
fi

# Start LumenDB server
echo "[INFO] Starting LumenDB server on port $LUMENDB_PORT..."
"$LUMENDB_BIN" \
    --port "$LUMENDB_PORT" \
    --dim "$DIM" \
    --data-dir "$DATA_DIR" \
    --api-key "$API_KEY" &
LUMENDB_PID=$!
echo "[INFO] LumenDB PID: $LUMENDB_PID"

# Wait for LumenDB to start
sleep 2

# Check if LumenDB is up
if ! kill -0 $LUMENDB_PID 2>/dev/null; then
    echo "[ERROR] LumenDB failed to start"
    exit 1
fi

# Start Python agent server
echo "[INFO] Starting AgenticDB agent on port $AGENT_PORT..."
cd "$PROJECT_DIR"
export LLM_PROVIDER="$LLM_PROVIDER"
export AGENTICDB_LUMENDB_URL="http://localhost:$LUMENDB_PORT"
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
echo "  LumenDB API:  http://localhost:$LUMENDB_PORT"
echo "  Agent API:    http://localhost:$AGENT_PORT"
echo "  Data Dir:     $DATA_DIR"
echo "  LLM Provider: $LLM_PROVIDER"
echo ""
echo "  Quick test:"
echo "    curl http://localhost:$LUMENDB_PORT/health"
echo ""
echo "  Stop with: kill $LUMENDB_PID $AGENT_PID"
echo "============================================"

# Trap SIGINT/SIGTERM to clean up
trap "echo ''; echo 'Shutting down...'; kill $LUMENDB_PID $AGENT_PID 2>/dev/null; exit 0" SIGINT SIGTERM

# Wait for both processes
wait $LUMENDB_PID
