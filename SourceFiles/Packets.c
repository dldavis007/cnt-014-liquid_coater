#include "Packets.h"
#include "MenuSerialize.h"

/* Define BOOL for C89 */
typedef unsigned char BOOL;


// external declaration for the process image array
extern UNSIGNED8 gProcImg[];

// external declaration for Timer1
extern unsigned int Timer1;

// test timer, remove before release
unsigned int myTimer = RTI_One_Sec * .003;

// variables for mco_processtack_menu
char rxbuffer[32];
int rxbuffer_len = 0;


// @brief Calculates an 8-bit XOR checksum over a block of data.
// @param data Pointer to the data buffer.
// @param length The number of bytes to include in the checksum calculation.
// @return The calculated 8-bit XOR checksum.


static uint8_t calculate_xor_checksum(const uint8_t *data, size_t length)
{
    uint8_t checksum = 0;
    size_t i;
    for (i = 0; i < length; ++i) {
        checksum ^= data[i];
    }
    return checksum;
}

// int prev_ls_bits;
int receive_error = 0;



int receivePackets(uint8_t *rcvData, size_t bufSize)
{
    // Static state variables maintain context across multiple calls to this function,
    // allowing for the reassembly of messages that span multiple packets.
    static BOOL receiving = 0;    // Flag: 1 if currently in the middle of receiving a multi-packet message.
    static uint8_t nextSeq = 1;       // Expected sequence number for the next packet in a multi-packet message.
    static size_t totalBytes = 0;     // Total bytes accumulated so far for the current multi-packet message.
    static uint8_t prev_ls_bits = 0; // Holds the last two bits of the previous packet number for comparison.

    // Local variables - all declared at the top as required by C89.
    uint8_t rawCtrl;      // The raw control byte read from the CAN message.
    uint8_t packetInfo;   // The 6-bit control information extracted from rawCtrl.
    BOOL isLast;          // Flag: TRUE if the current packet is the last in its sequence.
    uint8_t pkt_num;   // Holds either the packet sequence number or the data byte count in the last packet.
    size_t i;             // Loop counter.
    size_t count;         // Number of bytes to copy from the current packet.
    size_t offset;        // Offset into rcvData buffer for placing data from middle packets.
    size_t copyBytes;     // Number of bytes to copy for middle packets, considering buffer limits.
    int received_len;     // Stores the total length of the successfully received message.
    int received_checksum;
    int calculated_checksum;
    int ls_bits; // Holds the last two bits of pkt_num for the last_sent_lsb calculation.
    int current_bits;
    int prev_bits;
    uint8_t numberField;
    uint8_t sequenceNum;
    uint8_t dataLen;

    

    // Read the control byte from the global hardware receive buffer.
    // OUT_digi_0 is an index to where this specific CAN message's data begins.
    rawCtrl = gProcImg[OUT_digi_0];


    // Clear the control byte in the hardware buffer to mark it as processed.
    // This prevents reprocessing the same packet on subsequent calls.
    gProcImg[OUT_digi_0] = 0;

    // Extract the 6-bit packet control information.
    // PACKET_CTRL_MASK (0xFC) isolates bits 7-2.
    // PACKET_CTRL_SHIFT (2) shifts these bits to positions 5-0.
    // So, packetInfo contains the original bits B7 B6 B5 B4 B3 B2 of rawCtrl as 0 0 B7 B6 B5 B4 B3 B2.
    packetInfo = (uint8_t)((rawCtrl & PACKET_CTRL_MASK) >> PACKET_CTRL_SHIFT);

    // Determine if this is the last packet in a sequence.
    // PACKET_LAST_FLAG (0x20) checks bit 5 of packetInfo (original B7 of rawCtrl).
    isLast = (packetInfo & PACKET_LAST_FLAG) ? 1 : 0;


    // Use a separate variable for the full 5-bit number field
    numberField = (uint8_t)(packetInfo & ~PACKET_LAST_FLAG);
    // Extract the sequence number or byte count.
    // This takes the lower 5 bits of packetInfo (original B6 B5 B4 B3 B2 of rawCtrl).
    // Its meaning depends on the 'isLast' flag. pkt_num in the last packet is calculated 
    // packetNumber = LAST_PKT + packetDataLength in sendPackets 
    // ONLY on the last packet, ptk_num contains the last packet flag and the length of the last packet
    // otherwise, it just contains the packet number of the message you're receiving
    pkt_num = (uint8_t)(packetInfo & ~PACKET_LAST_FLAG); // Clear the 'isLast' bit
    

    if (isLast) {
        // For the LAST packet, the lower 3 bits are the data length.
        dataLen = numberField & 0x07;
        // The sequence number is irrelevant here, or could be inferred from nextSeq
        sequenceNum = nextSeq; 
    } else {
        // For ALL OTHER packets, the full 5-bit field is the sequence number.
        pkt_num = numberField;
        dataLen = 0; // Not used
        prev_ls_bits = numberField & 0x03;
    }
    // ls_bits = numberField & 0x03; // Mask to get the 0st and 1st bits for the last_sent_lsb (would be 2-3 but already shifted down by 2)

    
    
    // If rawCtrl is 0, it might indicate no new packet or an empty control byte.
    // In this protocol, a valid control byte (even for an empty packet) would likely have flags set.
    // So, 0 typically means no actionable packet.
    if (rawCtrl == 0) {
        receive_error++;
        return 0; // No new packet data to process.
    }
    
    
    
    // Main state machine logic for packet assembly
    if (receiving == 0) {
        // Not currently in the middle of receiving a multi-packet message.
        // Expecting either a single-packet message or the first packet of a multi-packet message.

        if (isLast == 1) {
            // This is a single-packet message because receiving is 0 and isLast is 1.
            // pkt_num here represents the number of data bytes in this single packet.
            // Ensure not to copy more bytes than the buffer can hold.
            count = (dataLen > bufSize) ? bufSize : dataLen;
            // Copy data from the hardware buffer (gProcImg) to the destination buffer (rcvData).
            // Data starts at gProcImg[OUT_digi_0 + 1], after the control byte.
            for (i = 0; i < count; ++i) {
                // (uint8_t)i is used to ensure C89 compliance if size_t is larger than uint8_t
                // and the compiler is strict about array indexing types with pointer arithmetic.
                rcvData[i] = gProcImg[OUT_digi_0 + 1 + (uint8_t)i];
            }
            // The last byte of the data is the received checksum.
            received_checksum = rcvData[count - 1];
            // Calculate checksum on the message payload (all bytes except the last one).
            calculated_checksum = calculate_xor_checksum(rcvData, count - 1);

            if (received_checksum == calculated_checksum) {
                // Checksum is valid. Return the length of the payload.
                return (int)(count);
            } else {
                // Checksum failed. Return error.
                receive_error++;
                return -1;
            }
        }
        // Not a single-packet message (because last packet flag not set), so check if it's the start of a multi-packet message.
        // For the first packet of a multi-packet message, pkt_num must be 1.
        else if (pkt_num == 1) {
            // Start of a new multi-packet message.
            receiving = 1;   // Set state to indicate we are now receiving a multi-packet message.
            nextSeq = 2;        // Expect the packet with sequence number 2 next.

            // Determine how many bytes to copy from this first packet.
            // PACKET_DATA_BYTES includes an assumed Node ID and up to 6 payload bytes.
            // This suggests rcvData will store [NodeID_pkt1, payload_pkt1, NodeID_pkt2, payload_pkt2, ...].
            totalBytes = (PACKET_DATA_BYTES > bufSize) ? bufSize : PACKET_DATA_BYTES;
            for (i = 0; i < totalBytes; ++i) {
                rcvData[i] = gProcImg[OUT_digi_0 + 1 + (uint8_t)i];
            }
            // Message is not yet complete, so return 0.
            prev_ls_bits = ls_bits;
        }
        // else: If isLast is 0 and pkt_num is not 1, it's an unexpected packet
        // (e.g., a middle packet received out of sequence when not 'receiving').
        // This packet is ignored, and the function will return 0 at the end.
    } else {
        // Currently in the middle of receiving a multi-packet message (receiving == 1).

        if (isLast == 1) {
            
            current_bits = (rawCtrl & 0x60);
            prev_bits = (prev_ls_bits << 5); // SHIFTING bits at 0-1 to 5-6
            
            // This is the final packet of a multi-packet message.
            // pkt_num here represents the number of actual data bytes in this final packet's payload.
            // Calculate how many bytes can be copied from this packet without overflowing rcvData.
            count = (dataLen > (bufSize - totalBytes)) ? (bufSize - totalBytes) : dataLen;
            // Append data from this final packet to rcvData.
            for (i = 0; i < count; ++i) {
                rcvData[totalBytes + i] = gProcImg[OUT_digi_0 + 1 + (uint8_t)i];
            }
            // Calculate total length of the reassembled message.
            received_len = (int)(totalBytes + count);

            // Reset state machine for the next message.
            receiving = 0;
            nextSeq = 1;
            totalBytes = 0;

            // uses checksum to verify the message integrity
            if (received_len < 1) { // Must have at least a checksum byte.
                receive_error++;
                return 0; 
            }

            // Perform checksum validation.
            received_checksum = rcvData[received_len - 1]; // Last byte is the checksum.
            calculated_checksum = calculate_xor_checksum(rcvData, received_len - 1); // this  calculates checksum on all bytes except the last one

            if (received_checksum == calculated_checksum) {
                // Checksum is valid. Return the length of the payload.
                return (int)(received_len);
            } else {
                // Checksum failed. Return error.
                receive_error++;
                return -1;
            }
        }
        // Not the last packet, so it should be a middle packet.
        // Check if the sequence number matches the expected sequence number.
        else if (pkt_num == nextSeq) {
            // This is an expected middle packet.
            // pkt_num here is the sequence number.
            // Calculate the offset in rcvData where this packet's data should be placed.
            // The offset assumes that each previous packet contributed PACKET_DATA_BYTES.
            offset = (size_t)(pkt_num - 1) * PACKET_DATA_BYTES; // PACKET_DATA_BYTES = 7 

            // Determine how many bytes to copy from this middle packet.
            // It should be PACKET_DATA_BYTES, unless it would overflow the buffer.
            if ((offset + PACKET_DATA_BYTES) > bufSize) {
                copyBytes = (offset < bufSize) ? (bufSize - offset) : 0;
            } else {
                copyBytes = PACKET_DATA_BYTES;
            }

            // Copy data for the middle packet.
            for (i = 0; i < copyBytes; ++i) {
                rcvData[offset + i] = gProcImg[OUT_digi_0 + 1 + (uint8_t)i];
            }
            // Update the total bytes received so far.
            // Note: if copyBytes was less than PACKET_DATA_BYTES due to buffer limit,
            // totalBytes might not accurately reflect full segments if calculation relied on it later.
            // However, for just summing up, this is okay.
            // If data is truncated, the message is effectively corrupted from protocol view.
            if (offset + copyBytes > totalBytes) { // only update if we are actually adding new sequential data
                totalBytes = offset + copyBytes;
            }
            nextSeq++; // Increment expected sequence number.
            prev_ls_bits = ls_bits;
            // Message is not yet complete, so return 0.
        } else {
            // Out-of-order packet or unexpected sequence number.
            // Abort the current message reception and reset the state.
            receiving = 0;
            nextSeq = 1;
            totalBytes = 0;
            receive_error++;
            // Error condition, message incomplete, return 0.
        }
    }
    return 0; // Return 0 if message is still incomplete or an error occurred.
}


// keeps record of the last sent packet number's 5-6 bits (to send in 5-6 bits in header byte only when sending last packet)
int last_sent_lsb = 0;

void sendPacket(int dest_nodeID, int packetNumber, char packetData[], int length) {
	CAN_MSG CAN_Packet_MSG;
    char i;
    
	packetNumber <<= 2; // Shifting left by 2 locations

    // packetNumber |= gProcImg[IN_digi_0] & 0x01; line was removed because not all units update IN_digi_0 when menu is active
    // instead, we check if Gen_Flags_Menu_Active is set, which is set

    // only sets first bit of packetNumber if In_digi_0 is set, meaning that the menu is active
    // this is so that we don't send a packet with the first bit set to 0 because button box will register 
    // it as the menu being inactive. Additionally, active_menu_flag is set when the unit receives the "activ" command
    // from the C48, saying that a node somewhere has an active menu 
    
    // TODO remarked out for testing, might need to be put back in
    // if (Gen_Flags & Gen_Flags_Menu_Active){
    //     packetNumber |= 0x01; // setting first bit to 1 if menu is active, so button box doesn't kick us out of the menu
    // }


    // Assign values to CAN_Packet_MSG members
    CAN_Packet_MSG.ID = dest_nodeID;
    CAN_Packet_MSG.LEN = 8;
    CAN_Packet_MSG.dummy32bit = 0;
    CAN_Packet_MSG.BUF[0] = packetNumber;
	

    for (i = 0; i < 7; i++) {
        CAN_Packet_MSG.BUF[i+1] = i<length ? packetData[i] : 0;
    }
 	if (!MCOHW_PushMessage(&CAN_Packet_MSG))
    {
      // failed to transmit
      MCOUSER_FatalError(0x8801);
    }
}

signed int delayFlag = 1;

void sendPackets(char data[], int length) {

	#define LAST_PKT 0b00100000

    int i,j;

	int packetNumber = 1;
    int dest_nodeID = MASTER_NODE_ID; // was 0x200, made it 0x201 to work backwards compatible with older versions of c48 controller, if 0x200 and c48 controller isn't updated
    // it perceives any message that has the first bit of the packet number set to 0 to turn off the node menu and it doesn't turn back on
    char emptyPacketData[8] = {0};  // Empty packet data
    int emptyPacketDataLength = 0;
	int packetDataLength = 0;
    char packetData[7];
    last_sent_lsb = 0; // Reset last_sent_lsb for each new transmission, making sure that if a single packet is sent, last_sent_lsb is 0
	
    for (i = 0; i < length; i += 6) {
		if (length - i < 7) {
		    // Last packet, set end indicator and add number of bytes
			packetDataLength = (length - i) & 0x07; // masking to get only bits 0-2 (though this should be already happening, this value should never be above 6 anyway)
            
            packetNumber = LAST_PKT + packetDataLength; // adding LAST_PKT (bit 5) to data length (bits 0-2) leaving bits 3-4 for last_sent_lsb
            packetNumber |= last_sent_lsb; // adding last_sent_lsb to the packet number at bits 3-4
		}
		else {
		    // Not the last packet
			packetDataLength = 6;

            // UPDATE last_sent_packet to the current packet number
            last_sent_lsb = ((packetNumber) & 0x03) << 3; // Masking to get the bits 0-1 and shifting to bits 3-4
		}

		packetData[0] = NODE_ID;
        // Copy 6 elements to the packet data
        for (j = 0; j < packetDataLength; j++) {
            packetData[j+1] = data[i + j];
        }

        // resets COP watchdog timer so COP doesn't reset MCU
        ARMCOP = 0x55;
        ARMCOP = 0xAA;

        // Send the packet

        // slowing by 5ms
        Timer1 = myTimer;
        while (Timer1 && delayFlag);


        sendPacket(dest_nodeID, packetNumber++, packetData, packetDataLength+1);

        Timer1 = myTimer;
        while (Timer1 && delayFlag);
    }
    
    // Timer1 = myTimer;
    // while (Timer1 && delayFlag);
    
}

/**
 * @brief This is a wrapper function for MCO_ProcessStack to check for Packet data
 * @details This function includes the functionality of pushing and pulling from the can stack
 *          that the mco_processStack function has but also wraps around that the capacity to 
 *          check for Packet data to then check if a complete message is received. If so, we send
 *          the buffer to be handled based on the type of message received.
 * @param void
 * @return returns 0 on completion
 */

int MCO_ProcessStack_Menu(void) {
	
	MCO_ProcessStack();

	if (gProcImg[OUT_digi_0] & 0b11111100){
    // if any bits are set in gProcImg, packet has been received
        

        // triggers when any packet is received
        rxbuffer_len = receivePackets(rxbuffer, (sizeof(rxbuffer)/sizeof(rxbuffer[0])));
      

        // rxbuffer_len is only >0 when last packet is received in message from receivePackets
		// ReceivedPackets returns -1 on failure or 0 if it hasn't received a complete message yet
        if (rxbuffer_len > 0)
        {
            // handles can message
            handle_buffer(rxbuffer);

            if (Gen_Flags & Gen_Flags_Menu_Active){
              UpdateMenu = 1;
            }
            
        }

    }
    return 0;
}