# MeshFlow v0.1.2

This M9 release adds a high-contrast light theme and improves conversation navigation. Light mode uses white backgrounds, dark text, and a black selection with a yellow border. The node list keeps text within its rows.

Chats open with the most recent messages and load older messages in smaller pages. In a conversation, Up from the composer enters message history. Press Left for message selection or Right for stepped scrolling; Up and Down then perform the selected action. An envelope or ruler beside the battery indicates the mode. Long-press Center on an eligible sent message for its actions, including Retry. Left and Right remain cursor keys while editing.

The ThinkNode M9 application image is **4,025,568 bytes**, fitting the observed 4,063,232-byte app partition with 37,664 bytes remaining. Its SHA-256 is `64242aa1f7ff00e481a513caad19b55439ecb3c102bc26a75a642faa81b47894`. The production build passed and this image was flashed to my M9. The new indicator uses glyphs confirmed present in the actual M9 font; its appearance and placement still need visual confirmation on the device.

Install `firmware.bin` as an application update using the [installation guide](INSTALLATION.md). The Meshtastic Web Flasher upload workflow has not yet been tested on an M9. Back up important configuration and keep a recovery image available. MeshFlow is experimental firmware for the Elecrow ThinkNode M9.
