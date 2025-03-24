################################################################################
# Automatically-generated file. Do not edit!
# Toolchain: GNU Tools for STM32 (13.3.rel1)
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../Application/Src/MESS/mess_adc.c \
../Application/Src/MESS/mess_calibration.c \
../Application/Src/MESS/mess_demodulate.c \
../Application/Src/MESS/mess_encoding.c \
../Application/Src/MESS/mess_error_correction.c \
../Application/Src/MESS/mess_evaluate.c \
../Application/Src/MESS/mess_feedback.c \
../Application/Src/MESS/mess_input.c \
../Application/Src/MESS/mess_main.c \
../Application/Src/MESS/mess_modulate.c \
../Application/Src/MESS/mess_packet.c 

OBJS += \
./Application/Src/MESS/mess_adc.o \
./Application/Src/MESS/mess_calibration.o \
./Application/Src/MESS/mess_demodulate.o \
./Application/Src/MESS/mess_encoding.o \
./Application/Src/MESS/mess_error_correction.o \
./Application/Src/MESS/mess_evaluate.o \
./Application/Src/MESS/mess_feedback.o \
./Application/Src/MESS/mess_input.o \
./Application/Src/MESS/mess_main.o \
./Application/Src/MESS/mess_modulate.o \
./Application/Src/MESS/mess_packet.o 

C_DEPS += \
./Application/Src/MESS/mess_adc.d \
./Application/Src/MESS/mess_calibration.d \
./Application/Src/MESS/mess_demodulate.d \
./Application/Src/MESS/mess_encoding.d \
./Application/Src/MESS/mess_error_correction.d \
./Application/Src/MESS/mess_evaluate.d \
./Application/Src/MESS/mess_feedback.d \
./Application/Src/MESS/mess_input.d \
./Application/Src/MESS/mess_main.d \
./Application/Src/MESS/mess_modulate.d \
./Application/Src/MESS/mess_packet.d 


# Each subdirectory must supply rules for building sources it contributes
Application/Src/MESS/%.o Application/Src/MESS/%.su Application/Src/MESS/%.cyclo: ../Application/Src/MESS/%.c Application/Src/MESS/subdir.mk
	arm-none-eabi-gcc "$<" -mcpu=cortex-m7 -std=gnu11 -g3 -DDEBUG -DARM_MATH_CM7 -DUSE_PWR_LDO_SUPPLY -DUSE_HAL_DRIVER -DSTM32H723xx -c -I../Core/Inc -I../Drivers/CMSIS/DSP/Include -I../Drivers/STM32H7xx_HAL_Driver/Inc -I../Drivers/STM32H7xx_HAL_Driver/Inc/Legacy -I../Middlewares/Third_Party/FreeRTOS/Source/include -I../Middlewares/Third_Party/FreeRTOS/Source/CMSIS_RTOS_V2 -I../Middlewares/Third_Party/FreeRTOS/Source/portable/GCC/ARM_CM4F -I../Drivers/CMSIS/Device/ST/STM32H7xx/Include -I../Drivers/CMSIS/Include -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/drivers" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src" -I../USB_DEVICE/App -I../USB_DEVICE/Target -I../Middlewares/ST/STM32_USB_Device_Library/Core/Inc -I../Middlewares/ST/STM32_USB_Device_Library/Class/CDC/Inc -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/COMM" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/common/utils" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/MESS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/SYS" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Inc/CFG" -I"C:/Users/ericv/STM32CubeIDE/workspace_1.17.0/UAM/Application/Src/CFG" -O0 -ffunction-sections -fdata-sections -Wall -Wextra -Wswitch-default -fstack-usage -fcyclomatic-complexity -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" --specs=nano.specs -mfpu=fpv5-d16 -mfloat-abi=hard -mthumb -o "$@"

clean: clean-Application-2f-Src-2f-MESS

clean-Application-2f-Src-2f-MESS:
	-$(RM) ./Application/Src/MESS/mess_adc.cyclo ./Application/Src/MESS/mess_adc.d ./Application/Src/MESS/mess_adc.o ./Application/Src/MESS/mess_adc.su ./Application/Src/MESS/mess_calibration.cyclo ./Application/Src/MESS/mess_calibration.d ./Application/Src/MESS/mess_calibration.o ./Application/Src/MESS/mess_calibration.su ./Application/Src/MESS/mess_demodulate.cyclo ./Application/Src/MESS/mess_demodulate.d ./Application/Src/MESS/mess_demodulate.o ./Application/Src/MESS/mess_demodulate.su ./Application/Src/MESS/mess_encoding.cyclo ./Application/Src/MESS/mess_encoding.d ./Application/Src/MESS/mess_encoding.o ./Application/Src/MESS/mess_encoding.su ./Application/Src/MESS/mess_error_correction.cyclo ./Application/Src/MESS/mess_error_correction.d ./Application/Src/MESS/mess_error_correction.o ./Application/Src/MESS/mess_error_correction.su ./Application/Src/MESS/mess_evaluate.cyclo ./Application/Src/MESS/mess_evaluate.d ./Application/Src/MESS/mess_evaluate.o ./Application/Src/MESS/mess_evaluate.su ./Application/Src/MESS/mess_feedback.cyclo ./Application/Src/MESS/mess_feedback.d ./Application/Src/MESS/mess_feedback.o ./Application/Src/MESS/mess_feedback.su ./Application/Src/MESS/mess_input.cyclo ./Application/Src/MESS/mess_input.d ./Application/Src/MESS/mess_input.o ./Application/Src/MESS/mess_input.su ./Application/Src/MESS/mess_main.cyclo ./Application/Src/MESS/mess_main.d ./Application/Src/MESS/mess_main.o ./Application/Src/MESS/mess_main.su ./Application/Src/MESS/mess_modulate.cyclo ./Application/Src/MESS/mess_modulate.d ./Application/Src/MESS/mess_modulate.o ./Application/Src/MESS/mess_modulate.su ./Application/Src/MESS/mess_packet.cyclo ./Application/Src/MESS/mess_packet.d ./Application/Src/MESS/mess_packet.o ./Application/Src/MESS/mess_packet.su

.PHONY: clean-Application-2f-Src-2f-MESS

