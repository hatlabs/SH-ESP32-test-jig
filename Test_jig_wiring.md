# Test jig wiring

| Controller | Device Under Test |
| ---------- | ----------------- |
| GPIO 12    | SCL |
| GPIO 14    | SDA |
| Opto IN    | Opto OUT |
| Opto OUT   | Opto IN |
| DQ         | DQ |

| DUT        | DUT |
| ---------- | --- |
| Vin Prot - | Opto GND |
| Vin Prot + | Opto Vcc |
| Vin +      | CAN 12V |
| Vin -      | CAN GND |
