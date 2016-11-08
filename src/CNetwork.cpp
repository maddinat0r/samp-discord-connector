#include "CNetwork.hpp"
#include "CLog.hpp"

#include <beast/core/to_string.hpp>
#include <json.hpp>

using json = nlohmann::json;


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
	HttpGet("", "/gateway", [this](HttpGetResponse res)
	{
		if (res.status != 200)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't retrieve Discord gateway URL: {} ({})",
				res.reason, res.status);
			return;
		}
		auto gateway_res = json::parse(res.body);
		std::string url = gateway_res["url"];

		// get rid of protocol
		size_t protocol_pos = url.find("wss://");
		if (protocol_pos != std::string::npos)
			url.erase(protocol_pos, 6); // 6 = length of "wss://"

		m_WssStream.set_verify_mode(asio::ssl::verify_none);

		// connect to gateway
		asio::ip::tcp::resolver r{ m_IoService };
		boost::system::error_code error;
		auto target = r.resolve({ url, "https" }, error);
		if (error)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't resolve Discord gateway URL '{}': {} ({})",
				url, error.message(), error.value());
			return;
		}

		error.clear();
		asio::connect(m_WssStream.lowest_layer(), target, error);
		if (error)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord gateway: {} ({})",
				error.message(), error.value());
			return;
		}

		error.clear();
		m_WssStream.handshake(asio::ssl::stream_base::client, error);
		if (error)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord gateway: {} ({})",
				error.message(), error.value());
			return;
		}

		error.clear();
		m_WebSocket.handshake(url, "/", error);
		if (error)
		{
			CLog::Get()->Log(LogLevel::ERROR, "Can't upgrade to WSS protocol: {} ({})",
				error.message(), error.value());
			return;
		}

		Read();

#ifdef WIN32
		string os_name = "Windows";
#else
		string os_name = "Linux";
#endif

		json identify_payload = {
			{ "op", 2 },
			{ "d", {
				{ "token", m_Token },
				{ "v", 5 },
				{ "compress", false },
				{ "large_threshold", 100 },
				{ "properties", {
					{ "$os", os_name },
					{ "$browser", "boost::asio" },
					{ "$device", "SA-MP DCC plugin" },
					{ "$referrer", "" },
					{ "$referring_domain", "" }
                }}
            }}
		};

		CLog::Get()->Log(LogLevel::DEBUG, "identify payload: {}", identify_payload.dump(4));
		
		m_WebSocket.write(asio::buffer(identify_payload.dump()));
	});

	m_IoThread = new std::thread([this]()
	{ 
		m_IoService.run();
	});
}

CNetwork::~CNetwork()
{
	if (m_IoThread)
	{
		m_IoThread->join();
		delete m_IoThread;
		m_IoThread = nullptr;
	}
}

void CNetwork::Read()
{
	m_WebSocket.async_read(m_WebSocketOpcode, m_WebSocketBuffer,
		std::bind(&CNetwork::OnRead, this, std::placeholders::_1));
}

void CNetwork::OnRead(boost::system::error_code ec)
{
	if (ec)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't read from Discord websocket gateway: {} ({})",
			ec.message(), ec.value());
	}
	else
	{
		json result = json::parse(beast::to_string(m_WebSocketBuffer.data()));
		CLog::Get()->Log(LogLevel::DEBUG, "OnRead: data: {}", result.dump(4));
		m_WebSocketBuffer.consume(m_WebSocketBuffer.size());
	}
	Read();
}


void CNetwork::HttpWriteRequest(std::string const &token, std::string const &method,
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


void CNetwork::HttpGet(const std::string &token, std::string const &url,
	HttpGetCallback_t &&callback)
{
	HttpWriteRequest(token, "GET", url, "", [this, callback]()
	{
		HttpReadResponse([callback](SharedStreambuf_t sb, SharedResponse_t resp)
		{
			callback({ resp->status, resp->reason, beast::to_string(resp->body.data()),
				beast::to_string(sb->data()) });
		});
	});
}

void CNetwork::HttpPost(const std::string &token, std::string const &url, std::string const &content)
{
	HttpWriteRequest(token, "POST", url, content, nullptr);
}
