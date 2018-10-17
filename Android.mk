LOCAL_PATH := $(call my-dir)

###########################################################

include $(CLEAR_VARS)
LOCAL_CFLAGS += -fPIE
LOCAL_LDFLAGS += -fPIE -pie

LOCAL_MODULE    := bootstrap
LOCAL_SRC_FILES := bootstrap.c
LOCAL_LDLIBS := -llog
LOCAL_STATIC_LIBRARIES :=
NDK_APP_DST_DIR := build/$(TARGET_ARCH_ABI)

include $(BUILD_EXECUTABLE)

###########################################################

include $(CLEAR_VARS)
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
LOCAL_MODULE    := bridge
LOCAL_SRC_FILES := bridge.cpp
LOCAL_LDLIBS := -llog
LOCAL_LDFLAGS	:=	-L./lib/armeabi -L./lib/x86
LOCAL_STATIC_LIBRARIES :=
LOCAL_CFLAGS :=  -I./include/
LOCAL_STATIC_LIBRARIES :=
NDK_APP_DST_DIR := build/$(TARGET_ARCH_ABI)
LOCAL_CPPFLAGS := -std=c++1y -frtti

include $(BUILD_SHARED_LIBRARY)

###########################################################

include $(CLEAR_VARS)
LOCAL_DISABLE_FATAL_LINKER_WARNINGS := true
NDK_APP_DST_DIR := build/$(TARGET_ARCH_ABI)

LOCAL_MODULE    := payload
LOCAL_SRC_FILES := payload.cpp api.cpp common.cpp ui.cpp entity.cpp \
	resource.cpp inventory.cpp spell.cpp buff.cpp actor.cpp card.cpp \
	chat.cpp map.cpp engine.cpp json2lua.cpp net.cpp

ifeq ($(TARGET_ARCH), arm)
	OBJCOPY := ./bin/arm-objcopy
	OBJCOPY-O := elf32-littlearm
	OBJCOPY-B := arm
endif

ifeq ($(TARGET_ARCH), x86)
	OBJCOPY := ./bin/i686-objcopy
	OBJCOPY-O := elf32-i386
	OBJCOPY-B := i386
endif

RESOURCE_LUA := ./build/bundle
RESOURCE_NAME := $(NDK_APP_DST_DIR)/resource.o

$(shell $(OBJCOPY) -I binary -O $(OBJCOPY-O) -B $(OBJCOPY-B) $(RESOURCE_LUA) $(RESOURCE_NAME))
LOCAL_RESOURCE := $(RESOURCE_NAME)
LOCAL_LDLIBS := -llog -llua -liconv -lcurl -luv
LOCAL_LDFLAGS	:= -L./lib/armeabi -L./lib/x86 $(LOCAL_RESOURCE)

LOCAL_CFLAGS :=  -I./include/ \
 								 -I./3rd/lua/jni \
								 -I./3rd/iconv/jni/include \
								 -I./3rd/curl/jni/include \
								 -I./3rd/libuv/include \
								 -DHAVE_LITTLE_ENDIAN \
								 # -O0 \
								 # -mllvm -enable-bcfobf \
								 # -mllvm -enable-cffobf \
								 # -mllvm -enable-splitobf \
								 # -mllvm -enable-subobf \
								 # -mllvm -enable-strcry \

LOCAL_CPPFLAGS := -std=c++1y -frtti


include $(BUILD_SHARED_LIBRARY)
