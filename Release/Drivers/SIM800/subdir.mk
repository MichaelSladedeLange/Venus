################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/SIM800/SIM800H.c 

OBJS += \
./Drivers/SIM800/SIM800H.o 

C_DEPS += \
./Drivers/SIM800/SIM800H.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/SIM800/%.o Drivers/SIM800/%.su: ../Drivers/SIM800/%.c Drivers/SIM800/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -DUSE_HAL_DRIVER -DSTM32F070xB -c -I../Core/Inc -I"D:/Work/STM32IDEWorkspace/Venus/Drivers/SIM800" -I"D:/Work/STM32IDEWorkspace/Venus/Drivers/EEPROM" -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM0 -O3 -ffunction-sections -fdata-sections -Wall -fstack-usage -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-SIM800

clean-Drivers-2f-SIM800:
	-$(RM) ./Drivers/SIM800/SIM800H.d ./Drivers/SIM800/SIM800H.o ./Drivers/SIM800/SIM800H.su

.PHONY: clean-Drivers-2f-SIM800

