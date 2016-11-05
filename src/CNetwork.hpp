#pragma once

#include "CSingleton.hpp"

#include <string>
#include <functional>
#include <boost/asio.hpp>




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
	void HttpRequest(const std::string &token, std::string const &method, 
		std::string const &url, std::string const &content);

public: // functions
	void HttpGet(const std::string &token, std::string const &url,
		HttpGetCallback_t &&callback);
	void HttpPost(const std::string &token, std::string const &url, 
		std::string const &content);
};
