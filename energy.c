#include "contiki.h"
#include "sys/energest.h"
#include "net/linkaddr.h"
#include <stdio.h>
#include "energy.h"

void
energy_print(void)
{
#if ENERGEST_CONF_ON
  unsigned long cpu, lpm, tx, rx, total, unit;

  energest_flush();
  cpu = energest_type_time(ENERGEST_TYPE_CPU);
  lpm = energest_type_time(ENERGEST_TYPE_LPM);
  tx = energest_type_time(ENERGEST_TYPE_TRANSMIT);
  rx = energest_type_time(ENERGEST_TYPE_LISTEN);
  total = cpu + lpm;
  unit = total / 1000UL;            /* ticks per 0.1 % */
  if(unit == 0) {
    return;
  }
  printf("Node %d energy: cpu %lu.%lu%% radio %lu.%lu%% (tx %lu.%lu%% rx %lu.%lu%%) over %lus\n",
         linkaddr_node_addr.u8[0],
         (cpu / unit) / 10, (cpu / unit) % 10,
         ((tx + rx) / unit) / 10, ((tx + rx) / unit) % 10,
         (tx / unit) / 10, (tx / unit) % 10,
         (rx / unit) / 10, (rx / unit) % 10,
         total / RTIMER_SECOND);
#else
  printf("Energest not enabled on this platform\n");
#endif
}
