#include <iostream>
#include "DemoBridgeClient.h"
#include "CommandLineUtil.h"

using namespace std;
using namespace fpnn;

FPQuestPtr QWriter(const char* method, bool oneway){
    FPQWriter qw(6,method, oneway);
    qw.param("quest", "one");
    qw.param("int", 2); 
    qw.param("double", 3.3);
    qw.param("boolean", true);
    qw.paramArray("ARRAY",2);
    qw.param("first_vec");
    qw.param(4);
    qw.paramMap("MAP",5);
    qw.param("map1","first_map");
    qw.param("map2",true);
    qw.param("map3",5);
    qw.param("map4",5.7);
    qw.param("map5","中文");
	return qw.take();
}

int main(int argc, char* argv[])
{
	CommandLineParser::init(argc, argv);
	std::vector<std::string> mainParams = CommandLineParser::getRestParams();

	if (mainParams.size() != 3)
	{
		cout<<"Usage: "<<argv[0]<<" ip port quest_period(seconds) [-udp] [-keepAlive]"<<endl;
		return 0;
	}

	int period = std::stoi(mainParams[2]);

	demoGlobalClientManager.start();

	DemoBridgeClient client(mainParams[0], atoi(mainParams[1].c_str()), !CommandLineParser::exist("udp"));
	
	if (CommandLineParser::exist("keepAlive"))
		client.keepAlive();

	client.setConnectedNotify([](uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool connected) {

		cout<<"client connected. ID: "<<connectionId<<", endpoint: "<<endpoint<<", isTCP: "<<isTCP<<", encrypted: "<<encrypted<<", connect : "<<(connected ? "successful" : "failed")<<endl;
	});

	client.setConnectionClosedNotify([](uint64_t connectionId, const std::string& endpoint,
		bool isTCP, bool encrypted, bool closedByError) {

		cout<<"client Closed. ID: "<<connectionId<<", endpoint: "<<endpoint<<", isTCP: "<<isTCP<<", encrypted: "<<encrypted<<", closedByError: "<<closedByError<<endl;
	});

	while (true)
	{
		FPQuestPtr quest = QWriter("two way demo", false);
		try
		{
			client.sendQuest(quest, [](FPAnswerPtr answer, int errorCode){
				if (answer)
					cout<<" -- recv answer: "<<answer->json()<<endl;
				else
					cout<<" -- recv error for answer, code: "<<errorCode<<endl;
			});
			cout << "send a quest" << endl;
		}
		catch (...)
		{
			cout<<"error occurred when sending"<<endl;
			break;
		}

		sleep(period);
	}

	client.close();
	return 0;
}

