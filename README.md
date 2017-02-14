# denon-ipod-dock-protocol
This is a small utility that attempts to decode the communication between a Denon receiver and iPod dock.

For a little write-up, please visit [my blog post](https://svenschwermer.de/2017/02/14/denon-ipod-dock-protocol.html) on the topic.

The four threads that are being started in `main` listen to a total of four serial ports. The first two listen to RX and TX
between the iPod and the dock, the other two listen to the two signal lines between dock and receiver which is why the data
needs to be decoded by XOR'ing all bytes with `0x31`.
