@echo off

echo Formatting C++ code with clang-format...

for /r "engine\include" %%f in (*.hpp *.cpp) do (
    clang-format -i "%%f"
)

for /r "engine\src" %%f in (*.hpp *.cpp) do (
    clang-format -i "%%f"
)

for /r "games" %%f in (*.hpp *.cpp) do (
    clang-format -i "%%f"
)

echo Code formatting complete!
