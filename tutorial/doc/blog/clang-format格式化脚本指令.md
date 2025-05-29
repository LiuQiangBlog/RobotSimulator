
https://github.com/cross-platform/dspatchables/blob/master/scripts/clang-format.sh

```shell
#!/bin/bash

DIR=$(cd `dirname $0` && pwd)
cd $DIR/..

find Components -iname *.h -o -iname *.cpp | grep -v kissfft | grep -v Mongoose | grep -v RtAudio | xargs clang-format --style=file --verbose -i
```
可以在每个仓库下创建一个clang-format.sh脚本，用户通过执行这个格式化的脚本，就可以格式化当前的仓库代码。
```shell
#!/bin/bash

# 获取脚本所在目录的绝对路径
SCRIPT_DIR=$(realpath "$(dirname "$0")")
echo $SCRIPT_DIR
TARGET_DIR="../3rdparty/lz4"

#PROJECT_ROOT=$(realpath "$SCRIPT_DIR/../..")
#echo $PROJECT_ROOT

CLANG_FORMAT="$SCRIPT_DIR/clang_format/clang-format"  # 调整路径以匹配你的 clang-format 可执行文件位置
FORMATTED_FILES="$SCRIPT_DIR/formatted_files.txt"

# 定义格式化和统计函数
format_and_count() {
    local target_dir="$1"
    local original_times="$SCRIPT_DIR/original_times.txt"
    local new_times="$SCRIPT_DIR/new_times.txt"

    # 记录格式化前的文件修改时间
    find "$target_dir" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec stat -c "%n %Y" {} \; > "$original_times"

    # 格式化文件
#    find "$target_dir" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec "$CLANG_FORMAT" -i {} +
    find "$target_dir" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -print0 | xargs -0 "$CLANG_FORMAT" -i

    # 记录格式化后的文件修改时间
    find "$target_dir" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec stat -c "%n %Y" {} \; > "$new_times"

    # 比较文件修改时间，找出被格式化的文件
    comm -13 <(sort "$original_times") <(sort "$new_times") | cut -d' ' -f1 > "$FORMATTED_FILES"

    # 统计被修改的文件数量
    local formatted_count=$(wc -l < "$FORMATTED_FILES")

    echo "Formatted files in $target_dir:"
    cat "$FORMATTED_FILES"

    echo "Number of formatted files in $target_dir: $formatted_count"

    # 清理临时文件
    rm -f "$original_times" "$new_times"
}

format_and_count "$TARGET_DIR"

# 清理之前的记录文件
rm -f "$FORMATTED_FILES"
```