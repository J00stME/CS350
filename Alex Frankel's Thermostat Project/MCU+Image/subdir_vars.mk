################################################################################
# Automatically-generated file. Do not edit!
################################################################################

SHELL = cmd.exe

# Add inputs and outputs from these tool invocations to the build variables 
SYSCFG_SRCS += \
../gpiointerrupt.syscfg \
../image.syscfg 

LDS_SRCS += \
../cc32xxs_nortos.lds 

C_SRCS += \
../gpiointerrupt.c \
./syscfg/ti_drivers_config.c \
../main_nortos.c 

GEN_FILES += \
./syscfg/ti_drivers_config.c 

GEN_MISC_DIRS += \
./syscfg/ \
./syscfg/ 

C_DEPS += \
./gpiointerrupt.d \
./syscfg/ti_drivers_config.d \
./main_nortos.d 

OBJS += \
./gpiointerrupt.o \
./syscfg/ti_drivers_config.o \
./main_nortos.o 

GEN_MISC_FILES += \
./syscfg/ti_drivers_config.h \
./syscfg/ti_utils_build_linker.cmd.genlibs \
./syscfg/syscfg_c.rov.xs \
./syscfg/ti_drivers_net_wifi_config.json 

GEN_MISC_DIRS__QUOTED += \
"syscfg\" \
"syscfg\" 

OBJS__QUOTED += \
"gpiointerrupt.o" \
"syscfg\ti_drivers_config.o" \
"main_nortos.o" 

GEN_MISC_FILES__QUOTED += \
"syscfg\ti_drivers_config.h" \
"syscfg\ti_utils_build_linker.cmd.genlibs" \
"syscfg\syscfg_c.rov.xs" \
"syscfg\ti_drivers_net_wifi_config.json" 

C_DEPS__QUOTED += \
"gpiointerrupt.d" \
"syscfg\ti_drivers_config.d" \
"main_nortos.d" 

GEN_FILES__QUOTED += \
"syscfg\ti_drivers_config.c" 

C_SRCS__QUOTED += \
"../gpiointerrupt.c" \
"./syscfg/ti_drivers_config.c" \
"../main_nortos.c" 

SYSCFG_SRCS__QUOTED += \
"../gpiointerrupt.syscfg" \
"../image.syscfg" 


