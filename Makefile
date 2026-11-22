CONTIKI_PROJECT = sensor-node sink-node
PROJECT_SOURCEFILES += energy.c
all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RIME = 1
CONTIKI = ../..
include $(CONTIKI)/Makefile.include
