#include "contiki.h"
#include "net/rime/rime.h"
#include "dev/leds.h"
#include "dev/serial-line.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"

#define HISTORY 5
typedef struct { uint8_t node, type, presence; int16_t temp; unsigned long t; } entry_t;
static entry_t history[HISTORY];
static uint8_t count = 0, head = 0;
static int16_t threshold = DEFAULT_THRESHOLD;
static uint8_t alarms_enabled = 1;
static struct unicast_conn uc;
static struct broadcast_conn bc;

static void print_entry(const entry_t *e)
{
  printf("[%lus] node %d: %s temp=%d presence=%d\n", e->t, e->node,
         e->type == MSG_TEMP ? "HIGH TEMPERATURE" : "PRESENCE CHANGE",
         e->temp, e->presence);
}

static void uc_recv(struct unicast_conn *c, const linkaddr_t *from)
{
  alarm_msg_t m;
  entry_t *e;
  if(packetbuf_datalen() != sizeof(m)) return;
  memcpy(&m, packetbuf_dataptr(), sizeof(m));
  e = &history[head];
  e->node = from->u8[0]; e->type = m.type; e->temp = m.temp;
  e->presence = m.presence; e->t = clock_seconds();
  head = (head + 1) % HISTORY;
  if(count < HISTORY) count++;
  printf("ALARM "); print_entry(e);
}
static const struct unicast_callbacks uc_cb = { uc_recv };
static const struct broadcast_callbacks bc_cb = { NULL };

static void send_config(void)
{
  config_msg_t cfg;
  cfg.threshold = threshold; cfg.alarms_enabled = alarms_enabled;
  packetbuf_copyfrom(&cfg, sizeof(cfg));
  broadcast_send(&bc);
}

static void list_history(void)
{
  uint8_t i, idx = (head + HISTORY - count) % HISTORY;
  printf("--- last %d notifications (oldest first) ---\n", count);
  for(i = 0; i < count; i++) {
    print_entry(&history[idx]);
    idx = (idx + 1) % HISTORY;
  }
}

static void handle_cmd(char *cmd)
{
  if(strcmp(cmd, "list") == 0) {
    list_history();
  } else if(strncmp(cmd, "thr ", 4) == 0) {
    threshold = atoi(cmd + 4);
    send_config();
    printf("threshold=%d\n", threshold);
  } else if(strcmp(cmd, "alarm on") == 0) {
    alarms_enabled = 1; send_config(); printf("alarms ON\n");
  } else if(strcmp(cmd, "alarm off") == 0) {
    alarms_enabled = 0; send_config(); printf("alarms OFF\n");
  } else if(strcmp(cmd, "status") == 0) {
    printf("threshold=%d alarms=%d\n", threshold, alarms_enabled);
  } else {
    printf("commands: list | thr <n> | alarm on | alarm off | status\n");
  }
}

PROCESS(led_process, "Activity LED");
PROCESS(console_process, "Console");
AUTOSTART_PROCESSES(&led_process, &console_process);

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
  unicast_open(&uc, CH_ALARM, &uc_cb);
  broadcast_open(&bc, CH_CONFIG, &bc_cb);
  printf("Sink ready. commands: list | thr <n> | alarm on | alarm off | status\n");
  while(1) {
    PROCESS_WAIT_EVENT_UNTIL(ev == serial_line_event_message);
    handle_cmd((char *)data);
  }
  PROCESS_END();
}
