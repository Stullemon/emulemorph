#ifndef CRYPTOPP_FLTRIMPL_H
#define CRYPTOPP_FLTRIMPL_H

#define FILTER_BEGIN	\
	switch (m_continueAt)	\
	{	\
	case 0:	\
		m_inputPosition = 0;

#define FILTER_END_NO_MESSAGE_END_NO_RETURN	\
		break;	\
	default:	\
		assert(false);	\
	}

#define FILTER_END_NO_MESSAGE_END	\
	FILTER_END_NO_MESSAGE_END_NO_RETURN	\
	return 0;

/*
#define FILTER_END	\
	case -1:	\
		if (messageEnd && Output(-1, NULL, 0, messageEnd, blocking))	\
			return 1;	\
	FILTER_END_NO_MESSAGE_END
*/

#define FILTER_OUTPUT2(site, statement, output, length, messageEnd)	\
	{\
	case site:	\
	statement;	\
	if (Output(site, output, length, messageEnd, blocking))	\
		return STDMAX(1U, (unsigned int)length-m_inputPosition);\
	}

#define FILTER_OUTPUT(site, output, length, messageEnd)	\
	FILTER_OUTPUT2(site, 0, output, length, messageEnd)

#define FILTER_OUTPUT_BYTE(site, output)	\
	FILTER_OUTPUT(site, &(const byte &)(byte)output, 1, 0)

#endif
