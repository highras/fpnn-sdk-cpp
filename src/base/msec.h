#ifndef FPNN_MSEC_h
#define FPNN_MSEC_h

/*
	This file is fake msec.h.
	The real msec.h only running on Linux.
*/

#include "TimeUtil.h"

#define slack_real_msec fpnn::TimeUtil::curr_msec
#define slack_real_sec fpnn::TimeUtil::curr_sec

//-- Only for UDP.v2/UDPIOBuffer.v2
#define slack_mono_sec slack_real_sec
#define slack_mono_msec slack_real_msec

#endif
