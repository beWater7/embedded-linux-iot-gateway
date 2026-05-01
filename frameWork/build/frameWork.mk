ifndef PREFIX
PREFIX  = arm-linux-gnueabihf-#工具链
endif

CC 	    = $(PREFIX)gcc
STRIP   = $(PREFIX)strip
AR      = $(PREFIX)ar
CFLAGS  += -g -Wall -std=gnu99#-Werror

# 使用find命令获取所有.c文件
SRCS := $(shell find ../ -type f -name '*.c')

# 将所有.c文件替换为.o文件
OBJS := $(patsubst %.c, %.o, $(SRCS))	
CPP_SRC = 

ifeq (0,1)
INCLUDE_PATH = -I ../buffer       \
			   -I ../list         \
			   -I ../pthreadPool  \
			   -I ../timer        \
			   -I ../tree
endif
INCLUDE_PATH = -I ../../h/frameWork

LIB_PATH = ../../module  #库路径
LIB     = libFrameWork.a

all: $(LIB)

# .c文件的编译规则
%.o:%.c
	$(CC) $(CFLAGS) $(INCLUDE_PATH) -c $< -o $@ -lpthread

# 生成库文件
$(LIB):$(OBJS)
	rm -f $@
	$(AR) cr $@ $(OBJS) 
	rm -f $(OBJS)
	cp $(LIB) $(LIB_PATH)



# 删除中间文件
clean:
	#rm -f $(OBJS) $(TARGET) 

ifeq (0,1)
注释块
endif
