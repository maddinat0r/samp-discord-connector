#pragma once

#include "CSingleton.hpp"

#include <string>
#include <functional>
#include <memory>
#include <boost/asio.hpp>
#include <beast/http.hpp>




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
	boost::asio::io_service m_IoService;
	boost::asio::ip::tcp::socket m_HttpSocket;

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
