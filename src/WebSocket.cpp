#include "WebSocket.hpp"
#include "CLog.hpp"

#include <unordered_map>


WebSocket::WebSocket() :
	m_SslContext(asio::ssl::context::sslv23),
	m_HeartbeatTimer(m_IoService)
{
}

WebSocket::~WebSocket()
{
	m_IoService.stop();

	if (m_IoThread)
	{
		m_IoThread->join();
		delete m_IoThread;
		m_IoThread = nullptr;
	}
	
	Disconnect();
}

void WebSocket::Initialize(std::string token, std::string gateway_url)
{
	m_GatewayUrl = gateway_url;
	m_Token = token;

	if (!Connect())
		return;

	Read();
	Identify();

	m_IoThread = new std::thread([this]()
	{
		m_IoService.run();
	});
}

bool WebSocket::Connect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::Connect");

	boost::system::error_code error;
	asio::ip::tcp::resolver r{ m_IoService };
	auto target = r.resolve({ m_GatewayUrl, "https" }, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't resolve Discord gateway URL '{}': {} ({})",
			m_GatewayUrl, error.message(), error.value());
		return false;
	}

	m_WebSocket.reset(new WebSocketStream_t(m_IoService, m_SslContext));
	asio::connect(m_WebSocket->lowest_layer(), target, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	m_WebSocket->next_layer().set_verify_mode(asio::ssl::verify_none, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"Can't configure SSL stream peer verification mode for Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	m_WebSocket->next_layer().handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	m_WebSocket->handshake(m_GatewayUrl, "/?encoding=json&v=6", error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't upgrade to WSS protocol: {} ({})",
			error.message(), error.value());
		return false;
	}

	return true;
}

void WebSocket::Disconnect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::Disconnect");

	boost::system::error_code error;
	m_WebSocket->close(beast::websocket::close_code::normal, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while sending WS close frame: {} ({})",
			error.message(), error.value());
	}

	m_HeartbeatTimer.cancel();
}

void WebSocket::Identify()
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::WsIdentify");

#ifdef WIN32
	std::string os_name = "Windows";
#else
	std::string os_name = "Linux";
#endif

	json identify_payload = {
		{ "op", 2 },
		{ "d",{
			{ "token", m_Token },
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

	m_WebSocket->write(asio::buffer(identify_payload.dump()));
}

void WebSocket::SendResumePayload()
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::WsSendResumePayload");

	json resume_payload = {
		{ "op", 6 },
		{ "d",{
			{ "token", m_Token },
			{ "session_id", m_SessionId },
			{ "seq", m_SequenceNumber }
		} }
	};
	m_WebSocket->write(asio::buffer(resume_payload.dump()));
}

void WebSocket::Read()
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::WsRead");

	m_WebSocket->async_read(m_WebSocketBuffer,
		std::bind(&WebSocket::OnRead, this, std::placeholders::_1));
}

void WebSocket::OnRead(boost::system::error_code ec)
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::OnWsRead");

	if (ec)
	{
		bool reconnect = false;
		switch (ec.value())
		{
		case boost::asio::ssl::error::stream_errors::stream_truncated:
			CLog::Get()->Log(LogLevel::ERROR, "Discord terminated websocket connection; reason: {} ({})", 
				m_WebSocket->reason().reason.c_str(), m_WebSocket->reason().code);
			reconnect = true;
			break;
		case boost::asio::error::operation_aborted:
			// connection was closed, do nothing
			break;
		default:
			CLog::Get()->Log(LogLevel::ERROR, "Can't read from Discord websocket gateway: {} ({})",
				ec.message(), ec.value());
			reconnect = true;
			break;
		}

		if (reconnect)
		{
			CLog::Get()->Log(LogLevel::INFO,
				"websocket gateway connection terminated, attempting reconnect...");
			Reconnect();
			Read();
		}
		return;
	}

	std::stringstream ss;
	ss << beast::buffers(m_WebSocketBuffer.data());
	json result = json::parse(ss.str());
	m_WebSocketBuffer.consume(m_WebSocketBuffer.size());

	int payload_opcode = result["op"].get<int>();
	switch (payload_opcode)
	{
	case 0:
	{
		m_SequenceNumber = result["s"];

#define __WS_EVENT_MAP_PAIR(event) { #event, Event::event }
		static const std::unordered_map<std::string, Event> events_map{
			__WS_EVENT_MAP_PAIR(READY),
			__WS_EVENT_MAP_PAIR(RESUMED),
			__WS_EVENT_MAP_PAIR(CHANNEL_CREATE),
			__WS_EVENT_MAP_PAIR(CHANNEL_UPDATE),
			__WS_EVENT_MAP_PAIR(CHANNEL_DELETE),
			__WS_EVENT_MAP_PAIR(GUILD_CREATE),
			__WS_EVENT_MAP_PAIR(GUILD_UPDATE),
			__WS_EVENT_MAP_PAIR(GUILD_DELETE),
			__WS_EVENT_MAP_PAIR(GUILD_BAN_ADD),
			__WS_EVENT_MAP_PAIR(GUILD_BAN_REMOVE),
			__WS_EVENT_MAP_PAIR(GUILD_EMOJIS_UPDATE),
			__WS_EVENT_MAP_PAIR(GUILD_INTEGRATIONS_UPDATE),
			__WS_EVENT_MAP_PAIR(GUILD_MEMBER_ADD),
			__WS_EVENT_MAP_PAIR(GUILD_MEMBER_REMOVE),
			__WS_EVENT_MAP_PAIR(GUILD_MEMBER_UPDATE),
			__WS_EVENT_MAP_PAIR(GUILD_MEMBERS_CHUNK),
			__WS_EVENT_MAP_PAIR(GUILD_ROLE_CREATE),
			__WS_EVENT_MAP_PAIR(GUILD_ROLE_UPDATE),
			__WS_EVENT_MAP_PAIR(GUILD_ROLE_DELETE),
			__WS_EVENT_MAP_PAIR(MESSAGE_CREATE),
			__WS_EVENT_MAP_PAIR(MESSAGE_UPDATE),
			__WS_EVENT_MAP_PAIR(MESSAGE_DELETE),
			__WS_EVENT_MAP_PAIR(MESSAGE_DELETE_BULK),
			__WS_EVENT_MAP_PAIR(MESSAGE_REACTION_ADD),
			__WS_EVENT_MAP_PAIR(MESSAGE_REACTION_REMOVE),
			__WS_EVENT_MAP_PAIR(PRESENCE_UPDATE),
			__WS_EVENT_MAP_PAIR(TYPING_START),
			__WS_EVENT_MAP_PAIR(USER_SETTINGS_UPDATE),
			__WS_EVENT_MAP_PAIR(USER_UPDATE),
			__WS_EVENT_MAP_PAIR(VOICE_STATE_UPDATE),
			__WS_EVENT_MAP_PAIR(VOICE_SERVER_UPDATE)
		};

		auto it = events_map.find(result["t"].get<std::string>());
		if (it != events_map.end())
		{
			json &data = result["d"];
			Event event = it->second;
			switch (event)
			{
			case Event::READY:
				m_SessionId = data["session_id"].get<std::string>();
				break;
			}

			auto event_range = m_EventMap.equal_range(event);
			for (auto ev_it = event_range.first; ev_it != event_range.second; ++ev_it)
				ev_it->second(data);
		}
		else
		{
			CLog::Get()->Log(LogLevel::WARNING, "Unknown gateway event '{}'", result["t"].get<std::string>());
			CLog::Get()->Log(LogLevel::DEBUG, "UGE res: {}", result.dump(4));
		}
	} break;
	case 7: // reconnect
		Reconnect();
		break;
	case 9: // invalid session
		Identify();
		break;
	case 10: // hello
			 // start heartbeat
		m_HeartbeatInterval = std::chrono::milliseconds(result["d"]["heartbeat_interval"]);
		DoHeartbeat({});
		break;
	case 11: // heartbeat ACK
		CLog::Get()->Log(LogLevel::DEBUG, "heartbeat ACK");
		break;
	default:
		CLog::Get()->Log(LogLevel::WARNING, "Unhandled payload opcode '{}'", payload_opcode);
		CLog::Get()->Log(LogLevel::DEBUG, "UPO res: {}", result.dump(4));
	}

	Read();
}

void WebSocket::DoHeartbeat(boost::system::error_code ec)
{
	CLog::Get()->Log(LogLevel::DEBUG, "WebSocket::DoHeartbeat");

	if (ec)
	{
		switch (ec.value())
		{
		case boost::asio::error::operation_aborted:
			// timer was chancelled, do nothing
			CLog::Get()->Log(LogLevel::DEBUG, "heartbeat timer chancelled");
			break;
		default:
			CLog::Get()->Log(LogLevel::ERROR, "Heartbeat error: {} ({})",
				ec.message(), ec.value());
			break;
		}
		return;
	}

	json heartbeat_payload = {
		{ "op", 1 },
		{ "d", m_SequenceNumber }
	};

	boost::system::error_code error_code;
	m_WebSocket->write(asio::buffer(heartbeat_payload.dump()), error_code);
	if (error_code)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Heartbeat write error: {} ({})",
			ec.message(), ec.value());
		return;
	}

	CLog::Get()->Log(LogLevel::DEBUG, "sending heartbeat");

	m_HeartbeatTimer.expires_from_now(m_HeartbeatInterval);
	m_HeartbeatTimer.async_wait(std::bind(&WebSocket::DoHeartbeat, this, std::placeholders::_1));
}
