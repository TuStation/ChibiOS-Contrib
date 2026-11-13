##############################################################################
# Build options.
##############################################################################

USE_OPT = -O2 -ggdb -fomit-frame-pointer -falign-functions=16
USE_LINK_GC = yes
USE_LTO = yes
USE_THUMB = yes
USE_VERBOSE_COMPILE = no
USE_SMART_BUILD = yes
USE_PROCESS_STACKSIZE = 0x400
USE_EXCEPTIONS_STACKSIZE = 0x400
USE_FPU = no

##############################################################################
# Project sources and paths.
##############################################################################

PROJECT = ch32f203
CHIBIOS ?= ../../../../ChibiOS
CHIBIOS_CONTRIB ?= $(CHIBIOS)/../ChibiOS-Contrib

include $(CHIBIOS)/os/license/license.mk
include $(CHIBIOS_CONTRIB)/os/common/startup/ARMCMx/compilers/GCC/mk/startup_ch32f20x.mk
include $(CHIBIOS_CONTRIB)/os/hal/hal.mk
include $(CHIBIOS_CONTRIB)/os/hal/ports/WCH/CH32F20x/platform.mk

BOARD_PATH = $(CURDIR)/board
include $(BOARD_PATH)/board.mk

include $(CHIBIOS)/os/hal/osal/rt-nil/osal.mk
include $(CHIBIOS)/os/rt/rt.mk
include $(CHIBIOS)/os/common/ports/ARMv7-M/compilers/GCC/mk/port.mk

LDSCRIPT = $(CHIBIOS_CONTRIB)/os/common/startup/ARMCMx/compilers/GCC/ld/CH32F203.ld

CSRC = $(ALLCSRC) main.c
CPPSRC = $(ALLCPPSRC)
ACSRC =
ACPPSRC =
TCSRC =
TCPPSRC =
ASMSRC = $(ALLASMSRC)
ASMXSRC = $(ALLXASMSRC)
INCDIR = $(ALLINC)

##############################################################################
# Toolchain options.
##############################################################################

MCU = cortex-m3
TRGT = arm-none-eabi-
CC = $(TRGT)gcc
CPPC = $(TRGT)g++
LD = $(TRGT)gcc
CP = $(TRGT)objcopy
AS = $(TRGT)gcc -x assembler-with-cpp
AR = $(TRGT)ar
OD = $(TRGT)objdump
SZ = $(TRGT)size
HEX = $(CP) -O ihex
BIN = $(CP) -O binary

AOPT =
TOPT = -mthumb -DTHUMB
CWARN = -Wall -Wextra -Wundef -Wstrict-prototypes
CPPWARN = -Wall -Wextra -Wundef
UDEFS =
UADEFS =
UINCDIR =
ULIBDIR =
ULIBS =

RULESPATH = $(CHIBIOS)/os/common/startup/ARMCMx/compilers/GCC/mk
include $(RULESPATH)/rules.mk
