################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/audio_filter.c \
../src/main_arm.c \
../src/nios_bridge.c \
../src/sd_manager.c 

OBJS += \
./src/audio_filter.o \
./src/main_arm.o \
./src/nios_bridge.o \
./src/sd_manager.o 

C_DEPS += \
./src/audio_filter.d \
./src/main_arm.d \
./src/nios_bridge.d \
./src/sd_manager.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	arm-altera-eabi-gcc -O3 -Wall -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


