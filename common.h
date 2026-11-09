#ifndef COMMON_H
#define COMMON_H
#include <stdint.h>
#define CH_ALARM   129   /* unicast: sensor -> sink */
#define CH_CONFIG  130   /* broadcast: sink -> sensors */
#define SINK_ID    1
#define DEFAULT_THRESHOLD 30
#define MSG_TEMP      1
#define MSG_PRESENCE  2
typedef struct { uint8_t type; int16_t temp; uint8_t presence; } alarm_msg_t;
typedef struct { int16_t threshold; uint8_t alarms_enabled; } config_msg_t;
#endif
