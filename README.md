# AVRNetStack

This project is the start of a fully featured TCP/IP stack for Atmel AVR. There are a number of projects in the wings that require full TCP/IP support on an AVR. There has been some great work in small TCP stacks that use single packet tcp, etc., but for the projects at hand I needed support of full TCP features. 

Project Goals
* Full Featured TCP (support retransmit, etc)
* Network stack with ICMP, ARP, etc
* DHCP Support
* Minimum Footprint


Memory Footprint
      Date                             Comments       Ram      Flash     Eth   Arp   DHCP   ICMP   TCP   UDP   IPR  Tests 
|-------------|-------------------------------------|-------|----------|-----|-----|------|------|-----|-----|-----|------|
 12/16/2015   | Packet RX handler moved it Int25    |  1535 |     4340 |  x  |  x  |      |   x  |  -  |     |  x  |    0 |
