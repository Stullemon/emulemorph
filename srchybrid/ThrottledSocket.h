// ZZ:UploadBandWithThrottler (UDP) -->

#pragma once

struct SocketSentBytes {
    bool    success;
	uint32	sentBytesStandardPackets;
	uint32	sentBytesControlPackets;
};

class ThrottledControlSocket
{
public:
    virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) = 0;
};

class ThrottledFileSocket : public ThrottledControlSocket
{
public:
    virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) = 0;
    virtual DWORD GetLastCalledSend() = 0;
	//MORPH START - Changed by SiRoB, Scale to lowspeed
	/*
	virtual uint32	GetNeededBytes() = 0;
	*/
	virtual uint32	GetNeededBytes(bool lowspeed) = 0;
	//MORPH START - Changed by SiRoB, Scale to lowspeed
};

// <-- ZZ:UploadBandWithThrottler (UDP)
