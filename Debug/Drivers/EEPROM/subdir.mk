################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (10.3-2021.10)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Drivers/EEPROM/EEPROM.c 

OBJS += \
./Drivers/EEPROM/EEPROM.o 

C_DEPS += \
./Drivers/EEPROM/EEPROM.d 


# Each subdirectory must supply rules for building sources it contributes
Drivers/EEPROM/%.o Drivers/EEPROM/%.su Drivers/EEPROM/%.cyclo: ../Drivers/EEPROM/%.c Drivers/EEPROM/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m0 -std=gnu11 -g3 -DDEBUG -DUSE_HAL_DRIVER -DSTM32F070xB -c -I"C:/Users/USER/STM32CubeIDE/Neptune_4G/Venus/Drivers/EEPROM" -I"C:/Users/USER/STM32CubeIDE/Neptune_4G/Venus/Drivers/SIM800" -I../Core/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc -I../Drivers/STM32F0xx_HAL_Driver/Inc/Legacy -I../Drivers/CMSIS/Device/ST/STM32F0xx/Include -I../Drivers/CMSIS/Include -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM0 -O0 -ffunction-sections -fdata-sections -Wall -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfloat-abi=soft -mthumb -o "$@"

clean: clean-Drivers-2f-EEPROM

clean-Drivers-2f-EEPROM:
	-$(RM) ./Drivers/EEPROM/EEPROM.cyclo ./Drivers/EEPROM/EEPROM.d ./Drivers/EEPROM/EEPROM.o ./Drivers/EEPROM/EEPROM.su

.PHONY: clean-Drivers-2f-EEPROM

