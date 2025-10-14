#!/bin/bash

set -e

echo "Formatting C++ code with clang-format..."

find engine/include engine/src games -type f \( -name "*.hpp" -o -name "*.cpp" \) \
    -exec clang-format -i {} +

echo "Code formatting complete!"
