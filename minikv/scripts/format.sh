#!/bin/bash
find src include tests benches -name '*.cpp' -o -name '*.h' | xargs clang-format -i
echo "Done"
