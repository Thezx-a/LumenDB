#!/bin/bash
find src include tests benches gateway examples -name '*.cpp' -o -name '*.h' | xargs clang-format -i
echo "Done"
