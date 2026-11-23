# Monitoring and Alarm System (Contiki / Cooja)

Group X: *(names + numbers)*

Sensor nodes check temperature and presence in a room and alert a sink node.
Runs in Cooja (Sky motes), Contiki 3.0, Instant Contiki VM.

## Files

- `sensor-node.c` - room node
- `sink-node.c` - sink node (serial console)
- `common.h` - shared definitions (sink ID, channels, thresholds, periods)
- `energy.c` / `energy.h` - prints CPU and radio duty cycle (Energest)
- `Makefile` - build file
- `*.csc` - saved Cooja simulation(s)

## Run it

1. Put the folder here (it must be exactly here):
   ```
   ~/Desktop/contiki/examples/monitoring/
   ```
2. Open a terminal and start Cooja:
   ```
   cd ~/Desktop/contiki/tools/cooja
   ant run
   ```
3. In Cooja: **File -> Open simulation** -> pick the `.csc` file from this folder.
4. Press **Start**. Open **Tools -> Mote output**.
5. To see the LEDs: in the Network window, **View -> LEDs** (and **View -> Mote IDs**).

### No `.csc`? Build the simulation by hand

1. **File -> New simulation**.
2. **Motes -> Add motes -> Create new mote type -> Sky mote**.
   Pick `sink-node.c`, Compile, Create, add **1** node.
   **Add the sink first** so it gets ID 1 (`SINK_ID` in `common.h`).
3. Same again with `sensor-node.c`, add **3** nodes.
4. Place the motes close together, then Start.
5. Save it: **File -> Save simulation as** -> put the `.csc` in this folder.

If the sink is not ID 1, it prints a warning at boot, and the sensors' alarms
get lost. Either rebuild the simulation sink-first, or change `SINK_ID` in
`common.h` and recompile.

## Compile (optional check)

```
cd ~/Desktop/contiki/examples/monitoring
make TARGET=sky sensor-node.sky sink-node.sky
```

Warnings about `sht11` and `cc2420` are normal. After changing files, run
`make clean TARGET=sky` first.

## What you should see

- Green LED blinks every second on every node.
- Temperature ramps 20-40 C. Above 30 C: red LED on, alarm sent to the sink.
- Right-click a sensor -> **Click button on Sky N**: presence toggles, yellow LED toggles, sink is notified.
- The sink prints `ALARM ...` for each notification and keeps the last 5.
- Every 60 s each sensor prints an `energy` line (CPU and radio duty cycle).

## Sink console

Right-click the sink -> **Mote tools -> Mote Interface Viewer -> Serial port**, then type:

| Command | Does |
|---|---|
| `list` | last 5 notifications |
| `thr 25` | set temperature threshold (-40 to 125) |
| `alarm on` / `alarm off` | enable / disable alarms |
| `status` | show threshold, alarm state, stored notifications |
| `energy` | CPU and radio duty cycle of the sink |

## How it works (short)

- Alarms go sensor -> sink with `runicast` (acknowledged, up to 4 retransmissions).
  If one still fails, the sensor prints `alarm LOST`.
- The sink broadcasts the config (threshold, alarms on/off) when you change it,
  and again every 30 s so a missed packet or a late-booting node catches up.
- A node that is still hot sends its alarm again when alarms are switched back on.
- Temperature and presence are emulated (ramp and button), as Cooja has no real room.

## Test status

- [x] Alarms reach the sink with a working sink (ID 1), no `alarm LOST`
- [x] Energy lines printed every 60 s
- [x] Green LED blinks every second
- [x] Red LED on above the threshold
- [ ] Presence button toggles yellow LED and notifies the sink
- [x] `list` shows the last 5 (oldest first)
- [ ] `thr 25` reaches all sensors; bad values (`thr abc`, `thr 500`) are rejected --> not for letters
- [x] `alarm off` / `alarm on` work (alarm resent if still hot)
- [x] `status` and `energy` print correctly
