#pragma once

#include <string>
#include <functional>
#include <chrono>
#include <map>
#include <thread>
#include <memory>

#include <json.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/beast/core.hpp>
#include <boost/beast/websocket.hpp>
#include <boost/beast/websocket/ssl.hpp>

using json = nlohmann::json;
namespace asio = boost::asio;
namespace beast = boost::beast;

class WebSocket
{
	friend class Network;
public:
	enum class Event
	{
		READY,
		RESUMED,
		CHANNEL_CREATE,
		CHANNEL_UPDATE,
		CHANNEL_DELETE,
		CHANNEL_PINS_UPDATE,
		GUILD_CREATE,
		GUILD_UPDATE,
		GUILD_DELETE,
		GUILD_BAN_ADD,
		GUILD_BAN_REMOVE,
		GUILD_EMOJIS_UPDATE,
		GUILD_INTEGRATIONS_UPDATE,
		GUILD_MEMBER_ADD,
		GUILD_MEMBER_REMOVE,
		GUILD_MEMBER_UPDATE,
		GUILD_MEMBERS_CHUNK,
		GUILD_ROLE_CREATE,
		GUILD_ROLE_UPDATE,
		GUILD_ROLE_DELETE,
		MESSAGE_CREATE,
		MESSAGE_UPDATE,
		MESSAGE_DELETE,
		MESSAGE_DELETE_BULK,
		MESSAGE_REACTION_ADD,
		MESSAGE_REACTION_REMOVE,
		MESSAGE_REACTION_REMOVE_ALL,
		PRESENCE_UPDATE,
		TYPING_START,
		USER_UPDATE,
		VOICE_STATE_UPDATE,
		VOICE_SERVER_UPDATE,
		WEBHOOKS_UPDATE,
	};
	using EventCallback_t = std::function<void(json &)>;

private:
	WebSocket();

public:
	~WebSocket();

private: // variables
	asio::io_service m_IoService;
	std::thread *m_IoThread = nullptr;
	asio::ssl::context m_SslContext;
	using SslStream_t = asio::ssl::stream<asio::ip::tcp::socket>;
	using WebSocketStream_t = beast::websocket::stream<SslStream_t>;
	std::unique_ptr<WebSocketStream_t> m_WebSocket;

	beast::multi_buffer m_WebSocketBuffer;

	std::string m_Token;
	std::string m_GatewayUrl;
	uint64_t m_SequenceNumber = 0;
	std::string m_SessionId;
	asio::steady_timer m_HeartbeatTimer;
	std::chrono::steady_clock::duration m_HeartbeatInterval;
	std::multimap<Event, EventCallback_t> m_EventMap;

private: // functions
	void Initialize(std::string token, std::string gateway_url);
	bool Connect();
	void Disconnect(bool reconnect = false);
	void OnClose(boost::system::error_code ec, bool reconnect);
	void Identify();
	void SendResumePayload();
	void Read();
	void OnRead(boost::system::error_code ec);
	void DoHeartbeat(boost::system::error_code ec);

public: // functions
	void RegisterEvent(Event event, EventCallback_t &&callback)
	{
		m_EventMap.emplace(event, std::move(callback));
	}
};
