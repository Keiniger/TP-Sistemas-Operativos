################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/init_swamp.c \
../src/mensajes_swamp.c \
../src/swamp.c \
../src/utils.c 

OBJS += \
./src/init_swamp.o \
./src/mensajes_swamp.o \
./src/swamp.o \
./src/utils.o 

C_DEPS += \
./src/init_swamp.d \
./src/mensajes_swamp.d \
./src/swamp.d \
./src/utils.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/media/sf_carpeta-compartida/0 TP_Sistemas_operativos_2021-main/futiles" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


