################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../src/deadlock.c \
../src/dispositivos_io.c \
../src/kernel.c \
../src/mensajes_memoria.c \
../src/procesar_carpinchos.c \
../src/semaforos.c 

OBJS += \
./src/deadlock.o \
./src/dispositivos_io.o \
./src/kernel.o \
./src/mensajes_memoria.o \
./src/procesar_carpinchos.o \
./src/semaforos.o 

C_DEPS += \
./src/deadlock.d \
./src/dispositivos_io.d \
./src/kernel.d \
./src/mensajes_memoria.d \
./src/procesar_carpinchos.d \
./src/semaforos.d 


# Each subdirectory must supply rules for building sources it contributes
src/%.o: ../src/%.c
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C Compiler'
	gcc -I"/media/sf_carpeta-compartida/0 TP_Sistemas_operativos_2021-main/futiles/futiles" -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


