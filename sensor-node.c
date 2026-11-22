/*
 * Room node: emulated temperature sensor, presence button, 3 LEDs,
 * reliable alarms to the sink, configuration received from the sink.
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/button-sensor.h"
#include <stdio.h>
#include <string.h>
#include "common.h"
#include "energy.h"

#define PEND_TEMP 0x01
#define PEND_PRES 0x02

static int16_t threshold = DEFAULT_THRESHOLD;
static uint8_t alarms_enabled = 1;
static int16_t temperature;
static uint8_t presence = 0;
static uint8_t temp_high = 0;
static uint8_t pending = 0;          /* alarms waiting to be sent */
static struct runicast_conn rc;
static struct broadcast_conn bc;

PROCESS(led_process, "Activity LED");
PROCESS(monitor_process, "Monitor");
AUTOSTART_PROCESSES(&led_process, &monitor_process);

/* ---- network callbacks ------------------------------------------------ */

static void
bc_recv(struct broadcast_conn *c, const linkaddr_t *from)
{
  config_msg_t cfg;

  if(from->u8[0] != SINK_ID || from->u8[1] != 0) {
    return;
  }
  if(packetbuf_datalen() != sizeof(cfg)) {
    return;
  }
  memcpy(&cfg, packetbuf_dataptr(), sizeof(cfg));
  if(cfg.threshold < THR_MIN || cfg.threshold > THR_MAX) {
    return;
  }
  if(cfg.threshold != threshold || cfg.alarms_enabled != alarms_enabled) {
    printf("Node %d: config thr=%d alarms=%d\n",
           linkaddr_node_addr.u8[0], cfg.threshold, cfg.alarms_enabled);
  }
  if(!cfg.alarms_enabled) {
    pending = 0;
  } else if(!alarms_enabled && temp_high) {
    pending |= PEND_TEMP;            /* alarms just re-enabled while hot */
  }
  threshold = cfg.threshold;
  alarms_enabled = cfg.alarms_enabled;
  process_poll(&monitor_process);
}

static void
rc_sent(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  process_poll(&monitor_process);    /* channel free: send next alarm */
}

static void
rc_timedout(struct runicast_conn *c, const linkaddr_t *to, uint8_t retransmissions)
{
  printf("Node %d: alarm LOST (no ack after %d retransmissions)\n",
         linkaddr_node_addr.u8[0], retransmissions);
  process_poll(&monitor_process);
}

static const struct broadcast_callbacks bc_cb = { bc_recv, NULL };
static const struct runicast_callbacks rc_cb = { NULL, rc_sent, rc_timedout };

/* ---- alarm sending ---------------------------------------------------- */

static void
try_send(void)
{
  alarm_msg_t m;
  linkaddr_t sink;

  if(!alarms_enabled) {
    pending = 0;
    return;
  }
  if(pending == 0 || runicast_is_transmitting(&rc)) {
    return;
  }
  if(pending & PEND_TEMP) {
    m.type = MSG_TEMP;
    pending &= ~PEND_TEMP;
  } else {
    m.type = MSG_PRESENCE;
    pending &= ~PEND_PRES;
  }
  m.temp = temperature;
  m.presence = presence;
  sink.u8[0] = SINK_ID;
  sink.u8[1] = 0;
  packetbuf_copyfrom(&m, sizeof(m));
  runicast_send(&rc, &sink, MAX_RETRANSMISSIONS);
}

/* ---- processes -------------------------------------------------------- */

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
  static uint8_t secs = 0;

  PROCESS_BEGIN();

  runicast_open(&rc, CH_ALARM, &rc_cb);
  broadcast_open(&bc, CH_CONFIG, &bc_cb);
  SENSORS_ACTIVATE(button_sensor);

  if(linkaddr_node_addr.u8[0] == SINK_ID) {
    printf("WARNING: this sensor has ID %d, which is reserved for the sink\n",
           SINK_ID);
  }
  temperature = 20 + (linkaddr_node_addr.u8[0] % 10);   /* emulated sensor */
  etimer_set(&et, CLOCK_SECOND);

  while(1) {
    PROCESS_WAIT_EVENT();

    if(ev == sensors_event && data == &button_sensor) {
      /* button = people presence: toggles the lights (yellow LED) */
      presence = !presence;
      if(presence) {
        leds_on(LEDS_YELLOW);
      } else {
        leds_off(LEDS_YELLOW);
      }
      printf("Node %d: presence=%d\n", linkaddr_node_addr.u8[0], presence);
      pending |= PEND_PRES;

    } else if(etimer_expired(&et)) {
      /* temperature ramps 20..40 C, 1 C per second */
      temperature += dir;
      if(temperature >= 40) {
        dir = -1;
      }
      if(temperature <= 20) {
        dir = 1;
      }
      if(temperature > threshold) {
        if(!temp_high) {
          temp_high = 1;
          leds_on(LEDS_RED);
          printf("Node %d: HIGH TEMP %d\n", linkaddr_node_addr.u8[0], temperature);
          pending |= PEND_TEMP;
        }
      } else if(temp_high) {
        temp_high = 0;
        leds_off(LEDS_RED);
      }
      if(++secs >= ENERGY_PERIOD) {
        secs = 0;
        energy_print();
      }
      etimer_reset(&et);
    }

    try_send();   /* also runs on poll events from the network callbacks */
  }

  PROCESS_END();
}
