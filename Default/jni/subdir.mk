################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../jni/font.c \
../jni/main.c \
../jni/native-audio-jni.c 

OBJS += \
./jni/font.o \
./jni/main.o \
./jni/native-audio-jni.o 

C_DEPS += \
./jni/font.d \
./jni/main.d \
./jni/native-audio-jni.d 


# Each subdirectory must supply rules for building sources it contributes
jni/%.o: ../jni/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I/home/e303/下载/android-ndk-r9d/platforms/android-9/arch-arm/usr/include -I/home/e303/下载/android-ndk-r9d/sources/android/native_app_glue -I/home/e303/下载/android-ndk-r9d/cocos2d-x-3.0rc1/external/freetype2/include/android/freetype2 -O2 -g -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@:%.o=%.d)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


