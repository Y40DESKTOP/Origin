################################################################################
# Automatically-generated file. Do not edit!
################################################################################

# Add inputs and outputs from these tool invocations to the build variables 
CFG_SRCS += \
../dsp.cfg 

LIB_SRCS += \
../Platform.lib 

C_SRCS += \
../main_dsp.c \
../resource_sync.c \
../server.c \
../syslink.c \
../syslink_service.c 

OBJS += \
./main_dsp.obj \
./resource_sync.obj \
./server.obj \
./syslink.obj \
./syslink_service.obj 

C_DEPS += \
./main_dsp.pp \
./resource_sync.pp \
./server.pp \
./syslink.pp \
./syslink_service.pp 

GEN_MISC_DIRS += \
./configPkg/ 

GEN_CMDS += \
./configPkg/linker.cmd 

GEN_OPTS += \
./configPkg/compiler.opt 

GEN_FILES += \
./configPkg/linker.cmd \
./configPkg/compiler.opt 

GEN_FILES__QUOTED += \
"configPkg\linker.cmd" \
"configPkg\compiler.opt" 

GEN_MISC_DIRS__QUOTED += \
"configPkg\" 

C_DEPS__QUOTED += \
"main_dsp.pp" \
"resource_sync.pp" \
"server.pp" \
"syslink.pp" \
"syslink_service.pp" 

OBJS__QUOTED += \
"main_dsp.obj" \
"resource_sync.obj" \
"server.obj" \
"syslink.obj" \
"syslink_service.obj" 

GEN_CMDS__FLAG += \
-l"./configPkg/linker.cmd" 

GEN_OPTS__FLAG += \
--cmd_file="./configPkg/compiler.opt" 

C_SRCS__QUOTED += \
"../main_dsp.c" \
"../resource_sync.c" \
"../server.c" \
"../syslink.c" \
"../syslink_service.c" 


