// ZZ:UploadBandWithThrottler (UDP) -->

#pragma once

struct SocketSentBytes {
    bool    success;
	uint32	sentBytesStandardPackets;
	uint32	sentBytesControlPackets;
};

class ThrottledSocket
{
public:
    virtual SocketSentBytes Send(uint32 maxNumberOfBytesToSend, uint32 minFragSize, bool onlyAllowedToSendControlPacket) = 0;
};

// <-- ZZ:UploadBandWithThrottler (UDP)
