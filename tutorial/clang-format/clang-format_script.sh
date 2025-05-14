##!/bin/bash
#
#CLANG_FORMAT="./clang_format/clang-format"
#TARGET_DIR="../../pr10_common"
#
#find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec "$CLANG_FORMAT" -i {} +

#CLANG_FORMAT="./clang_format/clang-format"
#TARGET_DIR="../../pr10_common"
#FORMATTED_FILES="formatted_files.txt"
#
## 清理之前的记录文件
#rm -f "$FORMATTED_FILES"
#
## 找出被格式化的文件
#find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec stat -c "%n %Y" {} \; > original_times.txt
#find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec "$CLANG_FORMAT" -i {} +
#find "$TARGET_DIR" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec stat -c "%n %Y" {} \; > new_times.txt
#
## 比较文件修改时间
#comm -13 <(sort original_times.txt) <(sort new_times.txt) | cut -d' ' -f1 > "$FORMATTED_FILES"
#
#echo "Formatted files:"
#cat "$FORMATTED_FILES"
#
## 统计被修改的文件数量
#FORMATTED_COUNT=$(wc -l < "$FORMATTED_FILES")
#echo "Number of formatted files: $FORMATTED_COUNT"
#rm -f original_times.txt
#rm -f new_times.txt

#!/bin/bash

# 获取脚本所在目录的绝对路径
SCRIPT_DIR=$(realpath "$(dirname "$0")")
echo $SCRIPT_DIR
# 假设项目根目录是脚本所在目录的父目录的父目录
PROJECT_ROOT=$(realpath "$SCRIPT_DIR/../..")
echo $PROJECT_ROOT
CLANG_FORMAT="$SCRIPT_DIR/clang_format/clang-format"  # 调整路径以匹配你的 clang-format 可执行文件位置
FORMATTED_FILES="$SCRIPT_DIR/formatted_files.txt"

# 清理之前的记录文件
rm -f "$FORMATTED_FILES"

# 定义格式化和统计函数
format_and_count() {
    local target_dir="$1"
    local original_times="$SCRIPT_DIR/original_times.txt"
    local new_times="$SCRIPT_DIR/new_times.txt"

    # 记录格式化前的文件修改时间
    find "$target_dir" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec stat -c "%n %Y" {} \; > "$original_times"

    # 格式化文件
    find "$target_dir" -type f \( -name "*.cpp" -o -name "*.h" -o -name "*.hpp" -o -name "*.cc" -o -name "*.c" \) -exec "$CLANG_FORMAT" -i {} +

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

# 主脚本逻辑
if [ $# -eq 0 ]; then
    echo "Usage: $0 <module1> [module2 ...]"
    exit 1
fi

for module in "$@"; do
    # 检查输入是否是绝对路径或基于home目录的路径
    if [[ "$module" == /* ]] || [[ "$module" == ~/* ]]; then
        # 如果是绝对路径或基于home目录的路径，直接使用
        full_path=$(realpath "$module")
    else
        # 否则，基于项目根目录拼接路径
        full_path=$(realpath "$PROJECT_ROOT/$module")
    fi

    if [ ! -d "$full_path" ]; then
        echo "Error: Directory $full_path does not exist."
        continue
    fi

    format_and_count "$full_path"
done
