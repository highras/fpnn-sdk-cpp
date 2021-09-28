#ifndef FPNN_Config_h
#define FPNN_Config_h

#include <string>
#include "FPMessage.h"

namespace fpnn
{
#define FPNN_SDK_VERSION "1.0.1"

#define FPNN_DEFAULT_MAX_PACKAGE_LEN (8*1024*1024)

//-- UDP max data length without IPv6 jumbogram.
#define FPNN_UDP_MAX_DATA_LENGTH (65507)

	class Config
	{
		public:
			//global config
			static std::string _version;
			static int _max_recv_package_length;

		public:
			//client config
			static bool _log_client_quest;
			static bool _log_client_answer;
			//static int16_t _log_client_slow;//no used, 
			static bool _embed_receiveBuffer_freeBySDK;

			class Client
			{
			public:
				class KeepAlive
				{
				public:
					static bool defaultEnable;
					static int pingInterval;			//-- In milliseconds
					static int maxPingRetryCount;
				};
			};

		public:
			class UDP
			{
			public:
				static int _LAN_MTU;
				static int _internet_MTU;
				static int _heartbeat_interval_seconds;
				static uint32_t _disordered_seq_tolerance;
				static uint32_t _disordered_seq_tolerance_before_first_package_received;
				static uint64_t _arq_reAck_interval_milliseconds;
				static uint64_t _arq_seqs_sync_interval_milliseconds;
				static int _max_cached_uncompleted_segment_package_count;
				static int _max_cached_uncompleted_segment_seconds;
				static int _max_untransmitted_seconds;
				static size_t _arq_urgent_seqs_sync_triggered_threshold;
				static int64_t _arq_urgnet_seqs_sync_interval_milliseconds;

				static size_t _unconfiremed_package_limitation;
				static size_t _max_package_sent_limitation_per_connection_second;
				static int _max_resent_count_per_call;

				static int _max_tolerated_milliseconds_before_first_package_received;
				static int _max_tolerated_milliseconds_before_valid_package_received;
				static int _max_tolerated_count_before_valid_package_received;
			};

		public:

			static inline void ClientQuestLog(const FPQuestPtr quest, const std::string& ip, uint16_t port){
				if(Config::_log_client_quest){
					UXLOG("CLI.QUEST","%s:%d Q=%s", ip.c_str(), port, quest->info().c_str());
				}
			}

			static inline void ClientAnswerLog(const FPAnswerPtr answer, const std::string& ip, uint16_t port){
				if(Config::_log_client_answer){
					UXLOG("CLI.ANSWER","%s:%d A=%s", ip.c_str(), port, answer->info().c_str());
				}
				/*
				if(Config::_log_client_slow > 0){
					int32_t cost = answer->timeCost();
					if(cost >= Config::_log_server_slow){
						UXLOG("CLI.SLOW","%s:%d C:%d Q=%s A=%s", ip.c_str(), port, cost, quest->info().c_str(), answer->info().c_str());
					}
				}*/
			}

			static inline void embed_ConfigReceiveBufferAutoFree(bool autoFree)
			{
				_embed_receiveBuffer_freeBySDK = autoFree;
			}
	};
}

#endif
