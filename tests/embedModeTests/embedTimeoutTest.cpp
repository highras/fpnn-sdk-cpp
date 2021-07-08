#include <iostream>
#include "DemoBridgeClient.h"
#include "CommandLineUtil.h"

using namespace std;
using namespace fpnn;

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();

	if (mainParams.size() != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port delay_seconds [-e engine_quest_timeout] [-c client_quest_timeout] [-q single_quest_timeout] [-udp]"<<endl;
		return 0;
	}

	int clientEngineQuestTimeout = CommandLineParser::getInt("e", 0);
	int clientQuestTimeout = CommandLineParser::getInt("c", 0);
	int questTimeout = CommandLineParser::getInt("q", 0);

	if (clientEngineQuestTimeout > 0)
		ClientEngine::setQuestTimeout(clientEngineQuestTimeout);

	demoGlobalClientManager.start();

	DemoBridgeClient client(mainParams[0], atoi(mainParams[1].c_str()), !CommandLineParser::exist("udp"));

	client.setConnectedNotify([](uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool connected) {

		cout<<"client connected. ID: "<<connectionId<<", endpoint: "<<endpoint<<", isTCP: "<<isTCP<<", encrypted: "<<encrypted<<", connect : "<<(connected ? "successful" : "failed")<<endl;
	});

	client.setConnectionClosedNotify([](uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool closedByError) {

		cout<<"client Closed. ID: "<<connectionId<<", endpoint: "<<endpoint<<", isTCP: "<<isTCP<<", encrypted: "<<encrypted<<", closedByError: "<<closedByError<<endl;
	});

	if (clientQuestTimeout > 0)
		client.setQuestTimeout(clientQuestTimeout);

	FPQWriter qw(1, "custom delay");
	qw.param("delaySeconds", atoi(mainParams[2].c_str()));

	try
	{
		std::mutex locker;
		std::mutex* lockerPtr = &locker;
		locker.lock();
		client.sendQuest(qw.take(), [lockerPtr](FPAnswerPtr answer, int errorCode){
			if (answer)
				cout<<" -- recv answer: "<<answer->json()<<endl;
			else
				cout<<" -- recv error for answer, code: "<<errorCode<<endl;

			lockerPtr->unlock();
		}, questTimeout);
		cout << "send a quest" << endl;

		locker.lock();
		locker.unlock();
	}
	catch (...)
	{
		cout<<"error occurred when sending"<<endl;
	}

	client.close();
	return 0;
}

