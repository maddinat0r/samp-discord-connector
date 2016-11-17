#pragma once

#include "CSingleton.hpp"
#include "types.hpp"

#include <string>
#include <unordered_map>
#include <functional>
#include <memory>
#include <thread>
#include <chrono>
#include <json.hpp>
#include <beast/http.hpp>
#include <beast/websocket.hpp>
#include <beast/websocket/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>
#include <boost/asio/steady_timer.hpp>


using json = nlohmann::json;
namespace asio = boost::asio;


class CNetwork : public CSingleton<CNetwork>
{
	friend class CSingleton<CNetwork>;
public:
	struct HttpGetResponse
	{
		int status;
		std::string reason;
		std::string body;
		std::string additional_data;
	};
	using HttpGetCallback_t = std::function<void(HttpGetResponse)>;

	enum class WsEvent
	{
		READY,
		RESUMED,
		CHANNEL_CREATE,
		CHANNEL_UPDATE,
		CHANNEL_DELETE,
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
		PRESENCE_UPDATE,
		TYPING_START,
		USER_SETTINGS_UPDATE,
		USER_UPDATE,
		VOICE_STATE_UPDATE,
		VOICE_SERVER_UPDATE
	};
	using WsEventCallback_t = std::function<void(json &)>;

private:
	CNetwork() = default;
	~CNetwork();

private: // variables
	asio::io_service m_IoService;
	std::thread *m_IoThread = nullptr;
	asio::ssl::context m_SslContext{ asio::ssl::context::sslv23 };
	asio::ssl::stream<asio::ip::tcp::socket> m_HttpsStream{ m_IoService, m_SslContext };
	asio::ssl::stream<asio::ip::tcp::socket> m_WssStream{ m_IoService, m_SslContext };
	beast::websocket::stream<decltype(m_WssStream)&> m_WebSocket{ m_WssStream };

	beast::streambuf m_WebSocketBuffer;
	beast::websocket::opcode m_WebSocketOpcode;

	std::string m_GatewayUrl;
	std::string m_Token;
	uint64_t m_SequenceNumber = 0;
	std::string m_SessionId;
	asio::steady_timer m_HeartbeatTimer{ m_IoService };
	std::chrono::steady_clock::duration m_HeartbeatInterval;
	std::unordered_multimap<WsEvent, WsEventCallback_t> m_EventMap;

private: // functions
	bool WsConnect();
	void WsDisconnect();
	void WsIdentify();
	void WsSendResumePayload();
	void WsRead();
	void OnWsRead(boost::system::error_code ec);
	void DoHeartbeat(boost::system::error_code ec);


	using SharedStreambuf_t = std::shared_ptr<beast::streambuf>;
	using SharedResponse_t = std::shared_ptr<beast::http::response<beast::http::streambuf_body>>;
	using HttpReadResponseCallback_t = std::function<void(SharedStreambuf_t, SharedResponse_t)>;

	void HttpWriteRequest(std::string const &method,
		std::string const &url, std::string const &content, std::function<void()> &&callback);
	void HttpReadResponse(HttpReadResponseCallback_t &&callback);

public: // functions
	void Initialize(std::string &&token);

	void HttpGet(std::string const &url, HttpGetCallback_t &&callback);
	void HttpPost(std::string const &url, std::string const &content);

	void WsRegisterEvent(WsEvent event, WsEventCallback_t &&callback)
	{
		m_EventMap.emplace(event, std::move(callback));
	}
};
