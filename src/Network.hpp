#pragma once

#include "CSingleton.hpp"

#include <string>
#include <thread>
#include <memory>
#include <boost/asio.hpp>

#undef SendMessage // Windows at its finest

namespace asio = boost::asio;

#include "Http.hpp"
#include "WebSocket.hpp"


class Network : public CSingleton<Network>
{
	friend class CSingleton<Network>;
private:
	Network() = default;
	~Network();

private: // variables
	asio::io_service m_IoService;
	std::thread *m_IoThread = nullptr;

	std::unique_ptr<::Http> m_Http;
	std::unique_ptr<::WebSocket> m_WebSocket;

public: // functions
	void Initialize(std::string const &token);

	::Http &Http();
	::WebSocket &WebSocket();
};
