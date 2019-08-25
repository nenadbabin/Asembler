################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CPP_SRCS += \
../FirstPass.cpp \
../Pass.cpp \
../Relocation.cpp \
../SecondPass.cpp \
../Section.cpp \
../Symbol.cpp \
../main.cpp 

OBJS += \
./FirstPass.o \
./Pass.o \
./Relocation.o \
./SecondPass.o \
./Section.o \
./Symbol.o \
./main.o 

CPP_DEPS += \
./FirstPass.d \
./Pass.d \
./Relocation.d \
./SecondPass.d \
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


