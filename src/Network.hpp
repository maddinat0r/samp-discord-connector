#pragma once

#include "Singleton.hpp"

#include <string>
#include <memory>

#include "Http.hpp"
#include "WebSocket.hpp"


class Network : public Singleton<Network>
{
	friend class Singleton<Network>;
private:
	Network()
	{
		m_WebSocket = std::unique_ptr<::WebSocket>(new ::WebSocket());
	}
	~Network();

private: // variables
	std::unique_ptr<::Http> m_Http;
	std::unique_ptr<::WebSocket> m_WebSocket;

public: // functions
	void Initialize(std::string const &token, int intents);

	::Http &Http();
	::WebSocket &WebSocket();
};
