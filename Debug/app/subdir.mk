################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
C_SRCS += \
../app/main.c 

OBJS += \
./app/main.o 

C_DEPS += \
./app/main.d 


# Each subdirectory must supply rules for building sources it contributes
app/%.o: ../app/%.c app/subdir.mk
	@echo 'Building file: $<'
	@echo 'Invoking: GNU RISC-V Cross C Compiler'
	riscv64-unknown-elf-gcc  -march=rv32imfc -mabi=ilp32f  -msmall-data-limit=8 -mstrict-align -mno-save-restore  -O0 -fmessage-length=0 -fsigned-char -ffunction-sections -fdata-sections  -g3 -DSYSCLK_PLL -DHSECLK_VAL=16000000 -DCKO_PLL0 -DRETARGET -I../platform/ldscripts -I../platform/include -std=gnu11 -MMD -MP -MF"$(@:%.o=%.d)" -MT"$@" -c -o "$@" "$<"
	@echo 'Finished building: $<'
	@echo ' '


