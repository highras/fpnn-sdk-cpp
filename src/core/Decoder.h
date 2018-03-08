#ifndef FPNN_Decoder_H
#define FPNN_Decoder_H

#include <exception>
#include "FPMessage.h"

namespace fpnn
{
	class Decoder
	{
	public:
		static FPQuestPtr decodeQuest(const char* buf, int len)
		{
			size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen(buf);

			if(len != (int)total_len) 
				throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_INVALID_PACKAGE, "Expect Len:%d, readed Len:%d", total_len, len);

			FPQuestPtr quest = NULL;
			try{
				quest.reset(new FPQuest(buf,total_len));
			}
			catch(const FpnnError& ex){
				LOG_ERROR("Can not create Quest from Raw:(%d) %s", ex.code(), ex.what());
			}
			catch(...){
				LOG_ERROR("Unknow Error, Can not create Quest from Raw Data.");
			}
			return quest;
		}

		static FPAnswerPtr decodeAnswer(const char* buf, int len)
		{
			size_t total_len = sizeof(FPMessage::Header) + FPMessage::BodyLen(buf);

			if(len != (int)total_len)
				throw FPNN_ERROR_CODE_FMT(FpnnCoreError, FPNN_EC_CORE_INVALID_PACKAGE, "Expect Len:%d, readed Len:%d", total_len, len);

			FPAnswerPtr answer = NULL;
			try{
				answer.reset(new FPAnswer(buf,total_len));
			}
			catch(const FpnnError& ex){
				LOG_ERROR("Can not create Answer from Raw:(%d) %s", ex.code(), ex.what());
			}
			catch(...){
				LOG_ERROR("Unknown Error, Can not create Answer from Raw Data");
			}
			return answer;
		}
	};
}

#endif
