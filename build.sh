#!/usr/bin/env bash
set -euo pipefail
cd -- "$(dirname -- "$0")"
mkdir -p build
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude src/main.cpp -o build/triewise
"${CXX:-g++}" -std=c++17 -O2 -Wall -Wextra -Wpedantic -Iinclude tests/test_engine.cpp -o build/triewise_tests
./build/triewise_tests
if command -v python3 >/dev/null 2>&1; then
  python3 tests/test_cli.py build/triewise data/dictionary.txt
fi
echo 'Build and tests passed. Run: ./build/triewise'
