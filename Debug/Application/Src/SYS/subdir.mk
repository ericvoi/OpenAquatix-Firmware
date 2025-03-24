################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/Src/SYS/sys_error.c \
../Application/Src/SYS/sys_main.c 

OBJS += \
./Application/Src/SYS/sys_error.o \
./Application/Src/SYS/sys_main.o 

C_DEPS += \
./Application/Src/SYS/sys_error.d \
./Application/Src/SYS/sys_main.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Src/SYS/%.o Application/Src/SYS/%.su Application/Src/SYS/%.cyclo: ../Application/Src/SYS/%.c Application/Src/SYS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM7 -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/CMSIS/DSP/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/drivers" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/CFG" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/CFG" -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Wswitch-default -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-Src-2f-SYS

clean-Application-2f-Src-2f-SYS:
	-$(RM) ./Application/Src/SYS/sys_error.cyclo ./Application/Src/SYS/sys_error.d ./Application/Src/SYS/sys_error.o ./Application/Src/SYS/sys_error.su ./Application/Src/SYS/sys_main.cyclo ./Application/Src/SYS/sys_main.d ./Application/Src/SYS/sys_main.o ./Application/Src/SYS/sys_main.su

.PHONY: clean-Application-2f-Src-2f-SYS

