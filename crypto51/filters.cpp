// filters.cpp - written and placed in the public domain by Wei Dai

#include "pch.h"
#include "filters.h"
#include "mqueue.h"
#include "fltrimpl.h"
#include "argnames.h"
#include <memory>
#include <functional>

NAMESPACE_BEGIN(CryptoPP)

Filter::Filter(BufferedTransformation *attachment)
	: m_attachment(attachment), m_continueAt(0)
{
}

BufferedTransformation * Filter::NewDefaultAttachment() const
{
	return new MessageQueue;
}

BufferedTransformation * Filter::AttachedTransformation()
{
	if (m_attachment.get() == NULL)
		m_attachment.reset(NewDefaultAttachment());
	return m_attachment.get();
}

const BufferedTransformation *Filter::AttachedTransformation() const
{
	if (m_attachment.get() == NULL)
		const_cast<Filter *>(this)->m_attachment.reset(NewDefaultAttachment());
	return m_attachment.get();
}

void Filter::Detach(BufferedTransformation *newOut)
{
	m_attachment.reset(newOut);
	NotifyAttachmentChange();
}

void Filter::Insert(Filter *filter)
{
	filter->m_attachment.reset(m_attachment.release());
	m_attachment.reset(filter);
	NotifyAttachmentChange();
}

unsigned int Filter::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	return AttachedTransformation()->CopyRangeTo2(target, begin, end, channel, blocking);
}

unsigned int Filter::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	return AttachedTransformation()->TransferTo2(target, transferBytes, channel, blocking);
}

void Filter::Initialize(const NameValuePairs &parameters, int propagation)
{
	m_continueAt = 0;
	IsolatedInitialize(parameters);
	PropagateInitialize(parameters, propagation);
}

bool Filter::Flush(bool hardFlush, int propagation, bool blocking)
{
	switch (m_continueAt)
	{
	case 0:
		if (IsolatedFlush(hardFlush, blocking))
			return true;
	case 1:
		if (OutputFlush(1, hardFlush, propagation, blocking))
			return true;
	}
	return false;
}

bool Filter::MessageSeriesEnd(int propagation, bool blocking)
{
	switch (m_continueAt)
	{
	case 0:
		if (IsolatedMessageSeriesEnd(blocking))
			return true;
	case 1:
		if (ShouldPropagateMessageSeriesEnd() && OutputMessageSeriesEnd(1, propagation, blocking))
			return true;
	}
	return false;
}

void Filter::PropagateInitialize(const NameValuePairs &parameters, int propagation, const std::string &channel)
{
	if (propagation)
		AttachedTransformation()->ChannelInitialize(channel, parameters, propagation-1);
}

unsigned int Filter::Output(int outputSite, const byte *inString, unsigned int length, int messageEnd, bool blocking, const std::string &channel)
{
	if (messageEnd)
		messageEnd--;
	unsigned int result = AttachedTransformation()->Put2(inString, length, messageEnd, blocking);
	m_continueAt = result ? outputSite : 0;
	return result;
}

bool Filter::OutputFlush(int outputSite, bool hardFlush, int propagation, bool blocking, const std::string &channel)
{
	if (propagation && AttachedTransformation()->ChannelFlush(channel, hardFlush, propagation-1, blocking))
	{
		m_continueAt = outputSite;
		return true;
	}
	m_continueAt = 0;
	return false;
}

bool Filter::OutputMessageSeriesEnd(int outputSite, int propagation, bool blocking, const std::string &channel)
{
	if (propagation && AttachedTransformation()->ChannelMessageSeriesEnd(channel, propagation-1, blocking))
	{
		m_continueAt = outputSite;
		return true;
	}
	m_continueAt = 0;
	return false;
}

// *************************************************************

unsigned int MeterFilter::Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	if (m_transparent)
	{
		FILTER_BEGIN;
		m_currentMessageBytes += length;
		m_totalBytes += length;

		if (messageEnd)
		{
			m_currentMessageBytes = 0;
			m_currentSeriesMessages++;
			m_totalMessages++;
		}
		
		FILTER_OUTPUT(1, begin, length, messageEnd);
		FILTER_END_NO_MESSAGE_END;
	}
	return 0;
}

bool MeterFilter::IsolatedMessageSeriesEnd(bool blocking)
{
	m_currentMessageBytes = 0;
	m_currentSeriesMessages = 0;
	m_totalMessageSeries++;
	return false;
}

// *************************************************************

void FilterWithBufferedInput::BlockQueue::ResetQueue(unsigned int blockSize, unsigned int maxBlocks)
{
	m_buffer.New(blockSize * maxBlocks);
	m_blockSize = blockSize;
	m_maxBlocks = maxBlocks;
	m_size = 0;
	m_begin = m_buffer;
}

byte *FilterWithBufferedInput::BlockQueue::GetBlock()
{
	if (m_size >= m_blockSize)
	{
		byte *ptr = m_begin;
		if ((m_begin+=m_blockSize) == m_buffer.end())
			m_begin = m_buffer;
		m_size -= m_blockSize;
		return ptr;
	}
	else
		return NULL;
}

byte *FilterWithBufferedInput::BlockQueue::GetContigousBlocks(unsigned int &numberOfBytes)
{
	numberOfBytes = STDMIN(numberOfBytes, STDMIN((unsigned int)(m_buffer.end()-m_begin), m_size));
	byte *ptr = m_begin;
	m_begin += numberOfBytes;
	m_size -= numberOfBytes;
	if (m_size == 0 || m_begin == m_buffer.end())
		m_begin = m_buffer;
	return ptr;
}

unsigned int FilterWithBufferedInput::BlockQueue::GetAll(byte *outString)
{
	unsigned int size = m_size;
	unsigned int numberOfBytes = m_maxBlocks*m_blockSize;
	const byte *ptr = GetContigousBlocks(numberOfBytes);
	memcpy(outString, ptr, numberOfBytes);
	memcpy(outString+numberOfBytes, m_begin, m_size);
	m_size = 0;
	return size;
}

void FilterWithBufferedInput::BlockQueue::Put(const byte *inString, unsigned int length)
{
	assert(m_size + length <= m_buffer.size());
	byte *end = (m_size < (unsigned int)(m_buffer.end()-m_begin)) ? m_begin + m_size : m_begin + m_size - m_buffer.size();
	unsigned int len = STDMIN(length, (unsigned int)(m_buffer.end()-end));
	memcpy(end, inString, len);
	if (len < length)
		memcpy(m_buffer, inString+len, length-len);
	m_size += length;
}

FilterWithBufferedInput::FilterWithBufferedInput(BufferedTransformation *attachment)
	: Filter(attachment)
{
}

FilterWithBufferedInput::FilterWithBufferedInput(unsigned int firstSize, unsigned int blockSize, unsigned int lastSize, BufferedTransformation *attachment)
	: Filter(attachment), m_firstSize(firstSize), m_blockSize(blockSize), m_lastSize(lastSize)
	, m_firstInputDone(false)
{
	if (m_firstSize < 0 || m_blockSize < 1 || m_lastSize < 0)
		throw InvalidArgument("FilterWithBufferedInput: invalid buffer size");

	m_queue.ResetQueue(1, m_firstSize);
}

void FilterWithBufferedInput::IsolatedInitialize(const NameValuePairs &parameters)
{
	InitializeDerivedAndReturnNewSizes(parameters, m_firstSize, m_blockSize, m_lastSize);
	if (m_firstSize < 0 || m_blockSize < 1 || m_lastSize < 0)
		throw InvalidArgument("FilterWithBufferedInput: invalid buffer size");
	m_queue.ResetQueue(1, m_firstSize);
	m_firstInputDone = false;
}

bool FilterWithBufferedInput::IsolatedFlush(bool hardFlush, bool blocking)
{
	if (!blocking)
		throw BlockingInputOnly("FilterWithBufferedInput");
	
	if (hardFlush)
		ForceNextPut();
	FlushDerived();
	
	return false;
}

unsigned int FilterWithBufferedInput::PutMaybeModifiable(byte *inString, unsigned int length, int messageEnd, bool blocking, bool modifiable)
{
	if (!blocking)
		throw BlockingInputOnly("FilterWithBufferedInput");

	if (length != 0)
	{
		unsigned int newLength = m_queue.CurrentSize() + length;

		if (!m_firstInputDone && newLength >= m_firstSize)
		{
			unsigned int len = m_firstSize - m_queue.CurrentSize();
			m_queue.Put(inString, len);
			FirstPut(m_queue.GetContigousBlocks(m_firstSize));
			assert(m_queue.CurrentSize() == 0);
			m_queue.ResetQueue(m_blockSize, (2*m_blockSize+m_lastSize-2)/m_blockSize);

			inString += len;
			newLength -= m_firstSize;
			m_firstInputDone = true;
		}

		if (m_firstInputDone)
		{
			if (m_blockSize == 1)
			{
				while (newLength > m_lastSize && m_queue.CurrentSize() > 0)
				{
					unsigned int len = newLength - m_lastSize;
					byte *ptr = m_queue.GetContigousBlocks(len);
					NextPutModifiable(ptr, len);
					newLength -= len;
				}

				if (newLength > m_lastSize)
				{
					unsigned int len = newLength - m_lastSize;
					NextPutMaybeModifiable(inString, len, modifiable);
					inString += len;
					newLength -= len;
				}
			}
			else
			{
				while (newLength >= m_blockSize + m_lastSize && m_queue.CurrentSize() >= m_blockSize)
				{
					NextPutModifiable(m_queue.GetBlock(), m_blockSize);
					newLength -= m_blockSize;
				}

				if (newLength >= m_blockSize + m_lastSize && m_queue.CurrentSize() > 0)
				{
					assert(m_queue.CurrentSize() < m_blockSize);
					unsigned int len = m_blockSize - m_queue.CurrentSize();
					m_queue.Put(inString, len);
					inString += len;
					NextPutModifiable(m_queue.GetBlock(), m_blockSize);
					newLength -= m_blockSize;
				}

				if (newLength >= m_blockSize + m_lastSize)
				{
					unsigned int len = RoundDownToMultipleOf(newLength - m_lastSize, m_blockSize);
					NextPutMaybeModifiable(inString, len, modifiable);
					inString += len;
					newLength -= len;
				}
			}
		}

		m_queue.Put(inString, newLength - m_queue.CurrentSize());
	}

	if (messageEnd)
	{
		if (!m_firstInputDone && m_firstSize==0)
			FirstPut(NULL);

		SecByteBlock temp(m_queue.CurrentSize());
		m_queue.GetAll(temp);
		LastPut(temp, temp.size());

		m_firstInputDone = false;
		m_queue.ResetQueue(1, m_firstSize);

		Output(1, NULL, 0, messageEnd, blocking);
	}
	return 0;
}

void FilterWithBufferedInput::ForceNextPut()
{
	if (!m_firstInputDone)
		return;
	
	if (m_blockSize > 1)
	{
		while (m_queue.CurrentSize() >= m_blockSize)
			NextPutModifiable(m_queue.GetBlock(), m_blockSize);
	}
	else
	{
		unsigned int len;
		while ((len = m_queue.CurrentSize()) > 0)
			NextPutModifiable(m_queue.GetContigousBlocks(len), len);
	}
}

void FilterWithBufferedInput::NextPutMultiple(const byte *inString, unsigned int length)
{
	assert(m_blockSize > 1);	// m_blockSize = 1 should always override this function
	while (length > 0)
	{
		assert(length >= m_blockSize);
		NextPutSingle(inString);
		inString += m_blockSize;
		length -= m_blockSize;
	}
}

// *************************************************************

void Redirector::ChannelInitialize(const std::string &channel, const NameValuePairs &parameters, int propagation)
{
	if (channel.empty())
	{
		m_target = parameters.GetValueWithDefault("RedirectionTargetPointer", (BufferedTransformation*)NULL);
		m_passSignal = parameters.GetValueWithDefault("PassSignal", true);
	}

	if (m_target && m_passSignal)
		m_target->ChannelInitialize(channel, parameters, propagation);
}

// *************************************************************

ProxyFilter::ProxyFilter(BufferedTransformation *filter, unsigned int firstSize, unsigned int lastSize, BufferedTransformation *attachment)
	: FilterWithBufferedInput(firstSize, 1, lastSize, attachment), m_filter(filter)
{
	if (m_filter.get())
		m_filter->Attach(new OutputProxy(*this, false));
}

bool ProxyFilter::IsolatedFlush(bool hardFlush, bool blocking)
{
	return m_filter.get() ? m_filter->Flush(hardFlush, -1, blocking) : false;
}

void ProxyFilter::SetFilter(Filter *filter)
{
	m_filter.reset(filter);
	if (filter)
	{
		OutputProxy *proxy;
		std::auto_ptr<OutputProxy> temp(proxy = new OutputProxy(*this, false));
		m_filter->TransferAllTo(*proxy);
		m_filter->Attach(temp.release());
	}
}

void ProxyFilter::NextPutMultiple(const byte *s, unsigned int len) 
{
	if (m_filter.get())
		m_filter->Put(s, len);
}

// *************************************************************

unsigned int ArraySink::Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	memcpy(m_buf+m_total, begin, STDMIN(length, SaturatingSubtract(m_size, m_total)));
	m_total += length;
	return 0;
}

byte * ArraySink::CreatePutSpace(unsigned int &size)
{
	size = m_size - m_total;
	return m_buf + m_total;
}

void ArraySink::IsolatedInitialize(const NameValuePairs &parameters)
{
	ByteArrayParameter array;
	if (!parameters.GetValue(Name::OutputBuffer(), array))
		throw InvalidArgument("ArraySink: missing OutputBuffer argument");
	m_buf = array.begin();
	m_size = array.size();
	m_total = 0;
}

unsigned int ArrayXorSink::Put2(const byte *begin, unsigned int length, int messageEnd, bool blocking)
{
	xorbuf(m_buf+m_total, begin, STDMIN(length, SaturatingSubtract(m_size, m_total)));
	m_total += length;
	return 0;
}

// *************************************************************

unsigned int StreamTransformationFilter::LastBlockSize(StreamTransformation &c, BlockPaddingScheme padding)
{
	if (c.MinLastBlockSize() > 0)
		return c.MinLastBlockSize();
	else if (c.MandatoryBlockSize() > 1 && !c.IsForwardTransformation() && padding != NO_PADDING && padding != ZEROS_PADDING)
		return c.MandatoryBlockSize();
	else
		return 0;
}

StreamTransformationFilter::StreamTransformationFilter(StreamTransformation &c, BufferedTransformation *attachment, BlockPaddingScheme padding)
   : FilterWithBufferedInput(0, c.MandatoryBlockSize(), LastBlockSize(c, padding), attachment)
	, m_cipher(c)
{
	assert(c.MinLastBlockSize() == 0 || c.MinLastBlockSize() > c.MandatoryBlockSize());

	bool isBlockCipher = (c.MandatoryBlockSize() > 1 && c.MinLastBlockSize() == 0);

	if (padding == DEFAULT_PADDING)
	{
		if (isBlockCipher)
			m_padding = PKCS_PADDING;
		else
			m_padding = NO_PADDING;
	}
	else
		m_padding = padding;

	if (!isBlockCipher && (m_padding == PKCS_PADDING || m_padding == ONE_AND_ZEROS_PADDING))
		throw InvalidArgument("StreamTransformationFilter: PKCS_PADDING and ONE_AND_ZEROS_PADDING cannot be used with " + c.AlgorithmName());
}

void StreamTransformationFilter::FirstPut(const byte *inString)
{
	m_optimalBufferSize = m_cipher.OptimalBlockSize();
	m_optimalBufferSize = STDMAX(m_optimalBufferSize, RoundDownToMultipleOf(4096U, m_optimalBufferSize));
}

void StreamTransformationFilter::NextPutMultiple(const byte *inString, unsigned int length)
{
	if (!length)
		return;

	unsigned int s = m_cipher.MandatoryBlockSize();

	do
	{
		unsigned int len = m_optimalBufferSize;
		byte *space = HelpCreatePutSpace(*AttachedTransformation(), NULL_CHANNEL, s, length, len);
		if (len < length)
		{
			if (len == m_optimalBufferSize)
				len -= m_cipher.GetOptimalBlockSizeUsed();
			len = RoundDownToMultipleOf(len, s);
		}
		else
			len = length;
		m_cipher.ProcessString(space, inString, len);
		AttachedTransformation()->PutModifiable(space, len);
		inString += len;
		length -= len;
	}
	while (length > 0);
}

void StreamTransformationFilter::NextPutModifiable(byte *inString, unsigned int length)
{
	m_cipher.ProcessString(inString, length);
	AttachedTransformation()->PutModifiable(inString, length);
}

void StreamTransformationFilter::LastPut(const byte *inString, unsigned int length)
{
	byte *space = NULL;
	
	switch (m_padding)
	{
	case NO_PADDING:
	case ZEROS_PADDING:
		if (length > 0)
		{
			unsigned int minLastBlockSize = m_cipher.MinLastBlockSize();
			bool isForwardTransformation = m_cipher.IsForwardTransformation();

			if (isForwardTransformation && m_padding == ZEROS_PADDING && (minLastBlockSize == 0 || length < minLastBlockSize))
			{
				// do padding
				unsigned int blockSize = STDMAX(minLastBlockSize, m_cipher.MandatoryBlockSize());
				space = HelpCreatePutSpace(*AttachedTransformation(), NULL_CHANNEL, blockSize);
				memcpy(space, inString, length);
				memset(space + length, 0, blockSize - length);
				m_cipher.ProcessLastBlock(space, space, blockSize);
				AttachedTransformation()->Put(space, blockSize);
			}
			else
			{
				if (minLastBlockSize == 0)
				{
					if (isForwardTransformation)
						throw InvalidDataFormat("StreamTransformationFilter: plaintext length is not a multiple of block size and NO_PADDING is specified");
					else
						throw InvalidCiphertext("StreamTransformationFilter: ciphertext length is not a multiple of block size");
				}

				space = HelpCreatePutSpace(*AttachedTransformation(), NULL_CHANNEL, length, m_optimalBufferSize);
				m_cipher.ProcessLastBlock(space, inString, length);
				AttachedTransformation()->Put(space, length);
			}
		}
		break;

	case PKCS_PADDING:
	case ONE_AND_ZEROS_PADDING:
		unsigned int s;
		s = m_cipher.MandatoryBlockSize();
		assert(s > 1);
		space = HelpCreatePutSpace(*AttachedTransformation(), NULL_CHANNEL, s, m_optimalBufferSize);
		if (m_cipher.IsForwardTransformation())
		{
			assert(length < s);
			memcpy(space, inString, length);
			if (m_padding == PKCS_PADDING)
			{
				assert(s < 256);
				byte pad = s-length;
				memset(space+length, pad, s-length);
			}
			else
			{
				space[length] = 1;
				memset(space+length+1, 0, s-length-1);
			}
			m_cipher.ProcessData(space, space, s);
			AttachedTransformation()->Put(space, s);
		}
		else
		{
			if (length != s)
				throw InvalidCiphertext("StreamTransformationFilter: ciphertext length is not a multiple of block size");
			m_cipher.ProcessData(space, inString, s);
			if (m_padding == PKCS_PADDING)
			{
				byte pad = space[s-1];
				if (pad < 1 || pad > s || std::find_if(space+s-pad, space+s, std::bind2nd(std::not_equal_to<byte>(), pad)) != space+s)
					throw InvalidCiphertext("StreamTransformationFilter: invalid PKCS #7 block padding found");
				length = s-pad;
			}
			else
			{
				while (length > 1 && space[length-1] == '\0')
					--length;
				if (space[--length] != '\1')
					throw InvalidCiphertext("StreamTransformationFilter: invalid ones-and-zeros padding found");
			}
			AttachedTransformation()->Put(space, length);
		}
		break;

	default:
		assert(false);
	}
}

// *************************************************************

void HashFilter::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_putMessage = parameters.GetValueWithDefault(Name::PutMessage(), false);
	m_hashModule.Restart();
}

unsigned int HashFilter::Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking)
{
	FILTER_BEGIN;
	m_hashModule.Update(inString, length);
	if (m_putMessage)
		FILTER_OUTPUT(1, inString, length, 0);
	if (messageEnd)
	{
		{
			unsigned int size, digestSize = m_hashModule.DigestSize();
			m_space = HelpCreatePutSpace(*AttachedTransformation(), NULL_CHANNEL, digestSize, digestSize, size = digestSize);
			m_hashModule.Final(m_space);
		}
		FILTER_OUTPUT(2, m_space, m_hashModule.DigestSize(), messageEnd);
	}
	FILTER_END_NO_MESSAGE_END;
}

// *************************************************************

HashVerificationFilter::HashVerificationFilter(HashTransformation &hm, BufferedTransformation *attachment, word32 flags)
	: FilterWithBufferedInput(attachment)
	, m_hashModule(hm)
{
	IsolatedInitialize(MakeParameters(Name::HashVerificationFilterFlags(), flags));
}

void HashVerificationFilter::InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, unsigned int &firstSize, unsigned int &blockSize, unsigned int &lastSize)
{
	m_flags = parameters.GetValueWithDefault(Name::HashVerificationFilterFlags(), (word32)DEFAULT_FLAGS);
	m_hashModule.Restart();
	unsigned int size = m_hashModule.DigestSize();
	m_verified = false;
	firstSize = m_flags & HASH_AT_BEGIN ? size : 0;
	blockSize = 1;
	lastSize = m_flags & HASH_AT_BEGIN ? 0 : size;
}

void HashVerificationFilter::FirstPut(const byte *inString)
{
	if (m_flags & HASH_AT_BEGIN)
	{
		m_expectedHash.New(m_hashModule.DigestSize());
		memcpy(m_expectedHash, inString, m_expectedHash.size());
		if (m_flags & PUT_HASH)
			AttachedTransformation()->Put(inString, m_expectedHash.size());
	}
}

void HashVerificationFilter::NextPutMultiple(const byte *inString, unsigned int length)
{
	m_hashModule.Update(inString, length);
	if (m_flags & PUT_MESSAGE)
		AttachedTransformation()->Put(inString, length);
}

void HashVerificationFilter::LastPut(const byte *inString, unsigned int length)
{
	if (m_flags & HASH_AT_BEGIN)
	{
		assert(length == 0);
		m_verified = m_hashModule.Verify(m_expectedHash);
	}
	else
	{
		m_verified = (length==m_hashModule.DigestSize() && m_hashModule.Verify(inString));
		if (m_flags & PUT_HASH)
			AttachedTransformation()->Put(inString, length);
	}

	if (m_flags & PUT_RESULT)
		AttachedTransformation()->Put(m_verified);

	if ((m_flags & THROW_EXCEPTION) && !m_verified)
		throw HashVerificationFailed();
}

// *************************************************************

void SignerFilter::IsolatedInitialize(const NameValuePairs &parameters)
{
	m_putMessage = parameters.GetValueWithDefault(Name::PutMessage(), false);
	m_messageAccumulator.reset(m_signer.NewSignatureAccumulator());
}

unsigned int SignerFilter::Put2(const byte *inString, unsigned int length, int messageEnd, bool blocking)
{
	FILTER_BEGIN;
	m_messageAccumulator->Update(inString, length);
	if (m_putMessage)
		FILTER_OUTPUT(1, inString, length, 0);
	if (messageEnd)
	{
		m_buf.New(m_signer.SignatureLength());
		m_signer.Sign(m_rng, m_messageAccumulator.release(), m_buf);
		FILTER_OUTPUT(2, m_buf, m_buf.size(), messageEnd);
		m_messageAccumulator.reset(m_signer.NewSignatureAccumulator());
	}
	FILTER_END_NO_MESSAGE_END;
}

SignatureVerificationFilter::SignatureVerificationFilter(const PK_Verifier &verifier, BufferedTransformation *attachment, word32 flags)
	: FilterWithBufferedInput(attachment)
	, m_verifier(verifier)
{
	IsolatedInitialize(MakeParameters(Name::SignatureVerificationFilterFlags(), flags));
}

void SignatureVerificationFilter::InitializeDerivedAndReturnNewSizes(const NameValuePairs &parameters, unsigned int &firstSize, unsigned int &blockSize, unsigned int &lastSize)
{
	m_flags = parameters.GetValueWithDefault(Name::SignatureVerificationFilterFlags(), (word32)DEFAULT_FLAGS);
	m_messageAccumulator.reset(m_verifier.NewVerificationAccumulator());
	unsigned int size =	m_verifier.SignatureLength();
	assert(size != 0);	// TODO: handle recoverable signature scheme
	m_verified = false;
	firstSize = m_flags & SIGNATURE_AT_BEGIN ? size : 0;
	blockSize = 1;
	lastSize = m_flags & SIGNATURE_AT_BEGIN ? 0 : size;
}

void SignatureVerificationFilter::FirstPut(const byte *inString)
{
	if (m_flags & SIGNATURE_AT_BEGIN)
	{
		if (m_verifier.SignatureUpfront())
			m_verifier.InputSignature(*m_messageAccumulator, inString, m_verifier.SignatureLength());
		else
		{
			m_signature.New(m_verifier.SignatureLength());
			memcpy(m_signature, inString, m_signature.size());
		}

		if (m_flags & PUT_SIGNATURE)
			AttachedTransformation()->Put(inString, m_signature.size());
	}
	else
	{
		assert(!m_verifier.SignatureUpfront());
	}
}

void SignatureVerificationFilter::NextPutMultiple(const byte *inString, unsigned int length)
{
	m_messageAccumulator->Update(inString, length);
	if (m_flags & PUT_MESSAGE)
		AttachedTransformation()->Put(inString, length);
}

void SignatureVerificationFilter::LastPut(const byte *inString, unsigned int length)
{
	if (m_flags & SIGNATURE_AT_BEGIN)
	{
		assert(length == 0);
		m_verifier.InputSignature(*m_messageAccumulator, m_signature, m_signature.size());
		m_verified = m_verifier.VerifyAndRestart(*m_messageAccumulator);
	}
	else
	{
		m_verifier.InputSignature(*m_messageAccumulator, inString, length);
		m_verified = m_verifier.VerifyAndRestart(*m_messageAccumulator);
		if (m_flags & PUT_SIGNATURE)
			AttachedTransformation()->Put(inString, length);
	}

	if (m_flags & PUT_RESULT)
		AttachedTransformation()->Put(m_verified);

	if ((m_flags & THROW_EXCEPTION) && !m_verified)
		throw SignatureVerificationFailed();
}

// *************************************************************

unsigned int Source::PumpAll2(bool blocking)
{
	// TODO: switch length type
	unsigned long i = UINT_MAX;
	RETURN_IF_NONZERO(Pump2(i, blocking));
	unsigned int j = UINT_MAX;
	return PumpMessages2(j, blocking);
}

bool Store::GetNextMessage()
{
	if (!m_messageEnd && !AnyRetrievable())
	{
		m_messageEnd=true;
		return true;
	}
	else
		return false;
}

unsigned int Store::CopyMessagesTo(BufferedTransformation &target, unsigned int count, const std::string &channel) const
{
	if (m_messageEnd || count == 0)
		return 0;
	else
	{
		CopyTo(target, ULONG_MAX, channel);
		if (GetAutoSignalPropagation())
			target.ChannelMessageEnd(channel, GetAutoSignalPropagation()-1);
		return 1;
	}
}

void StringStore::StoreInitialize(const NameValuePairs &parameters)
{
	ConstByteArrayParameter array;
	if (!parameters.GetValue(Name::InputBuffer(), array))
		throw InvalidArgument("StringStore: missing InputBuffer argument");
	m_store = array.begin();
	m_length = array.size();
	m_count = 0;
}

unsigned int StringStore::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	unsigned long position = 0;
	unsigned int blockedBytes = CopyRangeTo2(target, position, transferBytes, channel, blocking);
	m_count += position;
	transferBytes = position;
	return blockedBytes;
}

unsigned int StringStore::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	unsigned int i = (unsigned int)STDMIN((unsigned long)m_count+begin, (unsigned long)m_length);
	unsigned int len = (unsigned int)STDMIN((unsigned long)m_length-i, end-begin);
	unsigned int blockedBytes = target.ChannelPut2(channel, m_store+i, len, 0, blocking);
	if (!blockedBytes)
		begin += len;
	return blockedBytes;
}

unsigned int RandomNumberStore::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	if (!blocking)
		throw NotImplemented("RandomNumberStore: nonblocking transfer is not implemented by this object");

	unsigned long transferMax = transferBytes;
	for (transferBytes = 0; transferBytes<transferMax && m_count < m_length; ++transferBytes, ++m_count)
		target.ChannelPut(channel, m_rng.GenerateByte());
	return 0;
}

unsigned int NullStore::CopyRangeTo2(BufferedTransformation &target, unsigned long &begin, unsigned long end, const std::string &channel, bool blocking) const
{
	static const byte nullBytes[128] = {0};
	while (begin < end)
	{
		unsigned int len = STDMIN(end-begin, 128UL);
		unsigned int blockedBytes = target.ChannelPut2(channel, nullBytes, len, 0, blocking);
		if (blockedBytes)
			return blockedBytes;
		begin += len;
	}
	return 0;
}

unsigned int NullStore::TransferTo2(BufferedTransformation &target, unsigned long &transferBytes, const std::string &channel, bool blocking)
{
	unsigned long begin = 0;
	unsigned int blockedBytes = NullStore::CopyRangeTo2(target, begin, transferBytes, channel, blocking);
	transferBytes = begin;
	m_size -= begin;
	return blockedBytes;
}

NAMESPACE_END
