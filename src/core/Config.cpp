#include "Config.h"

using namespace fpnn;

//global
std::string Config::_version(FPNN_SDK_VERSION);
int Config::_max_recv_package_length(FPNN_DEFAULT_MAX_PACKAGE_LEN);

//client config
bool Config::_log_client_quest(false);
bool Config::_log_client_answer(false);
//int16_t Config::_log_client_slow(false);
bool Config::_embed_receiveBuffer_freeBySDK(true);

bool Config::Client::KeepAlive::defaultEnable(false);
int Config::Client::KeepAlive::pingInterval(20*1000);
int Config::Client::KeepAlive::maxPingRetryCount(3);

//UDP
int Config::UDP::_LAN_MTU(1500);
int Config::UDP::_internet_MTU(576);
uint32_t Config::UDP::_disordered_seq_tolerance(10000);
uint32_t Config::UDP::_disordered_seq_tolerance_before_first_package_received(500);
uint64_t Config::UDP::_arq_reAck_interval_milliseconds(20);
uint64_t Config::UDP::_arq_seqs_sync_interval_milliseconds(50);
int Config::UDP::_heartbeat_interval_seconds(20);
int Config::UDP::_max_cached_uncompleted_segment_package_count(100);
int Config::UDP::_max_cached_uncompleted_segment_seconds(300);
int Config::UDP::_max_untransmitted_seconds(60);
int64_t Config::UDP::_arq_urgnet_seqs_sync_interval_milliseconds(20);
size_t Config::UDP::_arq_urgent_seqs_sync_triggered_threshold(280);
size_t Config::UDP::_unconfiremed_package_limitation(320);
size_t Config::UDP::_max_package_sent_limitation_per_connection_second(5000);
int Config::UDP::_max_resent_count_per_call(640);
int Config::UDP::_max_tolerated_milliseconds_before_first_package_received(3000);
int Config::UDP::_max_tolerated_milliseconds_before_valid_package_received(20000);
int Config::UDP::_max_tolerated_count_before_valid_package_received(1000);
