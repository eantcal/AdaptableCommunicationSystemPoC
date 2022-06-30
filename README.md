# AdaptableCommunicationSystemPoC
AdaptableCommunicationSystem PoC Implementation

## Abstract
Communication requirements of railway operators for different types of services, including voice, video, railway signalling, data transmission are growing fast. As such requirements on railway communications evolve, there is a need to move towards new telecommunication technologies. The European Railways Train Management System (ERTMS) utilizes the Global System for Mobile Communications - Railway (GSM-R) as the legacy telecommunication network connecting trains with the ground. Such technology is to be substituted since it cannot satisfy an increasing demand of connectivity for new railway applications. 

Shift2Rail partners have agreed on adopting the Adaptable Communication System (ACS) as the reference system architecture for implementing the successor of the GSM-R.

This software implements ACS emulator as a proof-of-concept. The core part of it is a Tunnel Manager which can establish pseudo-virtual circuits through multi-bearer tunnels, forcing datagrams on a service-basis to follow specific paths between gateways (i.e., from on-board to a train to the network-side rail control center and vice versa). 

The Tunnel Manager can properly select a given tunnel/bearer for sending messages (and duplicating them on redundant paths) of critical rail applications for train traffic management, relying on tunnels based on either connection-oriented protocol (i.e., the Transport Control Protocol, TCP), connectionless protocol (i.e., the User Datagram Protocol, UDP) or a mix of them. 

This software has been written to adopt the SIP protocol (a SIP proxy/dedicated server plays this role) and to manage the tunnels via the Tunnel Manager. 

It relies on Linux Tunnel Addressing Protocol (TUN/TAP) module to create virtual network devices used to represent the endpoint interfaces of multi-path tunnels.

[ACS Emulator Software architecture scheme](https://www.mdpi.com/applsci/applsci-12-03013/article_deploy/html/images/applsci-12-03013-g009.png)

## Reference
You can find more information about ACS at https://www.mdpi.com/2076-3417/12/6/3013 and https://ieeexplore.ieee.org/document/9662904.

