# Monitoring and Alarm System (Contiki / Cooja)

Sensor nodes check temperature and presence in a room and alert a sink node.
Runs in Cooja (Sky motes), Contiki 3.0, Instant Contiki VM.

## Files

- `sensor-node.c` - room node
- `sink-node.c` - sink node (serial console)
- `common.h` - shared definitions
- `Makefile` - build file
- `*.csc` - saved Cooja simulation

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
3. In Cooja: **File -> Open simulation** -> pick the `.csc` file.
4. Press **Start**. Open **Tools -> Mote output**.

 Build it by hand: (in the future i think we can make a  `.csc` file or smth)

1. **File -> New simulation**.
2. **Motes -> Add motes -> Create new mote type -> Sky mote**.
   Pick `sink-node.c`, Compile, Create, add **1** node (sink first, so it gets ID 1).
3. Same again with `sensor-node.c`, add **3** nodes.
4. Place the motes close together, then Start.

## Compile (optional check)

```
cd ~/Desktop/contiki/examples/monitoring
make TARGET=sky sensor-node.sky sink-node.sky
```

## What you should see

- Green LED blinks every second on every node. (need to test)
- Temperature ramps 20-40 C. Above 30 C: red LED on, alarm sent to the sink. (need to test)
- Right-click a sensor -> **Click button on Sky N**: presence toggles, yellow LED toggles, sink is notified. (need to test)
- The sink prints `ALARM ...` for each notification and keeps the last 5. (need to test)

## Sink console

Right-click the sink -> **Mote tools -> Mote Interface Viewer -> Serial port**, then type:

| Command | Does |
|---|---|
| `list` | last 5 notifications | (need to test)
| `thr 25` | set temperature threshold | (need to test)
| `alarm on` / `alarm off` | enable / disable alarms | (need to test)
| `status` | show threshold and alarm state | (need to test)
