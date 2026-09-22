// Packets.h - C89-compliant packet framing over CAN
#ifndef PACKETS_H
#define PACKETS_H

#include <stddef.h>
#include <stdio.h>

//#include "Packets.h"
#include "mc9s12a128.h"
#include "Interrupts.h"
#include "mcohw.h"    // For MCOHW_PushMessage and CAN_MSG
#include "Subroutines.h"  // For OUT_digi_0 and gProcImg extern
#include "mco.h" // for TRUE and FALSE macros


#define MASTER_NODE_ID 0x201 // Node ID for the master node (C48)

// Base CAN ID for RPDO1 messages
#define RPDO1_BASE_ID         0x200U // Using 'U' for hex literals is good practice for clarity

// Control byte mask and flags (after shift)
// The control information is in the upper 6 bits of the first data byte of a CAN message.
// Bits 7-2 (MSB being bit 7) of the raw control byte.
#define PACKET_CTRL_MASK      0xFCU    // Mask to isolate bits 7-2 (11111100)
#define PACKET_CTRL_SHIFT     2        // Shift right by 2 to move these 6 bits to positions 5-0
// After shifting, the PACKET_LAST_FLAG is at bit 5 of the resulting 6-bit value.
#define PACKET_LAST_FLAG      0x20U    // Bit 5 (00100000) set indicates this is the last packet in a sequence

// Sizes
// PACKET_DATA_BYTES defines the number of bytes copied from the CAN frame's data payload
// *after* the control byte. This includes an assumed Node ID byte plus actual user data.
#define PACKET_DATA_BYTES     7U       // Assumed to be nodeID (1 byte) + up to 6 data bytes
// PACKET_PAYLOAD_BYTES defines the maximum user data bytes per packet,
// excluding the control byte and the assumed Node ID.
#define PACKET_PAYLOAD_BYTES  6U       // User data bytes per packet (max 6)



// emulating c99 fixed widths ints in c89
typedef signed   char        int8_t;
typedef unsigned char        uint8_t;
typedef signed   short       int16_t;
typedef unsigned short       uint16_t;
typedef signed   long        int32_t;
typedef unsigned long        uint32_t;


// assemble incoming CAN packets; returns total bytes if complete, 0 otherwise
int receivePackets(uint8_t *rcvData, 
					size_t bufSize);

// send one CAN packet: destNodeID = RPDO1_BASE_ID + nodeID
void sendPacket(int dest_nodeID, 
				int packetNumber, 
				char packetData[], 
				int length);   // Length of data in packetData

// split user data and send across multiple packets
void sendPackets(char data[], 
				int length);

int MCO_ProcessStack_Menu(void);

extern char Gen_Flags;
#endif // PACKETS_H