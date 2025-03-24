################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/Src/COMM/comm_config_menu.c \
../Application/Src/COMM/comm_debug_menu.c \
../Application/Src/COMM/comm_evaluation_menu.c \
../Application/Src/COMM/comm_function_loops.c \
../Application/Src/COMM/comm_hist_menu.c \
../Application/Src/COMM/comm_main.c \
../Application/Src/COMM/comm_main_menu.c \
../Application/Src/COMM/comm_menu_system.c \
../Application/Src/COMM/comm_txrx_menu.c 

OBJS += \
./Application/Src/COMM/comm_config_menu.o \
./Application/Src/COMM/comm_debug_menu.o \
./Application/Src/COMM/comm_evaluation_menu.o \
./Application/Src/COMM/comm_function_loops.o \
./Application/Src/COMM/comm_hist_menu.o \
./Application/Src/COMM/comm_main.o \
./Application/Src/COMM/comm_main_menu.o \
./Application/Src/COMM/comm_menu_system.o \
./Application/Src/COMM/comm_txrx_menu.o 

C_DEPS += \
./Application/Src/COMM/comm_config_menu.d \
./Application/Src/COMM/comm_debug_menu.d \
./Application/Src/COMM/comm_evaluation_menu.d \
./Application/Src/COMM/comm_function_loops.d \
./Application/Src/COMM/comm_hist_menu.d \
./Application/Src/COMM/comm_main.d \
./Application/Src/COMM/comm_main_menu.d \
./Application/Src/COMM/comm_menu_system.d \
./Application/Src/COMM/comm_txrx_menu.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Src/COMM/%.o Application/Src/COMM/%.su Application/Src/COMM/%.cyclo: ../Application/Src/COMM/%.c Application/Src/COMM/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM7 -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/CMSIS/DSP/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/drivers" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/CFG" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/CFG" -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Wswitch-default -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-Src-2f-COMM

clean-Application-2f-Src-2f-COMM:
	-$(RM) ./Application/Src/COMM/comm_config_menu.cyclo ./Application/Src/COMM/comm_config_menu.d ./Application/Src/COMM/comm_config_menu.o ./Application/Src/COMM/comm_config_menu.su ./Application/Src/COMM/comm_debug_menu.cyclo ./Application/Src/COMM/comm_debug_menu.d ./Application/Src/COMM/comm_debug_menu.o ./Application/Src/COMM/comm_debug_menu.su ./Application/Src/COMM/comm_evaluation_menu.cyclo ./Application/Src/COMM/comm_evaluation_menu.d ./Application/Src/COMM/comm_evaluation_menu.o ./Application/Src/COMM/comm_evaluation_menu.su ./Application/Src/COMM/comm_function_loops.cyclo ./Application/Src/COMM/comm_function_loops.d ./Application/Src/COMM/comm_function_loops.o ./Application/Src/COMM/comm_function_loops.su ./Application/Src/COMM/comm_hist_menu.cyclo ./Application/Src/COMM/comm_hist_menu.d ./Application/Src/COMM/comm_hist_menu.o ./Application/Src/COMM/comm_hist_menu.su ./Application/Src/COMM/comm_main.cyclo ./Application/Src/COMM/comm_main.d ./Application/Src/COMM/comm_main.o ./Application/Src/COMM/comm_main.su ./Application/Src/COMM/comm_main_menu.cyclo ./Application/Src/COMM/comm_main_menu.d ./Application/Src/COMM/comm_main_menu.o ./Application/Src/COMM/comm_main_menu.su ./Application/Src/COMM/comm_menu_system.cyclo ./Application/Src/COMM/comm_menu_system.d ./Application/Src/COMM/comm_menu_system.o ./Application/Src/COMM/comm_menu_system.su ./Application/Src/COMM/comm_txrx_menu.cyclo ./Application/Src/COMM/comm_txrx_menu.d ./Application/Src/COMM/comm_txrx_menu.o ./Application/Src/COMM/comm_txrx_menu.su

.PHONY: clean-Application-2f-Src-2f-COMM

