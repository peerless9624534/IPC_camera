CONFIG_PTZ=y

CROSS_COMPILE = mips-linux-gnu-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
CPLUSPLUS = $(CROSS_COMPILE)g++
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump

BULD_ROOT := .

SRC_FILE :=$(wildcard $(BULD_ROOT)/common/src/*.c) \
	$(wildcard $(BULD_ROOT)/modules/ingenic/imp/src/*.c) \
	$(wildcard $(BULD_ROOT)/modules/ingenic/sysutils/src/*.c) \
	$(wildcard $(BULD_ROOT)/modules/media/src/*.c) \
	$(wildcard $(BULD_ROOT)/modules/tuya/src/*.c) \
	$(wildcard $(BULD_ROOT)/modules/thrdlibs/zlib/src/*.c) \
	$(wildcard $(BULD_ROOT)/modules/*.c) \
	$(wildcard $(BULD_ROOT)/hal/wifi/src/*.c) \
	$(wildcard $(BULD_ROOT)/hal/led/src/*.c) \
	$(wildcard $(BULD_ROOT)/hal/misc/src/*.c) 



INC_PATH := -I$(BULD_ROOT)/common/include/ \
		    -I$(BULD_ROOT)/modules/ingenic/imp/include/ \
			-I$(BULD_ROOT)/sample/ingenic/include/ \
		    -I$(BULD_ROOT)/modules/ingenic/sysutils/include/ \
		    -I$(BULD_ROOT)/modules/media/include/ \
			-I$(BULD_ROOT)/modules/thrdlibs/zlib/include \
			-I$(BULD_ROOT)/modules/thrdlibs/zbar/include \
		    -I$(BULD_ROOT)/modules/tuya/include/ \
			-I$(BULD_ROOT)/modules/thrdlibs/iniparse/include/ \
			-I$(BULD_ROOT)/hal/wifi/include/ \
			-I$(BULD_ROOT)/hal/led/include/ \
			-I$(BULD_ROOT)/hal/misc/include/ 		
ifeq ($(CONFIG_PTZ), y)
INC_PATH += 
endif

LDFLAGS := -lpthread -lm -lrt -ldl
LDFLAGS += -Wl,-gc-section

# 编译器在编译时的参数设置 -Wall显示所有警告， -O2优化等级 -g调试信息
CFLAGS := -Wall -O2 -march=mips32r2
#ifeq ($(CONFIG_PTZ), y)
CFLAGS += -DCONFIG_PTZ_IPC
#endif

#debug调试开关
CFLAGS += -DDEBUG

#静态库链接
LIBS = 	$(BULD_ROOT)/libs/ingenic/libimp.a \
		$(BULD_ROOT)/libs/ingenic/libalog.a \
		$(BULD_ROOT)/libs/ingenic/libsysutils.a \
		$(BULD_ROOT)/libs/tuya/libtuya_ipc.a \
		$(BULD_ROOT)/libs/tuya/libmbedtls.a \
		$(BULD_ROOT)/libs/third/libzbar.a \
		$(BULD_ROOT)/libs/third/libiniparser.a 
ifeq ($(CONFIG_PTZ), y)
LIBS += 
endif

OBJS = $(SRC_FILE:%.c=%.o)

TARGET := $(BULD_ROOT)/out/media_app

all: $(TARGET) 

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(LDFLAGS) $^ -o $@ $(LIBS)
	$(STRIP) $(TARGET)
	@echo "******** build $(TARGET) success! ********"

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) $(INC_PATH) $< -o $@ 

clean:
	@rm -f $(shell find -name "*.o")
	@rm -f $(TARGET)
	@echo "******** clean success! ********"

.PHONY:all clean 
