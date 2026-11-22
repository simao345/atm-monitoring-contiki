#ifndef COMMON_H
#define COMMON_H

#include <stdint.h>

#define CH_ALARM            129   /* runicast: sensor -> sink (reliable) */
#define CH_CONFIG           130   /* broadcast: sink -> sensors          */

#define SINK_ID             1     /* the sink MUST be the first mote (ID 1) */
#define DEFAULT_THRESHOLD   30    /* degrees C */
#define THR_MIN             (-40)
#define THR_MAX             125

#define MAX_RETRANSMISSIONS 4     /* runicast retries per alarm */
#define CONFIG_PERIOD       30    /* seconds between config broadcasts */
#define ENERGY_PERIOD       60    /* seconds between energy reports   */

#define MSG_TEMP            1
#define MSG_PRESENCE        2

typedef struct {
  uint8_t type;       /* MSG_TEMP or MSG_PRESENCE */
  int16_t temp;
  uint8_t presence;
} alarm_msg_t;

typedef struct {
  int16_t threshold;
  uint8_t alarms_enabled;
} config_msg_t;

#endif
