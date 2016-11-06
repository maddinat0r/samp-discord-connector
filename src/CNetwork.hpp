#pragma once

#include "CSingleton.hpp"

#include <string>
#include <functional>
#include <memory>
#include <beast/http.hpp>
#include <beast/websocket.hpp>
#include <beast/websocket/ssl.hpp>
#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>


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
	};
	using HttpGetCallback_t = std::function<void(HttpGetResponse)>;

private:
	CNetwork();
	~CNetwork();

private: // variables
	asio::io_service m_IoService;
	asio::ssl::context m_SslContext{ asio::ssl::context::sslv23 };
	asio::ssl::stream<asio::ip::tcp::socket> m_HttpsStream{ m_IoService, m_SslContext };
	asio::ssl::stream<asio::ip::tcp::socket> m_WssStream{ m_IoService, m_SslContext };
	beast::websocket::stream<decltype(m_WssStream)&> m_WebSocket{ m_WssStream };

private: // functions
	using SharedStreambuf_t = std::shared_ptr<beast::streambuf>;
	using SharedResponse_t = std::shared_ptr<beast::http::response<beast::http::streambuf_body>>;
	using HttpReadResponseCallback_t = std::function<void(SharedStreambuf_t, SharedResponse_t)>;

	void HttpWriteRequest(const std::string &token, std::string const &method,
		std::string const &url, std::string const &content, std::function<void()> &&callback);
	void HttpReadResponse(HttpReadResponseCallback_t &&callback);

public: // functions
	void HttpGet(const std::string &token, std::string const &url,
		HttpGetCallback_t &&callback);
	void HttpPost(const std::string &token, std::string const &url, 
		std::string const &content);
};
