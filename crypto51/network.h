#ifndef CRYPTOPP_NETWORK_H
#define CRYPTOPP_NETWORK_H

#include "filters.h"
#include "hrtimer.h"

NAMESPACE_BEGIN(CryptoPP)

//! a Source class that can pump from a device for a specified amount of time.
class NonblockingSource : public AutoSignaling<Source>
{
public:
	NonblockingSource(BufferedTransformation *attachment)
		: AutoSignaling<Source>(attachment), m_messageEndSent(false) {}

	//!	\name NONBLOCKING SOURCE
	//@{

	//! pump up to maxSize bytes using at most maxTime milliseconds
	/*! If checkDelimiter is true, pump up to delimiter, which itself is not extracted or pumped. */
	virtual unsigned int GeneralPump2(unsigned long &byteCount, bool blockingOutput=true, unsigned long maxTime=INFINITE_TIME, bool checkDelimiter=false, byte delimiter='\n') =0;

	unsigned long GeneralPump(unsigned long maxSize=ULONG_MAX, unsigned long maxTime=INFINITE_TIME, bool checkDelimiter=false, byte delimiter='\n')
	{
		GeneralPump2(maxSize, true, maxTime, checkDelimiter, delimiter);
		return maxSize;
	}
	unsigned long TimedPump(unsigned long maxTime)
		{return GeneralPump(ULONG_MAX, maxTime);}
	unsigned long PumpLine(byte delimiter='\n', unsigned long maxSize=1024)
		{return GeneralPump(maxSize, INFINITE_TIME, true, delimiter);}

	unsigned int Pump2(unsigned long &byteCount, bool blocking=true)
		{return GeneralPump2(byteCount, blocking, blocking ? INFINITE_TIME : 0);}
	unsigned int PumpMessages2(unsigned int &messageCount, bool blocking=true);
	//@}

private:
	bool m_messageEndSent;
};

//! Network Receiver
class NetworkReceiver : public Waitable
{
public:
	virtual bool MustWaitToReceive() {return false;}
	virtual bool MustWaitForResult() {return false;}
	virtual void Receive(byte* buf, unsigned int bufLen) =0;
	virtual unsigned int GetReceiveResult() =0;
	virtual bool EofReceived() const =0;
};

//! a Sink class that queues input and can flush to a device for a specified amount of time.
class NonblockingSink : public Sink
{
public:
	bool IsolatedFlush(bool hardFlush, bool blocking);

	//! flush to device for no more than maxTime milliseconds
	/*! This function will repeatedly attempt to flush data to some device, until
		the queue is empty, or a total of maxTime milliseconds have elapsed.
		If maxTime == 0, at least one attempt will be made to flush some data, but
		it is likely that not all queued data will be flushed, even if the device
		is ready to receive more data without waiting. If you want to flush as much data
		as possible without waiting for the device, call this function in a loop.
		For example: while (sink.TimedFlush(0) > 0) {}
		\return number of bytes flushed
	*/
	virtual unsigned int TimedFlush(unsigned long maxTime, unsigned int targetSize = 0) =0;

	virtual void SetMaxBufferSize(unsigned int maxBufferSize) =0;
	virtual void SetAutoFlush(bool autoFlush = true) =0;

	virtual unsigned int GetMaxBufferSize() const =0;
	virtual unsigned int GetCurrentBufferSize() const =0;
};

//! Network Sender
class NetworkSender : public Waitable
{
public:
	virtual bool MustWaitToSend() {return false;}
	virtual bool MustWaitForResult() {return false;}
	virtual void Send(const byte* buf, unsigned int bufLen) =0;
	virtual unsigned int GetSendResult() =0;
	virtual void SendEof() =0;
};

#ifdef HIGHRES_TIMER_AVAILABLE

//! Network Source
class NetworkSource : public NonblockingSource
{
public:
	NetworkSource(BufferedTransformation *attachment);

	unsigned int GetMaxWaitObjectCount() const
		{return GetReceiver().GetMaxWaitObjectCount() + AttachedTransformation()->GetMaxWaitObjectCount();}
	void GetWaitObjects(WaitObjectContainer &container)
		{AccessReceiver().GetWaitObjects(container); AttachedTransformation()->GetWaitObjects(container);}

	unsigned int GeneralPump2(unsigned long &byteCount, bool blockingOutput=true, unsigned long maxTime=INFINITE_TIME, bool checkDelimiter=false, byte delimiter='\n');
	bool SourceExhausted() const {return GetReceiver().EofReceived();}

protected:
	virtual NetworkReceiver & AccessReceiver() =0;
	const NetworkReceiver & GetReceiver() const {return const_cast<NetworkSource *>(this)->AccessReceiver();}

private:
	enum {NORMAL, WAITING_FOR_RESULT, OUTPUT_BLOCKED};
	SecByteBlock m_buf;
	unsigned int m_bufSize, m_putSize, m_state;
};

//! Network Sink
class NetworkSink : public NonblockingSink
{
public:
	NetworkSink(unsigned int maxBufferSize, bool autoFlush)
		: m_maxBufferSize(maxBufferSize), m_autoFlush(autoFlush), m_needSendResult(false), m_blockedBytes(0) {}

	unsigned int GetMaxWaitObjectCount() const
		{return GetSender().GetMaxWaitObjectCount();}
	void GetWaitObjects(WaitObjectContainer &container)
		{if (m_blockedBytes || !m_buffer.IsEmpty()) AccessSender().GetWaitObjects(container);}

	unsigned int Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking);

	unsigned int TimedFlush(unsigned long maxTime, unsigned int targetSize = 0);

	void SetMaxBufferSize(unsigned int maxBufferSize) {m_maxBufferSize = maxBufferSize;}
	void SetAutoFlush(bool autoFlush = true) {m_autoFlush = autoFlush;}

	unsigned int GetMaxBufferSize() const {return m_maxBufferSize;}
	unsigned int GetCurrentBufferSize() const {return m_buffer.CurrentSize();}

protected:
	virtual NetworkSender & AccessSender() =0;
	const NetworkSender & GetSender() const {return const_cast<NetworkSink *>(this)->AccessSender();}

private:
	unsigned int m_maxBufferSize;
	bool m_autoFlush, m_needSendResult;
	ByteQueue m_buffer;
	unsigned int m_blockedBytes;
};

#endif	// #ifdef HIGHRES_TIMER_AVAILABLE

NAMESPACE_END

#endif
