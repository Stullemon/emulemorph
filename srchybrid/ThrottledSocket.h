// ZZ:UploadBandWithThrottler (UDP) -->

#pragma once

struct SocketSentBytes {
    bool    success;
	uint32	sentBytesStandardPackets;
	uint32	sentBytesControlPackets;
    uint32  errorThatOccured; //MORPH - ZZ
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
    //MORPH START - ZZ
    /*
    virtual DWORD GetLastCalledSend() = 0;
    virtual uint32	GetNeededBytes() = 0;
    */
    //MORPH END   - ZZ
	virtual bool IsBusyExtensiveCheck() = 0;
	virtual bool IsBusyQuickCheck() const = 0;
	virtual bool IsEnoughFileDataQueued(uint32 nMinFilePayloadBytes) const = 0;
    virtual bool HasQueues(bool bOnlyStandardPackets = false) const = 0;
#if defined DONT_USE_SOCKET_BUFFERING
	virtual bool UseBigSendBuffer()								{ return false; }
#endif
	//MORPH START - Changed by SiRoB, Show BusyTime
	virtual DWORD GetBusyTimeSince() const = 0;
	virtual float GetBusyRatioTime() const = 0;
	//MORPH END   - Changed by SiRoB, Show BusyTime
};

// <-- ZZ:UploadBandWithThrottler (UDP)
