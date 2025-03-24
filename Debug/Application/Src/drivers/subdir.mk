################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/Src/drivers/INA219-driver.c \
../Application/Src/drivers/PGA113-driver.c \
../Application/Src/drivers/WS2812b-driver.c \
../Application/Src/drivers/dau_card-driver.c \
../Application/Src/drivers/usb_comm.c 

OBJS += \
./Application/Src/drivers/INA219-driver.o \
./Application/Src/drivers/PGA113-driver.o \
./Application/Src/drivers/WS2812b-driver.o \
./Application/Src/drivers/dau_card-driver.o \
./Application/Src/drivers/usb_comm.o 

C_DEPS += \
./Application/Src/drivers/INA219-driver.d \
./Application/Src/drivers/PGA113-driver.d \
./Application/Src/drivers/WS2812b-driver.d \
./Application/Src/drivers/dau_card-driver.d \
./Application/Src/drivers/usb_comm.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Src/drivers/%.o Application/Src/drivers/%.su Application/Src/drivers/%.cyclo: ../Application/Src/drivers/%.c Application/Src/drivers/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM7 -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/CMSIS/DSP/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/drivers" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/CFG" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/CFG" -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Wswitch-default -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-Src-2f-drivers

clean-Application-2f-Src-2f-drivers:
	-$(RM) ./Application/Src/drivers/INA219-driver.cyclo ./Application/Src/drivers/INA219-driver.d ./Application/Src/drivers/INA219-driver.o ./Application/Src/drivers/INA219-driver.su ./Application/Src/drivers/PGA113-driver.cyclo ./Application/Src/drivers/PGA113-driver.d ./Application/Src/drivers/PGA113-driver.o ./Application/Src/drivers/PGA113-driver.su ./Application/Src/drivers/WS2812b-driver.cyclo ./Application/Src/drivers/WS2812b-driver.d ./Application/Src/drivers/WS2812b-driver.o ./Application/Src/drivers/WS2812b-driver.su ./Application/Src/drivers/dau_card-driver.cyclo ./Application/Src/drivers/dau_card-driver.d ./Application/Src/drivers/dau_card-driver.o ./Application/Src/drivers/dau_card-driver.su ./Application/Src/drivers/usb_comm.cyclo ./Application/Src/drivers/usb_comm.d ./Application/Src/drivers/usb_comm.o ./Application/Src/drivers/usb_comm.su

.PHONY: clean-Application-2f-Src-2f-drivers

