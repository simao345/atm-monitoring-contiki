CONTIKI_PROJECT = sensor-node sink-node
all: $(CONTIKI_PROJECT)

CONTIKI_WITH_RIME = 1
CONTIKI = ../..
include $(CONTIKI)/Makefile.include
