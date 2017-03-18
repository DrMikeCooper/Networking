#pragma once

#include "Application.h"
#include <glm/glm.hpp>
#include <RakPeerInterface.h>

struct GameObject
{
	glm::vec3 position;
	glm::vec4 colour;
};

class Client : public aie::Application {
public:

	Client();
	virtual ~Client();

	virtual bool startup();
	virtual void shutdown();

	virtual void update(float deltaTime);
	virtual void draw();

	// Initialize the connection
	void handleNetworkConnection();
	void initialiseClientConnection();
	// Handle incoming packets
	void handleNetworkMessages();

	void onSetClientIDPacket(RakNet::Packet* packet);
protected:

	RakNet::RakPeerInterface* m_pPeerInterface;
	const char* IP = "127.0.0.1";
	const unsigned short PORT = 5456;

	GameObject m_myGameObject;
	int m_myClientID;

	glm::mat4	m_viewMatrix;
	glm::mat4	m_projectionMatrix;
};