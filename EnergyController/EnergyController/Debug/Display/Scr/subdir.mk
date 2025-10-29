################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Display/Scr/Display.c \
../Display/Scr/Read_Sensor.c 

OBJS += \
./Display/Scr/Display.o \
./Display/Scr/Read_Sensor.o 

C_DEPS += \
./Display/Scr/Display.d \
./Display/Scr/Read_Sensor.d 


# Each subdirectory must supply rules for building sources it contributes
Display/Scr/%.o Display/Scr/%.su Display/Scr/%.cyclo: ../Display/Scr/%.c Display/Scr/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Peripheral/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Display/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Display-2f-Scr

clean-Display-2f-Scr:
	-$(RM) ./Display/Scr/Display.cyclo ./Display/Scr/Display.d ./Display/Scr/Display.o ./Display/Scr/Display.su ./Display/Scr/Read_Sensor.cyclo ./Display/Scr/Read_Sensor.d ./Display/Scr/Read_Sensor.o ./Display/Scr/Read_Sensor.su

.PHONY: clean-Display-2f-Scr

