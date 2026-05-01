#########################################################################
# File Name: mvHfile.sh
# Author: liudayi
# mail: 1781405401@qq.com
# Created Time: 2024年01月25日 星期四 23时27分06秒
#########################################################################
#!/bin/bash
#!/bin/bash

# 源目录
source_directory="../"

# 目标目录
target_directory="../../h/frameWork"


# 判断目标目录是否存在，如果不存在则创建
#if [ ! -d "$target_directory" ]; then
#    mkdir -p "$target_directory"
#fi

# 使用find命令查找所有.h文件，并复制到目标目录
find "$source_directory" -type f -name "*.h" -exec cp {} "$target_directory" \;

