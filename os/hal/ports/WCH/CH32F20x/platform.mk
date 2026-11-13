# Copyright 2026 TuStation
# SPDX-License-Identifier: Apache-2.0

PLATFORMSRC_CONTRIB := \
    $(CHIBIOS)/os/hal/ports/common/ARMCMx/nvic.c \
    $(CHIBIOS_CONTRIB)/os/hal/ports/WCH/CH32F20x/hal_lld.c

PLATFORMINC_CONTRIB := \
    $(CHIBIOS_CONTRIB)/os/hal/ports/WCH/CH32F20x \
    $(CHIBIOS_CONTRIB)/os/common/ext/CMSIS/WCH/CH32F20x \
    $(CHIBIOS)/os/hal/ports/common/ARMCMx

ifeq ($(USE_SMART_BUILD),yes)
ifeq ($(HALCONFDIR),)
  ifeq ($(CONFDIR),)
    HALCONFDIR = .
  else
    HALCONFDIR := $(CONFDIR)
  endif
endif

HALCONF := $(strip $(shell cat $(HALCONFDIR)/halconf.h $(HALCONFDIR)/halconf_community.h | egrep -e "\#define"))
endif

include $(CHIBIOS_CONTRIB)/os/hal/ports/WCH/LLD/GPIOv1/driver.mk
include $(CHIBIOS_CONTRIB)/os/hal/ports/WCH/LLD/SYSTICKv1/driver.mk
include $(CHIBIOS_CONTRIB)/os/hal/ports/WCH/LLD/USBv1/driver.mk

ALLCSRC += $(PLATFORMSRC_CONTRIB)
ALLINC  += $(PLATFORMINC_CONTRIB)
