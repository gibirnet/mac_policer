# MAC Policer

## Overview

The **MAC Policer** project is designed to enable or disable policing (limiting) of incoming and outgoing packets based on their MAC address. This tool allows you to set a bandwidth limit for specific MAC addresses, providing control over network traffic.

## Features

- Enable or disable MAC-based policing
- Add MAC addresses with specific bandwidth limits

## Usage

### Enabling/Disabling the MAC Policer

You can enable or disable the MAC policer using the following command:

```bash
mac_policer enable-disable
mac_policer enable-disable disable
mac_policer add mac b4de3113cdeb limit 100000
`mac_policer add mac <MAC_ADDRESS> limit <LIMIT_IN_BYTES_PER_SECOND>`
```


## Caveats
-   **Not Production Ready:** This project is still in the development phase and is not yet suitable for production environments.
-   **Packet Handoff System:** The packet handoff between workers has not been implemented yet, so depends of the worker count, limits are not sharp.
-   **MAC Rule Deletion:** Currently, there is no functionality to remove a MAC rule once it has been added.

# Credits

https://ix.gibir.net.tr/en