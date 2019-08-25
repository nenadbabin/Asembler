################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../Emulator.cpp \
../Instruction.cpp \
../Linker.cpp \
../Memory.cpp \
../ObjectFile.cpp \
../Relocation.cpp \
../Section.cpp \
../Symbol.cpp \
../main.cpp 

OBJS += \
./Emulator.o \
./Instruction.o \
./Linker.o \
./Memory.o \
./ObjectFile.o \
./Relocation.o \
./Section.o \
./Symbol.o \
./main.o 

CPP_DEPS += \
./Emulator.d \
./Instruction.d \
./Linker.d \
./Memory.d \
./ObjectFile.d \
./Relocation.d \
./Section.d \
./Symbol.d \
./main.d 


# Each subdirectory must supply rules for building sources it contributes
%.o: ../%.cpp
	@echo 'Building file: $<'
	@echo 'Invoking: GCC C++ Compiler'
	g++ -O0 -g3 -Wall -c -fmessage-length=0 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$(@)" -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


