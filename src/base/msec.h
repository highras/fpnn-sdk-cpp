#ifndef FPNN_MSEC_h
#define FPNN_MSEC_h

/*
	This file is fake msec.h.
	The real msec.h only running on Linux.
*/

#include "TimeUtil.h"

#define slack_real_msec fpnn::TimeUtil::curr_msec
#define slack_real_sec fpnn::TimeUtil::curr_sec

#endif
