# FirmRobut
firmware for NodeMCU motor controller based robuts

#Protocol
This firmware uses a simple ASCII based udp protocol.
Just send a packet containing 
```
[motor_a_value, motor_b_value]
```
where the motor values are strings that represent a integer between -1000 and 1000
