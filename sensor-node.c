#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include <stdio.h>
#include <string.h>
#include "common.h"

static int16_t threshold = DEFAULT_THRESHOLD;
static uint8_t alarms_enabled = 1;
static int16_t temperature;
static uint8_t presence = 0;
static uint8_t temp_high = 0;
static struct unicast_conn uc;
static struct broadcast_conn bc;

static void send_alarm(uint8_t type)
{
  alarm_msg_t m;
  linkaddr_t sink;
  if(!alarms_enabled) return;
  m.type = type; m.temp = temperature; m.presence = presence;
  sink.u8[0] = SINK_ID; sink.u8[1] = 0;
  packetbuf_copyfrom(&m, sizeof(m));
  unicast_send(&uc, &sink);
}

static void bc_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  config_msg_t cfg;
  if(packetbuf_datalen() != sizeof(cfg)) return;
  memcpy(&cfg, packetbuf_dataptr(), sizeof(cfg));
  threshold = cfg.threshold;
  alarms_enabled = cfg.alarms_enabled;
  printf("Node %d: config thr=%d alarms=%d\n",
         linkaddr_node_addr.u8[0], threshold, alarms_enabled);
}
static const struct broadcast_callbacks bc_cb = { bc_recv };
static const struct unicast_callbacks uc_cb = { NULL };

PROCESS(led_process, "Activity LED");
PROCESS(monitor_process, "Monitor");
AUTOSTART_PROCESSES(&led_process, &monitor_process);

PROCESS_THREAD(led_process, ev, data)
{
  static struct etimer et;
  PROCESS_BEGIN();
  etimer_set(&et, CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(etimer_expired(&et));
    leds_toggle(LEDS_GREEN);
    etimer_reset(&et);
  }
  PROCESS_END();
}

PROCESS_THREAD(monitor_process, ev, data)
{
  static struct etimer et;
  static int8_t dir = 1;
  PROCESS_BEGIN();
  unicast_open(&uc, CH_ALARM, &uc_cb);
  broadcast_open(&bc, CH_CONFIG, &bc_cb);
  SENSORS_ACTIVATE(button_sensor);
  temperature = 20 + (linkaddr_node_addr.u8[0] % 10);  /* emulated sensor */
  etimer_set(&et, CLOCK_SECOND);
  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == sensors_event && data == &button_sensor) {
      presence = !presence;               /* button = people presence */
      if(presence) leds_on(LEDS_YELLOW); else leds_off(LEDS_YELLOW);
      printf("Node %d: presence=%d\n", linkaddr_node_addr.u8[0], presence);
      send_alarm(MSG_PRESENCE);
    } else if(etimer_expired(&et)) {
      temperature += dir;                 /* ramp 20..40 C, 1 C/s */
      if(temperature >= 40) dir = -1;
      if(temperature <= 20) dir = 1;
      if(temperature > threshold) {
        if(!temp_high) {
          temp_high = 1;
          leds_on(LEDS_RED);
          printf("Node %d: HIGH TEMP %d\n", linkaddr_node_addr.u8[0], temperature);
          send_alarm(MSG_TEMP);
        }
      } else if(temp_high) {
        temp_high = 0;
        leds_off(LEDS_RED);
      }
      etimer_reset(&et);
    }
  }
  PROCESS_END();
}
