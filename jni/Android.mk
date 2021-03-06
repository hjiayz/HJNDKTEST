# Copyright (C) 2010 The Android Open Source Project
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
LOCAL_PATH := $(call my-dir)

include $(CLEAR_VARS)
LOCAL_C_INCLUDES	+= /home/e303/下载/android-ndk-r9d/cocos2d-x-3.0rc1/external/freetype2/include/android/freetype2/
LOCAL_C_INCLUDES	+= /home/e303/下载/android-ndk-r9d/cocos2d-x-3.0rc1/external/freetype2/include/android/freetype2/freetype/config/
LOCAL_C_INCLUDES	+= /home/e303/下载/android-ndk-r9d/cocos2d-x-3.0rc1/external/freetype2/include/android/freetype2/freetype/
LOCAL_LDFLAGS += $(LOCAL_PATH)/libfreetype.a
LOCAL_LDFLAGS += $(LOCAL_PATH)/libpng.a
LOCAL_MODULE    := native-activity
LOCAL_SRC_FILES := main.c
LOCAL_SRC_FILES += native-audio-jni.c
LOCAL_SRC_FILES += font.c
LOCAL_LDLIBS    := -llog -landroid -lOpenSLES -lz
LOCAL_STATIC_LIBRARIES := android_native_app_glue
include $(BUILD_SHARED_LIBRARY)

$(call import-module,android/native_app_glue)
