# Makefile

TOP=..

include $(TOP)/configure/CONFIG

TARGETS = $(CONFIG_TARGETS)
CONFIGS += $(subst ../,,$(wildcard $(CONFIG_INSTALLS)))

CFG += CONFIG_MODULE
EXPAND += CONFIG_MODULE@
EXPAND_VARS += MODULE=$(MODULE)

include $(TOP)/configure/RULES

