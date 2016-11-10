#include "CNetwork.hpp"
#include "CLog.hpp"

#include <beast/core/to_string.hpp>

#include <unordered_map>


void CNetwork::Initialize(std::string &&token)
{
	m_Token = std::move(token);
	m_HttpsStream.set_verify_mode(asio::ssl::verify_none);

	// connect to REST API
	asio::ip::tcp::resolver r{ m_IoService };
	boost::system::error_code error;
	auto target = r.resolve({ "discordapp.com", "https" }, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't resolve Discord API URL: {} ({})",
			error.message(), error.value());
		CSingleton::Destroy();
		return;
	}

	error.clear();
	asio::connect(m_HttpsStream.lowest_layer(), target, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord API: {} ({})",
			error.message(), error.value());
		CSingleton::Destroy();
		return;
	}

	// SSL handshake
	error.clear();
	m_HttpsStream.handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord API: {} ({})",
			error.message(), error.value());
		CSingleton::Destroy();
		return;
	}

	// retrieve WebSocket host URL
	HttpGet("/gateway", [this](HttpGetResponse res)
	{
		if (res.status != 200)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't retrieve Discord gateway URL: {} ({})",
				res.reason, res.status);
			return;
		}
		auto gateway_res = json::parse(res.body);
		m_GatewayUrl = gateway_res["url"];

		// get rid of protocol
		size_t protocol_pos = m_GatewayUrl.find("wss://");
		if (protocol_pos != std::string::npos)
			m_GatewayUrl.erase(protocol_pos, 6); // 6 = length of "wss://"

		m_WssStream.set_verify_mode(asio::ssl::verify_none);

		if (!WsConnect())
			return;

		WsRead();
		WsIdentify();
	});

	m_IoThread = new std::thread([this]()
	{ 
		m_IoService.run();
	});
}

CNetwork::~CNetwork()
{
	WsDisconnect();
	if (m_IoThread)
	{
		m_IoThread->join();
		delete m_IoThread;
		m_IoThread = nullptr;
	}
}

bool CNetwork::WsConnect()
{
	asio::ip::tcp::resolver r{ m_IoService };
	boost::system::error_code error;
	auto target = r.resolve({ m_GatewayUrl, "https" }, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't resolve Discord gateway URL '{}': {} ({})",
			m_GatewayUrl, error.message(), error.value());
		return false;
	}

	error.clear();
	asio::connect(m_WssStream.lowest_layer(), target, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	error.clear();
	m_WssStream.handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	error.clear();
	m_WebSocket.handshake(m_GatewayUrl, "/", error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't upgrade to WSS protocol: {} ({})",
			error.message(), error.value());
		return false;
	}

	return true;
}

void CNetwork::WsDisconnect()
{
	boost::system::error_code error;
	m_WebSocket.close(beast::websocket::close_code::normal, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while sending WS close frame: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_WssStream.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down WS connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_WssStream.lowest_layer().close(error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while closing WS connection: {} ({})",
			error.message(), error.value());
	}

	m_HeartbeatTimer.cancel();
}

void CNetwork::WsIdentify()
{
#ifdef WIN32
	std::string os_name = "Windows";
#else
	std::string os_name = "Linux";
#endif

	json identify_payload = {
		{ "op", 2 },
		{ "d",{
			{ "token", m_Token },
			{ "v", 5 },
			{ "compress", false },
			{ "large_threshold", 100 },
			{ "properties",{
				{ "$os", os_name },
				{ "$browser", "boost::asio" },
				{ "$device", "SA-MP DCC plugin" },
				{ "$referrer", "" },
				{ "$referring_domain", "" }
			} }
		} }
	};

	m_WebSocket.write(asio::buffer(identify_payload.dump()));
}

void CNetwork::WsSendResumePayload()
{
	if (!WsConnect())
		return;

	WsRead();

	json resume_payload = {
		{ "op", 6 },
		{ "d", {
			{ "token", m_Token },
			{ "session_id", m_SessionId },
			{ "seq", m_SequenceNumber }
		}}
	};
	m_WebSocket.write(asio::buffer(resume_payload.dump()));
}

void CNetwork::WsRead()
{
	m_WebSocket.async_read(m_WebSocketOpcode, m_WebSocketBuffer,
		std::bind(&CNetwork::OnWsRead, this, std::placeholders::_1));
}

void CNetwork::OnWsRead(boost::system::error_code ec)
{
	if (ec)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't read from Discord websocket gateway: {} ({})",
			ec.message(), ec.value());

		return;
	}

	json result = json::parse(beast::to_string(m_WebSocketBuffer.data()));
	m_WebSocketBuffer.consume(m_WebSocketBuffer.size());

	int payload_opcode = result["op"].get<int>();
	switch (payload_opcode)
	{
		case 0:
		{
			m_SequenceNumber = result["s"];

			static const std::unordered_map<std::string, WsEvent> events_map{
				{ "READY", WsEvent::READY },
				{ "RESUMED", WsEvent::RESUMED },
				{ "CHANNEL_CREATE", WsEvent::CHANNEL_CREATE },
				{ "CHANNEL_UPDATE", WsEvent::CHANNEL_UPDATE },
				{ "CHANNEL_DELETE", WsEvent::CHANNEL_DELETE },
				{ "GUILD_CREATE", WsEvent::GUILD_CREATE },
				{ "GUILD_UPDATE", WsEvent::GUILD_UPDATE },
				{ "GUILD_DELETE", WsEvent::GUILD_DELETE },
				{ "GUILD_BAN_ADD", WsEvent::GUILD_BAN_ADD },
				{ "GUILD_BAN_REMOVE", WsEvent::GUILD_BAN_REMOVE },
				{ "GUILD_EMOJIS_UPDATE", WsEvent::GUILD_EMOJIS_UPDATE },
				{ "GUILD_INTEGRATIONS_UPDATE", WsEvent::GUILD_INTEGRATIONS_UPDATE },
				{ "GUILD_MEMBER_ADD", WsEvent::GUILD_MEMBER_ADD },
				{ "GUILD_MEMBER_REMOVE", WsEvent::GUILD_MEMBER_REMOVE },
				{ "GUILD_MEMBER_UPDATE", WsEvent::GUILD_MEMBER_UPDATE },
				{ "GUILD_MEMBERS_CHUNK", WsEvent::GUILD_MEMBERS_CHUNK },
				{ "GUILD_ROLE_CREATE", WsEvent::GUILD_ROLE_CREATE },
				{ "GUILD_ROLE_UPDATE", WsEvent::GUILD_ROLE_UPDATE },
				{ "GUILD_ROLE_DELETE", WsEvent::GUILD_ROLE_DELETE },
				{ "MESSAGE_CREATE", WsEvent::MESSAGE_CREATE },
				{ "MESSAGE_UPDATE", WsEvent::MESSAGE_UPDATE },
				{ "MESSAGE_DELETE", WsEvent::MESSAGE_DELETE },
				{ "MESSAGE_DELETE_BULK", WsEvent::MESSAGE_DELETE_BULK },
				{ "PRESENCE_UPDATE", WsEvent::PRESENCE_UPDATE },
				{ "TYPING_START", WsEvent::TYPING_START },
				{ "USER_SETTINGS_UPDATE", WsEvent::USER_SETTINGS_UPDATE },
				{ "USER_UPDATE", WsEvent::USER_UPDATE },
				{ "VOICE_STATE_UPDATE", WsEvent::VOICE_STATE_UPDATE },
				{ "VOICE_SERVER_UPDATE", WsEvent::VOICE_SERVER_UPDATE }
			};

			auto it = events_map.find(result["t"].get<std::string>());
			if (it != events_map.end())
			{
				json &data = result["d"];
				WsEvent event = it->second;
				switch (event)
				{
					case WsEvent::READY:
						m_HeartbeatInterval = std::chrono::milliseconds(data["heartbeat_interval"]);
						m_SessionId = data["session_id"];

						// start heartbeat
						DoHeartbeat({ });
						break;
				}

				auto ev_it = m_EventMap.find(event);
				if (ev_it != m_EventMap.end())
					ev_it->second(data);
			}
			else
			{
				CLog::Get()->Log(LogLevel::WARNING, "Unknown gateway event '{}'", result["t"].get<std::string>());
				CLog::Get()->Log(LogLevel::DEBUG, "UGE res: {}", result.dump(4));
			}
		} break;
		case 7: // reconnect
			WsDisconnect();
			WsConnect();
			WsSendResumePayload();
			DoHeartbeat({ });
			break;
		case 9: // invalid session
			WsIdentify();
			break;
		default:
			CLog::Get()->Log(LogLevel::WARNING, "Unhandled payload opcode '{}'", payload_opcode);
			CLog::Get()->Log(LogLevel::DEBUG, "UPO res: {}", result.dump(4));
	}

	WsRead();
}

void CNetwork::DoHeartbeat(boost::system::error_code ec)
{
	if (ec)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Heartbeat error: {} ({})",
			ec.message(), ec.value());
		return;
	}

	json heartbeat_payload = {
		{ "op", 1 },
		{ "d", m_SequenceNumber }
	};
	m_WebSocket.write(asio::buffer(heartbeat_payload.dump()));

	m_HeartbeatTimer.expires_from_now(m_HeartbeatInterval);
	m_HeartbeatTimer.async_wait(std::bind(&CNetwork::DoHeartbeat, this, std::placeholders::_1));
}


void CNetwork::HttpWriteRequest(std::string const &method,
	std::string const &url, std::string const &content, std::function<void()> &&callback)
{
	beast::http::request<beast::http::string_body> req;
	req.method = method;
	req.url = "/api" + url;
	req.version = 11;
	req.headers.replace("Host", "discordapp.com");
	if (!token.empty())
		req.headers.replace("Authorization", "Bot " + token);
	req.body = content;

	beast::http::prepare(req);

	beast::http::async_write(
		m_HttpsStream,
		req,
		[url, method, callback](boost::system::error_code ec)
		{
			if (ec)
			{
				CLog::Get()->Log(LogLevel::ERROR, "Error while sending HTTP {} request to '{}': {}",
					method, url, ec.message());
				return;
			}

			if (callback)
				callback();
		}
	);
}

void CNetwork::HttpReadResponse(HttpReadResponseCallback_t &&callback)
{
	auto sb = std::make_shared<beast::streambuf>();
	auto response = std::make_shared<beast::http::response<beast::http::streambuf_body>>();
	beast::http::async_read(
		m_HttpsStream,
		*sb,
		*response,
		[callback, sb, response](boost::system::error_code ec)
		{
			if (ec)
			{
				CLog::Get()->Log(LogLevel::ERROR, "Error while retrieving HTTP response: {}",
					ec.message());
				return;
			}

			callback(sb, response);
		}
	);
}


void CNetwork::HttpGet(std::string const &url, HttpGetCallback_t &&callback)
{
	HttpWriteRequest("GET", url, "", [this, callback]()
	{
		HttpReadResponse([callback](SharedStreambuf_t sb, SharedResponse_t resp)
		{
			callback({ resp->status, resp->reason, beast::to_string(resp->body.data()),
				beast::to_string(sb->data()) });
		});
	});
}

void CNetwork::HttpPost(std::string const &url, std::string const &content)
{
	HttpWriteRequest("POST", url, content, nullptr);
}
