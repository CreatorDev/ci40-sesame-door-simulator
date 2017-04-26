This implementation uses DC Motor Relay Switch. Below info how to wire things and how to set jumpers.

ci40:
Jumpers JP1 and JP9 in position 2-3. All other jumpers in position 1-2.

DC Motor click:
Motor PWR Jumper in position INT

DC Motor click put in Mikrobus 1

CD tray wiring:
Red Wire -> DC Motor Out 2
Brown Wire -> DS Motor Out 1
Black Wire -> Mikrobus 2 GND
Orange Wire -> Mikrobus 2 INT
Yellow Wire -> Mikrobus 2 PWM

Buttons:
Mikrobus 2 GND -> Button switch 1 -> Mikrobus 2 AN
Mikrobus 2 GND -> Button switch 2 -> Mikrobus 2 RST

Attached photo for better understanding:
![wiring](wiring.jpg)

Button 1 will switch between slow/fast tray open.
Button 2 will trigger start/stop movement of tray.
