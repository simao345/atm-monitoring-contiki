/*
 * Sink node: receives alarms, keeps the last 5, serial console,
 * broadcasts threshold / alarm-enable configuration to the sensors.
 */
#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "energy.h"

#define HISTORY 5

typedef struct {
  uint8_t node, type, presence;
  int16_t temp;
  unsigned long t;
} entry_t;

static entry_t history[HISTORY];     /* ring buffer: last 5 notifications */
static uint8_t count = 0, head = 0;
static int16_t threshold = DEFAULT_THRESHOLD;
static uint8_t alarms_enabled = 1;
static struct runicast_conn rc;
static struct broadcast_conn bc;
static struct etimer cfg_timer;

PROCESS(led_process, "Activity LED");
PROCESS(console_process, "Console");
AUTOSTART_PROCESSES(&led_process, &console_process);

/* ---- alarms ----------------------------------------------------------- */

static void
print_entry(const entry_t *e)
{
  printf("[%lus] node %d: %s temp=%d presence=%d\n", e->t, e->node,
         e->type == MSG_TEMP ? "HIGH TEMPERATURE" : "PRESENCE CHANGE",
         e->temp, e->presence);
}

static void
rc_recv(struct runicast_conn *c, const linkaddr_t *from, uint8_t seqno)
{
  alarm_msg_t m;
  entry_t *e;

  if(packetbuf_datalen() != sizeof(m)) {
    return;
  }
  memcpy(&m, packetbuf_dataptr(), sizeof(m));
  e = &history[head];
  e->node = from->u8[0];
  e->type = m.type;
  e->temp = m.temp;
  e->presence = m.presence;
  e->t = clock_seconds();
  head = (head + 1) % HISTORY;
  if(count < HISTORY) {
    count++;
  }
  printf("ALARM ");
  print_entry(e);
}

static const struct runicast_callbacks rc_cb = { rc_recv, NULL, NULL };
static const struct broadcast_callbacks bc_cb = { NULL, NULL };

/* ---- configuration ---------------------------------------------------- */

static void
send_config(void)
{
  config_msg_t cfg;

  cfg.threshold = threshold;
  cfg.alarms_enabled = alarms_enabled;
  packetbuf_copyfrom(&cfg, sizeof(cfg));
  broadcast_send(&bc);
}

static void
config_changed(void)
{
  send_config();                                     /* tell sensors now   */
  etimer_set(&cfg_timer, CONFIG_PERIOD * CLOCK_SECOND); /* then every 30 s */
}

/* ---- console ---------------------------------------------------------- */

static void
list_history(void)
{
  uint8_t i, idx = (head + HISTORY - count) % HISTORY;

  printf("--- last %d notifications (oldest first) ---\n", count);
  for(i = 0; i < count; i++) {
    print_entry(&history[idx]);
    idx = (idx + 1) % HISTORY;
  }
}

static int
parse_int(const char *s, long *out)
{
  char *end;
  long v = strtol(s, &end, 10);

  if(end == s || *end != '\0') {
    return 0;
  }
  *out = v;
  return 1;
}

static void
handle_cmd(char *cmd)
{
  long v;

  if(strcmp(cmd, "list") == 0) {
    list_history();
  } else if(strncmp(cmd, "thr ", 4) == 0) {
    if(!parse_int(cmd + 4, &v) || v < THR_MIN || v > THR_MAX) {
      printf("usage: thr <%d..%d>\n", THR_MIN, THR_MAX);
    } else {
      threshold = (int16_t)v;
      config_changed();
      printf("threshold=%d\n", threshold);
    }
  } else if(strcmp(cmd, "alarm on") == 0) {
    alarms_enabled = 1;
    config_changed();
    printf("alarms ON\n");
  } else if(strcmp(cmd, "alarm off") == 0) {
    alarms_enabled = 0;
    config_changed();
    printf("alarms OFF\n");
  } else if(strcmp(cmd, "status") == 0) {
    printf("threshold=%d alarms=%d stored=%d\n", threshold, alarms_enabled, count);
  } else if(strcmp(cmd, "energy") == 0) {
    energy_print();
  } else {
    printf("commands: list | thr <n> | alarm on | alarm off | status | energy\n");
  }
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

PROCESS_THREAD(console_process, ev, data)
{
  PROCESS_BEGIN();

  runicast_open(&rc, CH_ALARM, &rc_cb);
  broadcast_open(&bc, CH_CONFIG, &bc_cb);
  if(linkaddr_node_addr.u8[0] != SINK_ID) {
    printf("WARNING: sink has ID %d but sensors send to ID %d. Add the sink FIRST in Cooja.\n",
           linkaddr_node_addr.u8[0], SINK_ID);
  }
  etimer_set(&cfg_timer, CONFIG_PERIOD * CLOCK_SECOND);
  printf("Sink ready. commands: list | thr <n> | alarm on | alarm off | status | energy\n");

  while(1) {
    PROCESS_WAIT_EVENT();
    if(ev == serial_line_event_message) {
      handle_cmd((char *)data);
    } else if(etimer_expired(&cfg_timer)) {
      send_config();                 /* periodic: covers lost packets and late boots */
      etimer_reset(&cfg_timer);
    }
  }

  PROCESS_END();
}
