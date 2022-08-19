#include <iostream>
#include "fpnn.h"

using namespace std;
using namespace fpnn;

class EventProcessor : public IQuestProcessor
{
public:
	virtual void connected(const ConnectionInfo& connInfo, bool connected)
	{
		cout<<"Connected to "<<connInfo.endpoint()<<" "<<(connected ? "successful" : "failed")<<endl;
	}

	virtual void connectionWillClose(const ConnectionInfo& connInfo, bool closeByError)
	{
		cout<<"Connection on "<<connInfo.endpoint()<<" closed"<<(closeByError ? " by error." : ".")<<endl;
	}
};

ClientPtr buildClient(int argc, const char** argv)
{
	if (argc == 2)
		return TCPClient::createClient(argv[1]);

	if (argc == 3)
	{
		if (strcasecmp("-udp", argv[2]) == 0)
			return UDPClient::createClient(argv[1]);

		if (strcasecmp("-udp", argv[1]) == 0)
			return UDPClient::createClient(argv[2]);
	}

	cout<<"Usage: "<<argv[0]<<" <endpoint> [-udp]"<<endl;
	return nullptr;
}

int main(int argc, const char** argv)
{
	ClientPtr client = buildClient(argc, argv);
	if (!client)
		return 0;
	
	client->setQuestProcessor(std::make_shared<EventProcessor>());

	FPQWriter qw(3, "two way demo");
	qw.param("int", 2);
	qw.param("double", 3.3);
	qw.param("boolean", true);

	FPQuestPtr quest = qw.take();

	bool status = client->sendQuest(quest, [](FPAnswerPtr answer, int errorCode) {
			if (errorCode == FPNN_EC_OK)
				cout<<"Received answer of first quest."<<endl;
			else
				cout<<"Received error answer of first quest. code is "<<errorCode<<endl;
		});

	if (!status)
		cout<<"Send first quest failed."<<endl;
	else
	{
		cout<<"Wait 2 seconds for the async answer received."<<endl;
		sleep(2);
	}


	FPQWriter qw2(1, "httpDemo");
	qw2.param("quest", "two");

	FPAnswerPtr answer = client->sendQuest(qw2.take());
	FPAReader ar(answer);
	if (ar.status() == 0)
		cout<<"Received answer of second quest."<<endl;
	else
		cout<<"Received error answer of second quest. code is "<<ar.wantInt("code")<<endl;


	return 0;
}