#include "CNetwork.hpp"

#include <memory>
#include <beast/http.hpp>
#include <beast/core/to_string.hpp>


CNetwork::CNetwork() :
	m_HttpSocket(m_IoService)
{
	boost::asio::ip::tcp::resolver r{ m_IoService };
	boost::asio::connect(m_HttpSocket,
		r.resolve(boost::asio::ip::tcp::resolver::query{ "https://discordapp.com/api", "http" }));

}

CNetwork::~CNetwork()
{
	m_HttpSocket.close();
}

void CNetwork::HttpRequest(std::string const &token, std::string const &method, 
	std::string const &url, std::string const &content)
{

}

void CNetwork::HttpGet(const std::string &token, std::string const &url,
	HttpGetCallback_t &&callback)
{
	beast::http::request<beast::http::empty_body> req;
	req.method = "GET";
	req.url = url;
	req.version = 11;
	req.headers.replace("Authorization", "Bot " + token);
	beast::http::prepare(req);
	beast::http::async_write(
		m_HttpSocket,
		req, 
		[this, callback](boost::system::error_code ec)
		{
			if (!ec)
				return ;

			auto sb = std::make_shared<beast::streambuf>();
			auto response = std::make_shared<beast::http::response<beast::http::streambuf_body>>();
			beast::http::async_read(
				m_HttpSocket, 
				*sb, 
				*response,
				[this, callback, sb, response](boost::system::error_code ec)
				{
					if (!ec)
						return ;

					callback({ response->status, response->reason, beast::to_string(sb->data()) });
				}
			);
		}
	);
}

void CNetwork::HttpPost(const std::string &token, std::string const &url, std::string const &content)
{
	beast::http::request<beast::http::string_body> req;
	req.method = "POST";
	req.url = url;
	req.version = 11;
	req.headers.replace("Authorization", "Bot " + token);
	req.body = content;
	beast::http::prepare(req);
	beast::http::async_write(
		m_HttpSocket,
		req,
		[this](boost::system::error_code ec)
		{
			if (!ec)
				return ;

		}
	);
}
