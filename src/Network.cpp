#include "Network.hpp"
#include "CLog.hpp"


void Network::Initialize(std::string const &token)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Network::Initialize");

	m_Http = std::unique_ptr<::Http>(new ::Http(token));

	// retrieve WebSocket host URL
	m_Http->Get("/gateway", [this, token](Http::GetResponse res)
	{
		if (res.status != 200)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't retrieve Discord gateway URL: {} ({})",
				res.reason, res.status);
			return;
		}
		auto gateway_res = json::parse(res.body);
		std::string gateway_url = gateway_res["url"];

		// get rid of protocol
		size_t protocol_pos = gateway_url.find("wss://");
		if (protocol_pos != std::string::npos)
			gateway_url.erase(protocol_pos, 6); // 6 = length of "wss://"

		m_WebSocket->Initialize(token, gateway_url);
	});

}

Network::~Network()
{
	CLog::Get()->Log(LogLevel::DEBUG, "Network::~Network");
}

::Http &Network::Http()
{
	return *m_Http;
}

::WebSocket &Network::WebSocket()
{
	return *m_WebSocket;
}
