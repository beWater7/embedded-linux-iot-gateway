# ============================================================
#  webServer — 交叉编译 Makefile（imx6ull / ARM Cortex-A7）
#
#  默认用法：
#    make              # 使用 arm-linux-gnueabihf- 工具链
#    make CROSS=       # 在 x86 本机编译（方便开发调试）
#
#  接入 SQLite（阶段二）：
#    make USE_SQLITE=1
#    需要目标板已安装 libsqlite3，或静态链接 -Wl,-Bstatic -lsqlite3
# ============================================================

CROSS    ?= arm-linux-gnueabihf-
CXX       = $(CROSS)g++
STRIP     = $(CROSS)strip

CXXFLAGS  = -std=c++14 -O2 -Wall -Wextra -pthread
INCLUDES  = -Icode
TARGET    = webserver

SRCS = code/main.cpp                  \
       code/server/webserver.cpp      \
       code/server/epoller.cpp        \
       code/http/httpconn.cpp         \
       code/http/httprequest.cpp      \
       code/http/httpresponse.cpp     \
       code/buffer/buffer.cpp         \
       code/timer/heaptimer.cpp       \
       code/log/log.cpp               \
       code/auth/userstore.cpp

LDFLAGS   = -pthread

# ---------- SQLite 可选后端 ----------
ifdef USE_SQLITE
CXXFLAGS += -DUSE_SQLITE
LDFLAGS  += -lsqlite3
endif

# ---------- 构建规则 ----------
OBJS = $(SRCS:.cpp=.o)

.PHONY: all clean strip

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $@ $^ $(LDFLAGS)
	@echo "Build OK: $(TARGET)"

%.o: %.cpp
	$(CXX) $(CXXFLAGS) $(INCLUDES) -c $< -o $@

# 剥除调试符号，减小部署体积
strip: $(TARGET)
	$(STRIP) $(TARGET)

clean:
	rm -f $(OBJS) $(TARGET)
