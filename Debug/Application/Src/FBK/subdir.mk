################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (12.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/Src/FBK/fbk_main.c 

OBJS += \
./Application/Src/FBK/fbk_main.o 

C_DEPS += \
./Application/Src/FBK/fbk_main.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Src/FBK/%.o Application/Src/FBK/%.su Application/Src/FBK/%.cyclo: ../Application/Src/FBK/%.c Application/Src/FBK/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/drivers" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/FBK" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/FBK" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/MESS" -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Wswitch-default -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-Src-2f-FBK

clean-Application-2f-Src-2f-FBK:
	-$(RM) ./Application/Src/FBK/fbk_main.cyclo ./Application/Src/FBK/fbk_main.d ./Application/Src/FBK/fbk_main.o ./Application/Src/FBK/fbk_main.su

.PHONY: clean-Application-2f-Src-2f-FBK

