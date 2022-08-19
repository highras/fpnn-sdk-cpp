#include <iostream>
#include "fpnn.h"

using namespace std;
using namespace fpnn;

ClientPtr buildClient(int argc, const char** argv)
{
	if (argc == 3)
	{
		TCPClientPtr client = TCPClient::createClient(argv[1]);
		client->enableEncryptorByPemFile(argv[2]);
		return client;
	}

	if (argc == 4)
	{
		if (strcasecmp("-udp", argv[3]) == 0)
		{
			UDPClientPtr client = UDPClient::createClient(argv[1]);
			client->enableEncryptorByPemFile(argv[2]);
			return client;
		}

		if (strcasecmp("-udp", argv[1]) == 0)
		{
			UDPClientPtr client = UDPClient::createClient(argv[2]);
			client->enableEncryptorByPemFile(argv[3]);
			return client;
		}
	}

	cout<<"Usage: "<<argv[0]<<" <endpoint> <server-pem-key-file> [-udp]"<<endl;
	return nullptr;
}

int main(int argc, const char** argv)
{
	ClientPtr client = buildClient(argc, argv);
	if (!client)
		return 0;

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