################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/memoria.c \
../src/tlb.c 

OBJS += \
./src/memoria.o \
./src/tlb.o 

C_DEPS += \
./src/memoria.d \
./src/tlb.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/media/sf_carpeta-compartida/0 TP_Sistemas_operativos_2021-main/futiles" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


