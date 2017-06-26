#include "CNetwork.hpp"
#include "CLog.hpp"
#include "version.hpp"

#include <boost/spirit/include/qi.hpp>
#include <boost/asio/system_timer.hpp>

#include <unordered_map>
#include <functional>


void CNetwork::Initialize(std::string &&token)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::Initialize");

	m_Token = std::move(token);

	if (!HttpConnect())
		return;

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
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::~CNetwork");

	HttpDisconnect();
	WsDisconnect();

	if (m_IoThread)
	{
		m_IoThread->join();
		delete m_IoThread;
		m_IoThread = nullptr;
	}
}

bool CNetwork::HttpConnect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpConnect");

	// connect to REST API
	asio::ip::tcp::resolver r{ m_IoService };
	boost::system::error_code error;
	auto target = r.resolve({ "discordapp.com", "https" }, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't resolve Discord API URL: {} ({})",
			error.message(), error.value());
		return false;
	}

	error.clear();
	asio::connect(m_HttpsStream.lowest_layer(), target, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	// SSL handshake
	error.clear();
	m_HttpsStream.set_verify_mode(asio::ssl::verify_none); // TODO error check
	m_HttpsStream.handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	return true;
}

void CNetwork::HttpDisconnect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpDisconnect");

	boost::system::error_code error;
	m_HttpsStream.shutdown(error);
	if (error && error != boost::asio::error::eof)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down SSL on HTTP connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_HttpsStream.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, error);
	if (error && error != boost::asio::error::eof)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down HTTP connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_HttpsStream.lowest_layer().close(error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while closing HTTP connection: {} ({})",
			error.message(), error.value());
	}
}

bool CNetwork::WsConnect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::WsConnect");

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
	m_WssStream.set_verify_mode(asio::ssl::verify_none); // TODO error check
	m_WssStream.handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord gateway: {} ({})",
			error.message(), error.value());
		return false;
	}

	error.clear();
	m_WebSocket.handshake(m_GatewayUrl, "/?encoding=json&v=5", error);
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
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::WsDisconnect");

	boost::system::error_code error;
	m_WebSocket.close(beast::websocket::close_code::normal, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while sending WS close frame: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_WssStream.shutdown(error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down SSL on WS connection: {} ({})",
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
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::WsIdentify");

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

	m_WebSocket.write(asio::buffer(identify_payload.dump()));
}

void CNetwork::WsSendResumePayload()
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::WsSendResumePayload");

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
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::WsRead");

	m_WebSocket.async_read(m_WebSocketOpcode, m_WebSocketBuffer,
		std::bind(&CNetwork::OnWsRead, this, std::placeholders::_1));
}

void CNetwork::OnWsRead(boost::system::error_code ec)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::OnWsRead");

	if (ec)
	{
		switch (ec.value())
		{
			case boost::asio::ssl::error::stream_errors::stream_truncated:
				CLog::Get()->Log(LogLevel::INFO,
					"Discord terminated websocket gateway connection, attempting reconnect...");
				WsReconnect();
				WsRead();
				break;
			case boost::asio::error::operation_aborted:
				// connection was closed, do nothing
				break;
			default:
				CLog::Get()->Log(LogLevel::ERROR, "Can't read from Discord websocket gateway: {} ({})",
					ec.message(), ec.value());
				CLog::Get()->Log(LogLevel::INFO,
					"websocket gateway connection terminated, attempting reconnect...");
				WsReconnect();
				WsRead();
				break;
		}
		return;
	}
	
	std::stringstream ss;
	ss << beast::buffers(m_WebSocketBuffer.data());
	json result = json::parse(ss.str());
	m_WebSocketBuffer.consume(m_WebSocketBuffer.size());
	CLog::Get()->Log(LogLevel::DEBUG, "OnWsRead: {}", result.dump(4));

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
				{ "MESSAGE_REACTION_ADD", WsEvent::MESSAGE_REACTION_ADD },
				{ "MESSAGE_REACTION_REMOVE", WsEvent::MESSAGE_REACTION_REMOVE },
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
			WsReconnect();
			break;
		case 9: // invalid session
			WsIdentify();
			break;
		case 10: // hello
			// start heartbeat
			m_HeartbeatInterval = std::chrono::milliseconds(result["d"]["heartbeat_interval"]);
			DoHeartbeat({ });
			break;
		case 11: // heartbeat ACK
			CLog::Get()->Log(LogLevel::DEBUG, "heartbeat ACK");
			break;
		default:
			CLog::Get()->Log(LogLevel::WARNING, "Unhandled payload opcode '{}'", payload_opcode);
			CLog::Get()->Log(LogLevel::DEBUG, "UPO res: {}", result.dump(4));
	}

	WsRead();
}

void CNetwork::DoHeartbeat(boost::system::error_code ec)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::DoHeartbeat");

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
	m_WebSocket.write(asio::buffer(heartbeat_payload.dump()), error_code);
	if (error_code)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Heartbeat write error: {} ({})",
			ec.message(), ec.value());
		return;
	}

	CLog::Get()->Log(LogLevel::DEBUG, "sending heartbeat");

	m_HeartbeatTimer.expires_from_now(m_HeartbeatInterval);
	m_HeartbeatTimer.async_wait(std::bind(&CNetwork::DoHeartbeat, this, std::placeholders::_1));
}

CNetwork::SharedRequest_t CNetwork::HttpPrepareRequest(beast::http::verb const method,
	std::string const &url, std::string const &content)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpPrepareRequest");

	auto req = std::make_shared<Request_t>();
	req->method(method);
	req->target("/api/v5" + url);
	req->version = 11;
	req->insert("Host", "discordapp.com");
	req->insert("User-Agent", "DiscordBot (github.com/maddinat0r/samp-discord-connector, " PLUGIN_VERSION ")");
	if (!content.empty())
		req->insert("Content-Type", "application/json");
	req->insert("Authorization", "Bot " + m_Token);
	req->body = content;

	req->prepare();

	return req;
}

void CNetwork::HttpReconnectRetry(SharedRequest_t request, HttpResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpReconnectRetry");

	CLog::Get()->Log(LogLevel::INFO, "trying reconnect...");

	HttpDisconnect();
	if (HttpConnect())
	{
		CLog::Get()->Log(LogLevel::INFO, "reconnect succeeded, resending request");
		HttpSendRequest(request, std::move(callback));
	}
	else
	{
		CLog::Get()->Log(LogLevel::WARNING, "reconnect failed, discarding request");
	}
}

void CNetwork::HttpWriteRequest(SharedRequest_t request, HttpResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpWriteRequest");

	boost::system::error_code error_code;
	beast::http::write(m_HttpsStream, *request, error_code);
	if (error_code)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Error while sending HTTP {} request to '{}': {}",
			request->method_string().to_string(), request->target().to_string(), error_code.message());
		HttpReconnectRetry(request, std::move(callback));
		return;
	}

	error_code.clear();

	Streambuf_t sb;
	Response_t response;
	beast::http::read(m_HttpsStream, sb, response, error_code);
	if (error_code)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Error while retrieving HTTP response: {}",
			error_code.message());
		HttpReconnectRetry(request, std::move(callback));
		return;
	}

	auto it = response.find("X-RateLimit-Remaining");
	if (it != response.end())
	{
		if (it->value().compare("0"))
		{
			std::string limited_url = request->target().to_string();
			auto lit = m_PathRateLimit.find(limited_url);
			if (lit != m_PathRateLimit.end())
			{
				// we sent the message to early, we're still rate-limited
				// hack in that message back into the queue and wait a little bit
				lit->second.push_front(std::make_tuple(request, std::move(callback)));
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				return;
			}

			it = response.find("X-RateLimit-Reset");
			if (it != response.end())
			{
				std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
				CLog::Get()->Log(LogLevel::DEBUG, "rate-limiting path {} until {} (current time: {})",
					request->target().to_string(), it->value().to_string(), std::chrono::duration_cast<std::chrono::seconds>(current_time.time_since_epoch()).count());
				m_PathRateLimit.insert({ limited_url, decltype(m_PathRateLimit)::mapped_type() }).first->second;

				string const &reset_time_str = it->value().to_string();
				long long reset_time_secs = 0;
				boost::spirit::qi::parse(reset_time_str.begin(), reset_time_str.end(),
					boost::spirit::qi::any_int_parser<long long>(),
					reset_time_secs);
				std::chrono::system_clock::time_point reset_time{ std::chrono::seconds(reset_time_secs) + std::chrono::seconds(1) };
				auto timer = std::make_shared<asio::system_timer>(m_IoService, reset_time);
				timer->async_wait([timer, this, limited_url](const boost::system::error_code &ec)
				{
					if (ec)
						return;

					CLog::Get()->Log(LogLevel::DEBUG, "removing rate-limit on path {}", limited_url);

					auto it = m_PathRateLimit.find(limited_url);
					if (it == m_PathRateLimit.end())
					{
						CLog::Get()->Log(LogLevel::ERROR, "attempted to remove rate-limit on path {}, but it doesn't exist", limited_url);
						return; // ????
					}

					auto &query = it->second;
					while (!query.empty())
					{
						auto req_data = query.front();
						query.pop_front();
						m_IoService.post([this, req_data]() mutable
						{
							HttpSendRequest(std::get<0>(req_data), std::move(std::get<1>(req_data)));
						});
					}
					m_PathRateLimit.erase(it);
				});
			}
		}
	}

	if (callback)
		callback(sb, response);
			
}


void CNetwork::HttpSendRequest(beast::http::verb const method,
	std::string const &url, std::string const &content, HttpResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpSendRequest");

	SharedRequest_t req = HttpPrepareRequest(method, url, content);
	m_IoService.dispatch([this, req, callback]() mutable
	{
		HttpSendRequest(req, std::move(callback));
	});
}

void CNetwork::HttpSendRequest(SharedRequest_t request, HttpResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpSendRequest({}) (actual send)", request->target().to_string());
	// check if this URL path is currently rate-limited
	auto it = m_PathRateLimit.find(request->target().to_string());
	if (it != m_PathRateLimit.end())
	{
		// yes, it is; queue request
		it->second.push_back(std::make_tuple(request, std::move(callback)));
	}
	else
	{
		// no it isn't, write/execute request
		HttpWriteRequest(request, std::move(callback));
	}
}

void CNetwork::HttpGet(std::string const &url, HttpGetCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpGet");

	HttpSendRequest(beast::http::verb::get, url, "", [callback](Streambuf_t &sb, Response_t &resp)
	{
		std::stringstream ss_body, ss_data;
		ss_body << beast::buffers(resp.body.data());
		ss_data << beast::buffers(sb.data());
		callback({ resp.result_int(), resp.reason().to_string(), ss_body.str(), ss_data.str() });
	});
}

void CNetwork::HttpPost(std::string const &url, std::string const &content)
{
	CLog::Get()->Log(LogLevel::DEBUG, "CNetwork::HttpPost");

	HttpSendRequest(beast::http::verb::post, url, content, nullptr);
}
