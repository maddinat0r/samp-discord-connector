#include "WebSocket.hpp"
#include "Logger.hpp"

#include <unordered_map>


WebSocket::WebSocket() :
	_sslContext(asio::ssl::context::tlsv12_client),
	m_HeartbeatTimer(_ioContext)
{
}

WebSocket::~WebSocket()
{
	Disconnect();

	if (_netThread)
		_netThread->join();
}

void WebSocket::Initialize(std::string token, std::string gateway_url)
{
	_gatewayUrl = gateway_url;
	_apiToken = token;

	if (!Connect())
		return;

	Read();
	Identify();

	_netThread = std::make_unique<std::thread>([this]()
	{
		_ioContext.run();
	});
}

bool WebSocket::Connect()
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::Connect");

	boost::system::error_code error;
	asio::ip::tcp::resolver r(asio::make_strand(_ioContext));
	auto target = r.resolve(_gatewayUrl, "443", error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Can't resolve Discord gateway URL '{}': {} ({})",
			_gatewayUrl, error.message(), error.value());
		return false;
	}

	_websocket.reset(new WebSocketStream_t(asio::make_strand(_ioContext), _sslContext));
	beast::get_lowest_layer(*_websocket).expires_after(std::chrono::seconds(30));
	beast::get_lowest_layer(*_websocket).connect(target, error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Can't connect to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	_websocket->next_layer().set_verify_mode(asio::ssl::verify_peer, error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"Can't configure SSL stream peer verification mode for Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	beast::get_lowest_layer(*_websocket).expires_after(std::chrono::seconds(30));
	_websocket->next_layer().handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, 
			"Can't establish secured connection to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	// websocket stream has its own timeout system
	beast::get_lowest_layer(*_websocket).expires_never();

	_websocket->set_option(
		beast::websocket::stream_base::timeout::suggested(beast::role_type::client));

	// set a decorator to change the User-Agent of the handshake
	_websocket->set_option(beast::websocket::stream_base::decorator(
		[](beast::websocket::request_type &req)
		{
			req.set(beast::http::field::user_agent,
				std::string(BOOST_BEAST_VERSION_STRING) +
				" websocket-client-async-ssl");
		}));

	_websocket->handshake(_gatewayUrl, "/?encoding=json&v=6", error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Can't upgrade to WSS protocol: {} ({})",
			error.message(), error.value());
		return false;
	}

	return true;
}

void WebSocket::Disconnect(bool reconnect /*= false*/)
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::Disconnect");

	if (_websocket)
	{
		_websocket->async_close(beast::websocket::close_code::normal,
			std::bind(&WebSocket::OnClose, this, std::placeholders::_1, reconnect));
	}
}

void WebSocket::OnClose(boost::system::error_code ec, bool reconnect)
{
	boost::ignore_unused(ec);

	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::OnClose");

	m_HeartbeatTimer.cancel();

	if (reconnect)
	{
		if (Connect())
		{
			SendResumePayload();
			Read();
		}
		else
		{
			// retry reconnect in 10 seconds
			std::this_thread::sleep_for(std::chrono::seconds(10));
			Disconnect(true);
		}
	}
}

void WebSocket::Identify()
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::WsIdentify");

#ifdef WIN32
	std::string os_name = "Windows";
#else
	std::string os_name = "Linux";
#endif

	json identify_payload = {
		{ "op", 2 },
		{ "d",{
			{ "token", _apiToken },
			{ "compress", false },
			{ "large_threshold", LARGE_THRESHOLD_NUMBER },
			{ "properties",{
				{ "$os", os_name },
				{ "$browser", BOOST_BEAST_VERSION_STRING },
				{ "$device", "SA-MP DCC plugin" },
				{ "$referrer", "" },
				{ "$referring_domain", "" }
			} }
		} }
	};

	_websocket->write(asio::buffer(identify_payload.dump()));
}

void WebSocket::SendResumePayload()
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::WsSendResumePayload");

	json resume_payload = {
		{ "op", 6 },
		{ "d",{
			{ "token", _apiToken },
			{ "session_id", m_SessionId },
			{ "seq", _sequenceNumber }
		} }
	};
	_websocket->write(asio::buffer(resume_payload.dump()));
}

void WebSocket::RequestGuildMembers(std::string guild_id)
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::RequestGuildMembers");

	json payload = {
		{ "op", 8 },
		{ "d",{
			{ "guild_id", guild_id },
			{ "query", "" },
			{ "limit", 0 }
		} }
	};
	_websocket->async_write(asio::buffer(payload.dump()),
		std::bind(&WebSocket::OnWrite, this, std::placeholders::_1));
}

void WebSocket::UpdateStatus(std::string const &status, std::string const &activity_name)
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::UpdateStatus");

	json payload = {
		{ "op", 3 },
		{ "d", {
			{ "since", nullptr },
			{ "game", nullptr },
			{ "status", status },
			{ "afk", false },
		} }
	};

	if (!activity_name.empty())
	{
		payload.at("d").at("game") = {
			{ "name", activity_name },
			{ "type", 0 }
		};
	}

	_websocket->async_write(asio::buffer(payload.dump()),
		std::bind(&WebSocket::OnWrite, this, std::placeholders::_1));
}

void WebSocket::Read()
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::WsRead");

	_websocket->async_read(_buffer,
		std::bind(&WebSocket::OnRead, this, std::placeholders::_1));
}

void WebSocket::OnRead(boost::system::error_code ec)
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::OnWsRead");

	if (ec)
	{
		bool reconnect = false;
		switch (ec.value())
		{
		case boost::asio::ssl::error::stream_errors::stream_truncated:
			Logger::Get()->Log(LogLevel::ERROR, "Discord terminated websocket connection; reason: {} ({})",
				_websocket->reason().reason.c_str(), _websocket->reason().code);
			reconnect = true;
			break;
		case boost::asio::error::operation_aborted:
			// connection was closed, do nothing
			break;
		default:
			Logger::Get()->Log(LogLevel::ERROR, "Can't read from Discord websocket gateway: {} ({})",
				ec.message(), ec.value());
			reconnect = true;
			break;
		}

		if (reconnect)
		{
			Logger::Get()->Log(LogLevel::INFO,
				"websocket gateway connection terminated, attempting reconnect...");
			Disconnect(true);
		}
		return;
	}

	json result = json::parse(
		beast::buffers_to_string(_buffer.data()));
	_buffer.clear();

	int payload_opcode = result["op"].get<int>();
	switch (payload_opcode)
	{
	case 0:
	{
		_sequenceNumber = result["s"];

#define __WS_EVENT_MAP_PAIR(event) { #event, Event::event }
		static const std::unordered_map<std::string, Event> events_map{
			__WS_EVENT_MAP_PAIR(READY),
			__WS_EVENT_MAP_PAIR(RESUMED),
			__WS_EVENT_MAP_PAIR(CHANNEL_CREATE),
			__WS_EVENT_MAP_PAIR(CHANNEL_UPDATE),
			__WS_EVENT_MAP_PAIR(CHANNEL_DELETE),
			__WS_EVENT_MAP_PAIR(CHANNEL_PINS_UPDATE),
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
			__WS_EVENT_MAP_PAIR(MESSAGE_REACTION_REMOVE_ALL),
			__WS_EVENT_MAP_PAIR(PRESENCE_UPDATE),
			__WS_EVENT_MAP_PAIR(TYPING_START),
			__WS_EVENT_MAP_PAIR(USER_UPDATE),
			__WS_EVENT_MAP_PAIR(VOICE_STATE_UPDATE),
			__WS_EVENT_MAP_PAIR(VOICE_SERVER_UPDATE),
			__WS_EVENT_MAP_PAIR(WEBHOOKS_UPDATE),
			__WS_EVENT_MAP_PAIR(PRESENCES_REPLACE),
		};

		auto it = events_map.find(result["t"].get<std::string>());
		if (it != events_map.end())
		{
			json &data = result["d"];
			Event event = it->second;

			if (event == Event::READY)
				m_SessionId = data["session_id"].get<std::string>();

			auto event_range = m_EventMap.equal_range(event);
			for (auto ev_it = event_range.first; ev_it != event_range.second; ++ev_it)
				ev_it->second(data);
		}
		else
		{
			Logger::Get()->Log(LogLevel::WARNING, "Unknown gateway event '{}'", result["t"].get<std::string>());
			Logger::Get()->Log(LogLevel::DEBUG, "UGE res: {}", result.dump(4));
		}
	} break;
	case 7: // reconnect
		Disconnect(true);
		return;
	case 9: // invalid session
		Identify();
		break;
	case 10: // hello
			 // start heartbeat
		m_HeartbeatInterval = std::chrono::milliseconds(result["d"]["heartbeat_interval"]);
		DoHeartbeat({});
		break;
	case 11: // heartbeat ACK
		Logger::Get()->Log(LogLevel::DEBUG, "heartbeat ACK");
		break;
	default:
		Logger::Get()->Log(LogLevel::WARNING, "Unhandled payload opcode '{}'", payload_opcode);
		Logger::Get()->Log(LogLevel::DEBUG, "UPO res: {}", result.dump(4));
	}

	Read();
}

void WebSocket::OnWrite(boost::system::error_code ec)
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::OnWrite");

	if (ec)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"Can't write to Discord websocket gateway: {} ({})",
			ec.message(), ec.value());
		// we don't handle reconnects here, as the read callback already does this
	}
}

void WebSocket::DoHeartbeat(boost::system::error_code ec)
{
	Logger::Get()->Log(LogLevel::DEBUG, "WebSocket::DoHeartbeat");

	if (ec)
	{
		switch (ec.value())
		{
		case boost::asio::error::operation_aborted:
			// timer was chancelled, do nothing
			Logger::Get()->Log(LogLevel::DEBUG, "heartbeat timer chancelled");
			break;
		default:
			Logger::Get()->Log(LogLevel::ERROR, "Heartbeat error: {} ({})",
				ec.message(), ec.value());
			break;
		}
		return;
	}

	json heartbeat_payload = {
		{ "op", 1 },
		{ "d", _sequenceNumber }
	};

	boost::system::error_code error_code;
	_websocket->write(asio::buffer(heartbeat_payload.dump()), error_code);
	if (error_code)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Heartbeat write error: {} ({})",
			ec.message(), ec.value());
		return;
	}

	Logger::Get()->Log(LogLevel::DEBUG, "sending heartbeat");

	m_HeartbeatTimer.expires_from_now(m_HeartbeatInterval);
	m_HeartbeatTimer.async_wait(std::bind(&WebSocket::DoHeartbeat, this, std::placeholders::_1));
}
