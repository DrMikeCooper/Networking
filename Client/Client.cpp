#include <glm/glm.hpp>
#include <glm/ext.hpp>
#include <iostream>

#include <MessageIdentifiers.h>
#include <BitStream.h>

#include "Client.h"
#include "Gizmos.h"
#include "Input.h"
#include "../Server/GameMessages.h"

using glm::vec3;
using glm::vec4;
using glm::mat4;
using aie::Gizmos;

Client::Client() {

}

Client::~Client() {
}

bool Client::startup() {
	
	setBackgroundColour(0.25f, 0.25f, 0.25f);

	// initialise gizmo primitive counts
	Gizmos::create(10000, 10000, 10000, 10000);

	// create simple camera transforms
	m_viewMatrix = glm::lookAt(vec3(10), vec3(0), vec3(0, 1, 0));
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	handleNetworkConnection();

	m_myGameObject.position = glm::vec3(0, 0, 0);
	m_myGameObject.colour = glm::vec4(1, 0, 0, 1);

	return true;
}

void Client::shutdown() {

	Gizmos::destroy();
}

void Client::update(float deltaTime) {

	// query time since application started
	float time = getTime();

	// wipe the gizmos clean for this frame
	Gizmos::clear();

	
	// quit if we press escape
	aie::Input* input = aie::Input::getInstance();

	if (input->isKeyDown(aie::INPUT_KEY_ESCAPE))
		quit();

	handleNetworkMessages();

	// move our gameobject
	if (input->isKeyDown(aie::INPUT_KEY_LEFT))
	{
		m_myGameObject.position.x -= 10.0f * deltaTime;
		sendClientGameObject();
	}
	if (input->isKeyDown(aie::INPUT_KEY_RIGHT))
	{
		m_myGameObject.position.x += 10.0f * deltaTime;
		sendClientGameObject();
	}

	Gizmos::addSphere(m_myGameObject.position,
		1.0f, 32, 32, m_myGameObject.colour);

	for (auto& otherClient : m_otherClientGameObjects)
	{
		Gizmos::addSphere(otherClient.second.position,
			1.0f, 32, 32, otherClient.second.colour);
	}

}

void Client::draw() {

	// wipe the screen to the background colour
	clearScreen();

	// update perspective in case window resized
	m_projectionMatrix = glm::perspective(glm::pi<float>() * 0.25f,
										  getWindowWidth() / (float)getWindowHeight(),
										  0.1f, 1000.f);

	Gizmos::draw(m_projectionMatrix * m_viewMatrix);
}

void Client::handleNetworkConnection()
{
	//Initialize the Raknet peer interface first
	m_pPeerInterface = RakNet::RakPeerInterface::GetInstance();
	initialiseClientConnection();
}

void Client::initialiseClientConnection()
{
	//Create a socket descriptor to describe this connection
	//No data needed, as we will be connecting to a server
	RakNet::SocketDescriptor sd;
	//Now call startup - max of 1 connections (to the server)
	m_pPeerInterface->Startup(1, &sd, 1);
	std::cout << "Connecting to server at: " << IP << std::endl;
	//Now call connect to attempt to connect to the given server
	RakNet::ConnectionAttemptResult res = m_pPeerInterface->Connect(IP, PORT, nullptr, 0);
	//Finally, check to see if we connected, and if not, throw a error
	if (res != RakNet::CONNECTION_ATTEMPT_STARTED)
	{
		std::cout << "Unable to start connection, Error number: " << res << std::endl;
	}
}

void Client::handleNetworkMessages()
{
	RakNet::Packet* packet;
	for (packet = m_pPeerInterface->Receive(); packet;
	m_pPeerInterface->DeallocatePacket(packet),
		packet = m_pPeerInterface->Receive())
	{
		switch (packet->data[0])
		{
		case ID_REMOTE_DISCONNECTION_NOTIFICATION:
			std::cout << "Another client has disconnected.\n";
			break;
		case ID_REMOTE_CONNECTION_LOST:
			std::cout << "Another client has lost the connection.\n";
			break;
		case ID_REMOTE_NEW_INCOMING_CONNECTION:
			std::cout << "Another client has connected.\n";
			break;
		case ID_CONNECTION_REQUEST_ACCEPTED:
			std::cout << "Our connection request has been accepted.\n";
			break;
		case ID_NO_FREE_INCOMING_CONNECTIONS:
			std::cout << "The server is full.\n";
			break;
		case ID_DISCONNECTION_NOTIFICATION:
			std::cout << "We have been disconnected.\n";
			break;
		case ID_CONNECTION_LOST:
			std::cout << "Connection lost.\n";
			break;
		case ID_SERVER_TEXT_MESSAGE:
		{
			RakNet::BitStream bsIn(packet->data, packet->length, false);
			bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
			RakNet::RakString str;
			bsIn.Read(str);
			std::cout << str.C_String() << std::endl;
			break;
		}
		case ID_SERVER_SET_CLIENT_ID:
			onSetClientIDPacket(packet);
			break;
		case ID_CLIENT_CLIENT_DATA:
			onReceivedClientDataPacket(packet);
			break;
		default:
			std::cout << "Received a message with a unknown id: " << packet->data[0];
			break;
		}
	}
}

void Client::onSetClientIDPacket(RakNet::Packet* packet)
{
	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	bsIn.Read(m_myClientID);
	std::cout << "Set my client ID to: " << m_myClientID << std::endl;
}

void Client::sendClientGameObject()
{
	RakNet::BitStream bs;
	bs.Write((RakNet::MessageID)GameMessages::ID_CLIENT_CLIENT_DATA);
	bs.Write(m_myClientID);
	bs.Write((char*)&m_myGameObject, sizeof(GameObject));
	m_pPeerInterface->Send(&bs, HIGH_PRIORITY, RELIABLE_ORDERED, 0,
		RakNet::UNASSIGNED_SYSTEM_ADDRESS, true);
}

void Client::onReceivedClientDataPacket(RakNet::Packet * packet)
{
	RakNet::BitStream bsIn(packet->data, packet->length, false);
	bsIn.IgnoreBytes(sizeof(RakNet::MessageID));
	int clientID;
	bsIn.Read(clientID);
	//If the clientID does not match our ID, we need to update
	//our client GameObject information.
	if (clientID != m_myClientID)
	{
		GameObject clientData;
		bsIn.Read((char*)&clientData, sizeof(GameObject));

		m_otherClientGameObjects[clientID] = clientData;

		//For now, just output the Game Object information to the
		//console
		std::cout << "Client " << clientID <<
			" at: " << clientData.position.x <<
			" " << clientData.position.z << std::endl;
	}
}