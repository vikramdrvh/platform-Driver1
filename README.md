# Linux Platform Driver

This repository contains a custom Linux platform driver designed to interface with specific hardware peripherals by binding them through the Linux Device Tree.
It serves as a practical implementation of kernel-space driver development, demonstrating how the kernel discovers, probes, 
and manages hardware devices that are not dynamically discoverable on buses like USB or PCI.

## Core Features
* **Device Tree Integration:** Includes `DT_changes.txt`, which outlines the specific Device Tree (DTS) node configurations and phandles required to successfully map the hardware and bind it to the driver.
* **Platform Driver Logic:** The `main.c` file implements the core driver structure, including the platform `probe` and `remove` routines, `of_match_table` device matching, and resource allocation.
* **Loadable Kernel Module (LKM):** Designed to be dynamically loaded into the kernel (a pre-compiled `main.ko` is included), allowing for rapid testing and debugging without needing to rebuild the entire kernel tree.
