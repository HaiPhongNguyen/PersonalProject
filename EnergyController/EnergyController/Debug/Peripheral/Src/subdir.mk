################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Peripheral/Src/ADC.c \
../Peripheral/Src/ADS1115.c \
../Peripheral/Src/DS1307.c \
../Peripheral/Src/INA219.c \
../Peripheral/Src/LCD2004.c \
../Peripheral/Src/Pzem004T.c 

OBJS += \
./Peripheral/Src/ADC.o \
./Peripheral/Src/ADS1115.o \
./Peripheral/Src/DS1307.o \
./Peripheral/Src/INA219.o \
./Peripheral/Src/LCD2004.o \
./Peripheral/Src/Pzem004T.o 

C_DEPS += \
./Peripheral/Src/ADC.d \
./Peripheral/Src/ADS1115.d \
./Peripheral/Src/DS1307.d \
./Peripheral/Src/INA219.d \
./Peripheral/Src/LCD2004.d \
./Peripheral/Src/Pzem004T.d 


# Each subdirectory must supply rules for building sources it contributes
Peripheral/Src/%.o Peripheral/Src/%.su Peripheral/Src/%.cyclo: ../Peripheral/Src/%.c Peripheral/Src/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m3 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F103xB -c -I../Core/Inc -I../Peripheral/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc -I../Drivers/STM32F1xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F1xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM3 -I../Display/Inc -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Peripheral-2f-Src

clean-Peripheral-2f-Src:
	-$(RM) ./Peripheral/Src/ADC.cyclo ./Peripheral/Src/ADC.d ./Peripheral/Src/ADC.o ./Peripheral/Src/ADC.su ./Peripheral/Src/ADS1115.cyclo ./Peripheral/Src/ADS1115.d ./Peripheral/Src/ADS1115.o ./Peripheral/Src/ADS1115.su ./Peripheral/Src/DS1307.cyclo ./Peripheral/Src/DS1307.d ./Peripheral/Src/DS1307.o ./Peripheral/Src/DS1307.su ./Peripheral/Src/INA219.cyclo ./Peripheral/Src/INA219.d ./Peripheral/Src/INA219.o ./Peripheral/Src/INA219.su ./Peripheral/Src/LCD2004.cyclo ./Peripheral/Src/LCD2004.d ./Peripheral/Src/LCD2004.o ./Peripheral/Src/LCD2004.su ./Peripheral/Src/Pzem004T.cyclo ./Peripheral/Src/Pzem004T.d ./Peripheral/Src/Pzem004T.o ./Peripheral/Src/Pzem004T.su

.PHONY: clean-Peripheral-2f-Src

