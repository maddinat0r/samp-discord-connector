#include "WebSocket.hpp"
#include "Logger.hpp"
#include "sdk.hpp"

#include <unordered_map>

extern logprintf_t logprintf;

WebSocket::WebSocket() :
	_ioContext(),
	_resolver(asio::make_strand(_ioContext)),
	_sslContext(asio::ssl::context::tlsv12_client),
	_reconnectTimer(_ioContext),
	m_HeartbeatTimer(_ioContext),
	m_HeartbeatInterval()
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::WebSocket");
}

WebSocket::~WebSocket()
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::~WebSocket");

	Disconnect();

	if (_netThread)
		_netThread->join();
}

void WebSocket::Initialize(std::string token, std::string gateway_url)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::Initialize");

	_gatewayUrl = gateway_url;
	_apiToken = token;

	Connect();

	_netThread = std::make_unique<std::thread>([this]()
	{
		_ioContext.run();
	});
}

void WebSocket::Connect()
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::Connect");

	_resolver.async_resolve(
		_gatewayUrl,
		"443",
		beast::bind_front_handler(
			&WebSocket::OnResolve,
			this));
}

void WebSocket::OnResolve(beast::error_code ec, 
	asio::ip::tcp::resolver::results_type results)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::OnResolve");

	if (ec)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, 
			"Can't resolve Discord gateway URL '{}': {} ({})",
			_gatewayUrl, ec.message(), ec.value());
		Disconnect(true);
		return;
	}

	_websocket.reset(
		new WebSocketStream_t(asio::make_strand(_ioContext), _sslContext));

	beast::get_lowest_layer(*_websocket).expires_after(
		std::chrono::seconds(30));
	beast::get_lowest_layer(*_websocket).async_connect(
		results, 
		beast::bind_front_handler(
			&WebSocket::OnConnect,
			this));
}

void WebSocket::OnConnect(beast::error_code ec,
	asio::ip::tcp::resolver::results_type::endpoint_type ep)
{
	boost::ignore_unused(ep);

	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::OnConnect");

	if (ec)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR, 
			"Can't connect to Discord gateway: {} ({})",
			ec.message(), ec.value());
		Disconnect(true);
		return;
	}

	beast::get_lowest_layer(*_websocket).expires_after(std::chrono::seconds(30));
	_websocket->next_layer().async_handshake(
		asio::ssl::stream_base::client, 
		beast::bind_front_handler(
			&WebSocket::OnSslHandshake,
			this));
}

void WebSocket::OnSslHandshake(beast::error_code ec)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::OnSslHandshake");

	if (ec)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"Can't establish secured connection to Discord gateway: {} ({})",
			ec.message(), ec.value());
		Disconnect(true);
		return;
	}

	// websocket stream has its own timeout system
	beast::get_lowest_layer(*_websocket).expires_never();

	_websocket->set_option(
		beast::websocket::stream_base::timeout::suggested(
			beast::role_type::client));

	// set a decorator to change the User-Agent of the handshake
	_websocket->set_option(beast::websocket::stream_base::decorator(
		[](beast::websocket::request_type &req)
	{
		req.set(beast::http::field::user_agent,
			std::string(BOOST_BEAST_VERSION_STRING) +
			" samp-discord-connector");
	}));

	_websocket->async_handshake(
		_gatewayUrl + ":443", 
		"/?encoding=json&v=10",
		beast::bind_front_handler(
			&WebSocket::OnHandshake,
			this));
}

void WebSocket::OnHandshake(beast::error_code ec)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::OnHandshake");

	if (ec)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"Can't upgrade to WSS protocol: {} ({})",
			ec.message(), ec.value());
		Disconnect(true);
		return;
	}

	_reconnectCount = 0;

	// read before identifying/resuming to make sure we catch the result
	Read();
	if (_reconnect)
		SendResumePayload();
	else
		Identify();
}

void WebSocket::Disconnect(bool reconnect /*= false*/)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::Disconnect");

	_reconnect = reconnect;

	if (_websocket)
	{
		_websocket->async_close(
			beast::websocket::close_code::normal,
			beast::bind_front_handler(
				&WebSocket::OnClose,
				this));
	}
}

void WebSocket::OnClose(beast::error_code ec)
{
	boost::ignore_unused(ec);

	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::OnClose");

	m_HeartbeatTimer.cancel();

	if (_reconnect)
	{
		auto time = std::chrono::seconds(
			std::min(_reconnectCount * 5, 60u));

		if (time.count() > 0)
		{
			Logger::Get()->Log(samplog_LogLevel::INFO,
				"reconnecting in {:d} seconds",
				time.count());
		}

		_reconnectTimer.expires_from_now(time);
		_reconnectTimer.async_wait(
			beast::bind_front_handler(
				&WebSocket::OnReconnect,
				this));
	}
}

void WebSocket::OnReconnect(beast::error_code ec)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::OnReconnect");

	if (ec)
	{
		switch (ec.value())
		{
		case boost::asio::error::operation_aborted:
			// timer was chancelled, do nothing
			Logger::Get()->Log(samplog_LogLevel::DEBUG, "reconnect timer chancelled");
			break;
		default:
			Logger::Get()->Log(samplog_LogLevel::ERROR, "reconnect timer error: {} ({})",
				ec.message(), ec.value());
			break;
		}
		return;
	}

	++_reconnectCount;
	Connect();
}

void WebSocket::Read()
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::Read");

	_websocket->async_read(
		_buffer,
		beast::bind_front_handler(
			&WebSocket::OnRead,
			this));
}

void WebSocket::OnRead(beast::error_code ec,
	std::size_t bytes_transferred)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, 
		"WebSocket::OnRead({:d})",
		bytes_transferred);

	if (ec)
	{
		bool reconnect = false;
		switch (ec.value())
		{
		case asio::ssl::error::stream_errors::stream_truncated:
			Logger::Get()->Log(samplog_LogLevel::ERROR,
				"Discord terminated websocket connection; reason: {} ({})",
				_websocket->reason().reason.c_str(),
				_websocket->reason().code);

			if (_websocket->reason().code == 4014)
			{
				logprintf(" >> discord-connector: bot could not connect due to intent permissions. Modify your discord bot settings and enable every intent.");
				reconnect = false;
			}
			else
			{
				reconnect = true;
			}
			break;
		case asio::error::operation_aborted:
			// connection was closed, do nothing
			break;
		default:
			Logger::Get()->Log(samplog_LogLevel::ERROR,
				"Can't read from Discord websocket gateway: {} ({})",
				ec.message(),
				ec.value());
			reconnect = true;
			break;
		}

		if (reconnect)
		{
			Logger::Get()->Log(samplog_LogLevel::INFO,
				"websocket gateway connection terminated; attempting reconnect...");
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
			__WS_EVENT_MAP_PAIR(MESSAGE_REACTION_REMOVE_EMOJI),
			__WS_EVENT_MAP_PAIR(PRESENCE_UPDATE),
			__WS_EVENT_MAP_PAIR(TYPING_START),
			__WS_EVENT_MAP_PAIR(USER_UPDATE),
			__WS_EVENT_MAP_PAIR(VOICE_STATE_UPDATE),
			__WS_EVENT_MAP_PAIR(VOICE_SERVER_UPDATE),
			__WS_EVENT_MAP_PAIR(WEBHOOKS_UPDATE),
			__WS_EVENT_MAP_PAIR(PRESENCES_REPLACE),
			__WS_EVENT_MAP_PAIR(INVITE_CREATE),
			__WS_EVENT_MAP_PAIR(INVITE_DELETE),
			__WS_EVENT_MAP_PAIR(INTERACTION_CREATE)
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
			Logger::Get()->Log(samplog_LogLevel::WARNING, "Unknown gateway event '{}'", result["t"].get<std::string>());
			Logger::Get()->Log(samplog_LogLevel::DEBUG, "UGE res: {}", result.dump(4));
		}
	} break;
	case 7: // reconnect
		Logger::Get()->Log(samplog_LogLevel::INFO,
			"websocket gateway requested reconnect; attempting reconnect...");
		Disconnect(true);
		return;
	case 9: // invalid session
		Identify();
		break;
	case 10: // hello
		// at this point we're connected to the gateway, but not authenticated
		// start heartbeat
		m_HeartbeatInterval = std::chrono::milliseconds(result["d"]["heartbeat_interval"]);
		DoHeartbeat({});
		break;
	case 11: // heartbeat ACK
		Logger::Get()->Log(samplog_LogLevel::DEBUG, "heartbeat ACK");
		break;
	default:
		Logger::Get()->Log(samplog_LogLevel::WARNING, "Unhandled payload opcode '{}'", payload_opcode);
		Logger::Get()->Log(samplog_LogLevel::DEBUG, "UPO res: {}", result.dump(4));
	}

	Read();
}

void WebSocket::Write(std::string const &data)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::Write");

	_websocket->async_write(
		asio::buffer(data),
		beast::bind_front_handler(
			&WebSocket::OnWrite,
			this));
}

void WebSocket::OnWrite(beast::error_code ec,
	size_t bytes_transferred)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, 
		"WebSocket::OnWrite({:d})", 
		bytes_transferred);

	if (ec)
	{
		Logger::Get()->Log(samplog_LogLevel::ERROR,
			"Can't write to Discord websocket gateway: {} ({})",
			ec.message(), ec.value());

		// we don't handle reconnects here, as the read handler already does this
	}
}

#define ALL_INTENTS 32767
void WebSocket::Identify()
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::Identify");

	const char *os_name =
#ifdef WIN32
		"Windows";
#else
		"Linux";
#endif

	json identify_payload = {
		{ "op", 2 },
		{ "d",{
			{ "token", _apiToken },
			{ "compress", false },
			{ "intents", ALL_INTENTS },
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

	Write(identify_payload.dump());
}

void WebSocket::SendResumePayload()
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::SendResumePayload");

	json resume_payload = {
		{ "op", 6 },
		{ "d",{
			{ "token", _apiToken },
			{ "session_id", m_SessionId },
			{ "seq", _sequenceNumber }
		} }
	};

	Write(resume_payload.dump());
}

void WebSocket::RequestGuildMembers(std::string guild_id)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::RequestGuildMembers");

	json payload = {
		{ "op", 8 },
		{ "d",{
			{ "guild_id", guild_id },
			{ "query", "" },
			{ "limit", 0 }
		} }
	};

	Write(payload.dump());
}

void WebSocket::UpdateStatus(std::string const &status, std::string const &activity_name)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::UpdateStatus");

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

	Write(payload.dump());
}

void WebSocket::DoHeartbeat(beast::error_code ec)
{
	Logger::Get()->Log(samplog_LogLevel::DEBUG, "WebSocket::DoHeartbeat");

	if (ec)
	{
		switch (ec.value())
		{
		case boost::asio::error::operation_aborted:
			// timer was chancelled, do nothing
			Logger::Get()->Log(samplog_LogLevel::DEBUG, "heartbeat timer chancelled");
			break;
		default:
			Logger::Get()->Log(samplog_LogLevel::ERROR, "Heartbeat error: {} ({})",
				ec.message(), ec.value());
			break;
		}
		return;
	}

	json heartbeat_payload = {
		{ "op", 1 },
		{ "d", _sequenceNumber }
	};

	Logger::Get()->Log(samplog_LogLevel::DEBUG, "sending heartbeat");
	Write(heartbeat_payload.dump());

	m_HeartbeatTimer.expires_from_now(m_HeartbeatInterval);
	m_HeartbeatTimer.async_wait(
		beast::bind_front_handler(
			&WebSocket::DoHeartbeat,
			this));
}
