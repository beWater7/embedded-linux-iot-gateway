#########################################################################
# File Name: backup.sh
# Author: liudayi
# mail: 1781405401@qq.com
# Created Time: 2023年11月25日 星期六 22时51分30秒
#########################################################################
#!/bin/bash
# 需要使用cron,使用which cron查看安装路径
#crontab -e 选择第一个编辑器、ctrl+s保存、ctrl+x退出
# 目标共享路径
shared_path="/mnt/hgfs/5练手/gateWay"

# 源目录，可以根据需要添加多个
source_dirs=(
    "../"
    # 添加更多源目录
)

# 使用 rsync 将源目录同步到共享路径
rsync -av "${source_dirs[@]}" "$shared_path"

echo "Sync completed successfully!"
