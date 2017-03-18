#include <iostream>
#include <string>
#include <RakPeerInterface.h>
#include <MessageIdentifiers.h>
#include <BitStream.h>
#include <thread>

#include "GameMessages.h"

const unsigned short PORT = 5456;
RakNet::RakPeerInterface* pPeerInterface = nullptr;

int nextClientID = 1;

void sendNewClientID(RakNet::RakPeerInterface* pPeerInterface,
	RakNet::SystemAddress& address)
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_SET_CLIENT_ID);
	bs.Write(nextClientID);
	nextClientID++;
	pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED,
		0, address, false);
}

void handleNetworkMessages()
{
	RakNet::Packet* packet = nullptr;
	while (true)
	{
		for (packet = pPeerInterface->Receive(); packet;
		pPeerInterface->DeallocatePacket(packet),
			packet = pPeerInterface->Receive())
		{
			switch (packet->data[0])
			{
			case ID_NEW_INCOMING_CONNECTION:
				std::cout << "A connection is incoming.\n";
				sendNewClientID(pPeerInterface, packet->systemAddress);
				break;
			case ID_DISCONNECTION_NOTIFICATION:
				std::cout << "A client has disconnected.\n";
				break;
			case ID_CONNECTION_LOST:
				std::cout << "A client lost the connection.\n";
				break;
			default:
				std::cout << "Received a message with a unknown id: " <<
					packet->data[0];
				break;
			}
		}
	}
}

void sendClientPing(RakNet::RakPeerInterface* pPeerInterface)
{
	while (true)
	{
		RakNet::BitStream bs;
		bs.Write((RakNet::MessageID)GameMessages::ID_SERVER_TEXT_MESSAGE);
		bs.Write("Ping!");
		pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
			RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
		std::this_thread::sleep_for(std::chrono::seconds(1));
	}
}

int main()
{
	//Startup the server, and start it listening to clients
	std::cout << "Starting up the server..." << std::endl;
	//Initialize the Raknet peer interface first
	pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	//Create a socket descriptor to describe this connection
	RakNet::SocketDescriptor sd(PORT, 0);
	//Now call startup - max of 32 connections, on the assigned port
	pPeerInterface->Startup(32, &sd, 1);
	pPeerInterface->SetMaximumIncomingConnections(32);

	//Startup a thread to ping clients every second
	std::thread pingThread(sendClientPing, pPeerInterface);

	handleNetworkMessages();

	
	return 0;
}