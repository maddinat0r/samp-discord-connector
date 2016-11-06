#include "CNetwork.hpp"
#include "CLog.hpp"

#include <beast/core/to_string.hpp>
#include <json.hpp>

using json = nlohmann::json;


CNetwork::CNetwork()
{
	m_HttpsStream.set_verify_mode(asio::ssl::verify_none);

	// connect to REST API
	asio::ip::tcp::resolver r{ m_IoService };
	asio::connect(m_HttpsStream.lowest_layer(),
		r.resolve(boost::asio::ip::tcp::resolver::query{ "discordapp.com/api", "https" }));

	// SSL handshake
	m_HttpsStream.handshake(asio::ssl::stream_base::client);

	// retrieve WebSocket host URL
	HttpGet("", "/gateway", [this](HttpGetResponse res)
	{
		auto json = json::parse(res.body);
		std::string url = json["url"];

		m_WssStream.set_verify_mode(asio::ssl::verify_none);

		asio::ip::tcp::resolver r{ m_IoService };
		asio::connect(m_WebSocket.next_layer().lowest_layer(),
			r.resolve(asio::ip::tcp::resolver::query{ url, "wss" }));
		m_WssStream.handshake(asio::ssl::stream_base::client);
		m_WebSocket.handshake(url, "/");
	});
}

CNetwork::~CNetwork()
{
	
}

void CNetwork::HttpWriteRequest(std::string const &token, std::string const &method,
	std::string const &url, std::string const &content, std::function<void()> &&callback)
{
	beast::http::request<beast::http::string_body> req;
	req.method = method;
	req.url = url;
	req.version = 11;
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
			callback({ resp->status, resp->reason, beast::to_string(sb->data()) });
		});
	});
}

void CNetwork::HttpPost(const std::string &token, std::string const &url, std::string const &content)
{
	HttpWriteRequest(token, "POST", url, content, nullptr);
}
