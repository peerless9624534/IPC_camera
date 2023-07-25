CONFIG_PTZ?=n
CONFIG_RTSP?=n
CONFIG_BABY?=n
CONFIG_LUXALG?=n

CROSS_COMPILE = mips-linux-gnu-
AS		= $(CROSS_COMPILE)as
LD		= $(CROSS_COMPILE)ld
CC		= $(CROSS_COMPILE)gcc
CPP		= $(CC) -E
AR		= $(CROSS_COMPILE)ar
NM		= $(CROSS_COMPILE)nm
CPLUSPLUS = $(CROSS_COMPILE)g++ -std=c++11
STRIP		= $(CROSS_COMPILE)strip
OBJCOPY		= $(CROSS_COMPILE)objcopy
OBJDUMP		= $(CROSS_COMPILE)objdump


BULD_ROOT := ..

SRC_FILE := $(wildcard $(BULD_ROOT)/common/src/*.c) \
			$(wildcard $(BULD_ROOT)/modules/ingenic/imp/src/*.c) \
			$(wildcard $(BULD_ROOT)/modules/ingenic/sysutils/src/*.c) \
			$(wildcard $(BULD_ROOT)/modules/media/src/*.c) \
			$(wildcard $(BULD_ROOT)/modules/tuya/src/*.c) \
			$(wildcard $(BULD_ROOT)/modules/thrdlibs/zlib/src/*.c) \
			$(wildcard $(BULD_ROOT)/modules/thrdlibs/rtsp/*.c) \
			$(wildcard $(BULD_ROOT)/sample/*.c) \
			$(wildcard $(BULD_ROOT)/sample/ingenic/src/*.c) \
			$(wildcard $(BULD_ROOT)/sample/tuya/demo/src/*.c) \
			$(wildcard $(BULD_ROOT)/hal/wifi/src/*.c) \
			$(wildcard $(BULD_ROOT)/hal/led/src/*.c) \
			$(wildcard $(BULD_ROOT)/hal/misc/src/*.c) \
			$(wildcard $(BULD_ROOT)/hal/motor/src/*.c) \
			$(wildcard $(BULD_ROOT)/hal/i2c/src/*.c)

INC_PATH := -I$(BULD_ROOT)/common/include/ \
		    -I$(BULD_ROOT)/hal/include/ \
		    -I$(BULD_ROOT)/modules/ingenic/imp/include/ \
		    -I$(BULD_ROOT)/modules/ingenic/sysutils/include/ \
		    -I$(BULD_ROOT)/modules/media/include/ \
		    -I$(BULD_ROOT)/modules/tuya/include/ \
		    -I$(BULD_ROOT)/sample/ingenic/include/ \
		    -I$(BULD_ROOT)/sample/tuya/include/ \
	 	    -I$(BULD_ROOT)/modules/thrdlibs/zlib/include/ \
		    -I$(BULD_ROOT)/modules/thrdlibs/zbar/include/ \
			-I$(BULD_ROOT)/modules/thrdlibs/rtsp/ \
		    -I$(BULD_ROOT)/modules/thrdlibs/iniparse/include/ \
		    -I$(BULD_ROOT)/sample/tuya/demo/inc/ \
		    -I$(BULD_ROOT)/hal/wifi/include/ \
		    -I$(BULD_ROOT)/hal/led/include/ \
		    -I$(BULD_ROOT)/hal/misc/include/ \
		    -I$(BULD_ROOT)/modules/ingenic/alg/include/ \
		    -I$(BULD_ROOT)/modules/ingenic/ivs/include/ \
		    -I$(BULD_ROOT)/hal/motor/include/ \
			-I$(BULD_ROOT)/hal/i2c/include/


CPP_FILE := $(wildcard $(BULD_ROOT)/modules/media/src/cpp/*.c)

CPP_INC_PATH := -I$(BULD_ROOT)/modules/media/include/cpp \
				-I$(BULD_ROOT)/modules/media/include/cpp/4.7.2

ifeq ($(CONFIG_PTZ), y)
INC_PATH += 
endif

LDFLAGS := -pthread -lm -lrt -lstdc++ -fPIC
LDFLAGS += -Wall -Wl,--gc-section

# 编译器在编译时的参数设置 -Wall显示所有警告， -O2优化等级 -g调试信息
CFLAGS := -Wall -O2 -march=mips32r2
ifeq ($(CONFIG_PTZ), y)
CFLAGS += -DCONFIG_PTZ_IPC
endif
ifeq ($(CONFIG_RTSP), y)
CFLAGS += -DRTSP_ENABLE
endif
ifeq ($(CONFIG_BABY), y)
CFLAGS += -DCONFIG_BABY
endif
ifeq ($(CONFIG_LUXALG), y)
CFLAGS += -DCONFIG_LUXALG
endif

#debug调试开关
CFLAGS += -DDEBUG -g 

#静态库链接, 注意库的先后顺序, 有些库有相互依赖关系 # 
LIBS = -L$(BULD_ROOT)/libs/ingenic/alg  -lcryDet_inf -ljzdl -liaac -ldl -lsmileDet_inf -lpersonDet_inf
	
ifeq ($(CONFIG_PTZ), y)
LIBS += -lperson_tracker_inf
endif

ifeq ($(CONFIG_LUXALG), y)
LIBS += -L$(BULD_ROOT)/libs/ingenic/alg -ljzdl.m -L$(BULD_ROOT)/libs/lux -ljz_faceDet
# LIBS += -L$(BULD_ROOT)/libs/ingenic/alg -ljzdl.m -L$(BULD_ROOT)/libs/lux -ljz_faceDet -lssd_pukeDet -lssd_kickquiltDet -lssd_actionDet
else
LIBS += -L$(BULD_ROOT)/libs/ingenic/alg -lfaceDet_inf -ljzdl.m
# LIBS += -L$(BULD_ROOT)/libs/ingenic/alg -lfaceDet_inf -ljzdl.m -L$(BULD_ROOT)/libs/lux -lssd_pukeDet -lssd_kickquiltDet -lssd_actionDet
endif

LIBS += $(BULD_ROOT)/libs/ingenic/libalog.a \
		$(BULD_ROOT)/libs/ingenic/libimp.a \
		$(BULD_ROOT)/libs/ingenic/libmove_inf.a \
		$(BULD_ROOT)/libs/ingenic/libsysutils.a \
		$(BULD_ROOT)/libs/tuya/libtuya_ipc.a \
		$(BULD_ROOT)/libs/tuya/libmbedtls.a \
		$(BULD_ROOT)/libs/third/libzbar.a\
		$(BULD_ROOT)/libs/third/libiniparser.a 

LIBS += $(BULD_ROOT)/libs/ingenic/alg/libmxu_merge.a \
		$(BULD_ROOT)/libs/ingenic/alg/libmxu_objdetect.a \
		$(BULD_ROOT)/libs/ingenic/alg/libmxu_contrib.a \
		$(BULD_ROOT)/libs/ingenic/alg/libmxu_video.a \
		$(BULD_ROOT)/libs/ingenic/alg/libmxu_imgproc.a \
		$(BULD_ROOT)/libs/ingenic/alg/libmxu_core.a \
		$(BULD_ROOT)/libs/ingenic/alg/libfftw3f.a \
		$(BULD_ROOT)/libs/ingenic/alg/libperm_inf.a 

OBJS = $(SRC_FILE:%.c=%.o) $(MAIN_FILE:%.c=%.o)

CPP_OBJS = $(CPP_FILE:%.c=%.o)

TARGET := $(BULD_ROOT)/out/media_sample

all: $(TARGET) 

$(TARGET): $(OBJS) $(CPP_OBJS)
	$(CPLUSPLUS)  $^ -o $@  $(CFLAGS) $(LIBS) $(LDFLAGS)
	$(STRIP) $(TARGET)

$(OBJS): %.o:%.c
	$(CC) -c $(CFLAGS) $(INC_PATH) $< -o $@

$(CPP_OBJS): %.o:%.c
	$(CPLUSPLUS) -c $(CFLAGS) $(LDFLAGS) $(INC_PATH) $(CPP_INC_PATH) $< -o $@

clean:
	rm -f $(shell find $(BULD_ROOT)  -name "*.o")
	rm -f $(TARGET)
	
.PHONY:all clean