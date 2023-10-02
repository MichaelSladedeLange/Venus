################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/RF_RX/RF_RX.c 

OBJS += \
./Drivers/RF_RX/RF_RX.o 

C_DEPS += \
./Drivers/RF_RX/RF_RX.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/RF_RX/%.o Drivers/RF_RX/%.su: ../Drivers/RF_RX/%.c Drivers/RF_RX/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F070xB -c -I../Core/Inc -I"D:/Work/STM32IDEWorkspace/Venus/Drivers/SIM800" -I"D:/Work/STM32IDEWorkspace/Venus/Drivers/EEPROM" -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM0 -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-RF_RX

clean-Drivers-2f-RF_RX:
	-$(RM) ./Drivers/RF_RX/RF_RX.d ./Drivers/RF_RX/RF_RX.o ./Drivers/RF_RX/RF_RX.su

.PHONY: clean-Drivers-2f-RF_RX

