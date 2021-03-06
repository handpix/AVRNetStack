# AVRNetStack

This project is the start of a fully featured TCP/IP stack for Atmel AVR. There are a number of projects in the wings that require full TCP/IP support on an AVR. There has been some great work in small TCP stacks that use single packet tcp, etc., but for the projects at hand I needed support of full TCP features. 

Project Goals
* Full Featured TCP (support retransmit, etc)
* Network stack with ICMP, ARP, etc
* DHCP Support
* Minimum Memory Footprint



|  Date       |  Ram  |  Flash  |   Eth |  Arp |  DHCP | ICMP |  TCP | UDP | DNS | IPR | Syslog |
|-------------|-------|---------|-------|------|-------|------|------|-----|-----|-----|--------|
| 12/16/2015  |  1535 |    4340 | RX/TX |  RX  |       |  RX  |      |     |     |     |        |
| 12/17/2015  |  1539 |    4098 | RX/TX |  RX  |       |  RX  |      | TX  |     |     |   TX   |
| 12/18/2015  |  1649 |    6640 | RX/TX |  RX  |       |  RX  |      | TX  |     |     |   TX   |
| 12/19/2015  |  1703 |    6930 | RX/TX |  RX  |       |  RX  |      | TX  |     |     |   TX   |
| 12/20/2015  |  1703 |    7008 | RX/TX |  RX  |       |  RX  |      | TX  |     |     |   TX   |

