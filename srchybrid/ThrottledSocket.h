// ZZ:UploadBandWithThrottler (UDP) -->

#pragma once

struct SocketSentBytes {
    bool    success;
	uint32	sentBytesStandardPackets;
	uint32	sentBytesControlPackets;
    uint32  errorThatOccured;
};

class ThrottledControlSocket
{
public:
#if !defined DONT_USE_SOCKET_BUFFERING
    virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize, uint32 bufferlimit = 0) = 0;
#else
	virtual SocketSentBytes SendControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) = 0;
#endif
};

class ThrottledFileSocket : public ThrottledControlSocket
{
public:
#if !defined DONT_USE_SOCKET_BUFFERING
    virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize, uint32 bufferlimit = 0) = 0;
#else
	virtual SocketSentBytes SendFileAndControlData(uint32 maxNumberOfBytesToSend, uint32 minFragSize) = 0;
#endif
	//virtual DWORD GetLastCalledSend() = 0;
    //virtual uint32	GetNeededBytes() = 0;
	virtual bool	IsBusy() const = 0;
    virtual bool    HasQueues() const = 0;
	virtual DWORD GetBusyTimeSince() = 0;
	virtual float GetBusyRatioTime() = 0;
};

// <-- ZZ:UploadBandWithThrottler (UDP)
