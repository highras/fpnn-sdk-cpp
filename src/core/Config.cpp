//#include <sys/time.h>
//#include <sys/resource.h>
#include "Config.h"

using namespace fpnn;

//global
int Config::_max_recv_package_length(FPNN_DEFAULT_MAX_PACKAGE_LEN);

//client config
bool Config::_log_client_quest(false);
bool Config::_log_client_answer(false);
//int16_t Config::_log_client_slow(false);

