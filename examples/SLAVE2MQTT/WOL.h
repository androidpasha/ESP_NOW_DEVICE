#include <WiFiUdp.h>
#include <WakeOnLan.h> // скачать ардуино библиотеку от a7md0

void wakeMyPC()
{
  WiFiUDP UDP;
  WakeOnLan WOL(UDP);
  WOL.setRepeat(3, 100);                                            // Optional, repeat the packet three times with 100ms between. WARNING delay() is used between send packet function.
  WOL.calculateBroadcastAddress(WiFi.localIP(), WiFi.subnetMask()); // Optional  => To calculate the broadcast address, otherwise 255.255.255.255 is used (which is denied in some networks).
  WOL.sendMagicPacket(settings.MACAddress);                         // Send Wake On Lan packet with the above MAC address. Default to port 9.
}
