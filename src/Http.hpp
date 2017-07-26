#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <deque>

#include <boost/asio.hpp>
#include <boost/asio/ssl.hpp>

#include <boost/beast/core.hpp>
#include <boost/beast/http.hpp>


namespace asio = boost::asio;
namespace beast = boost::beast;


class Http
{
public:
	struct GetResponse
	{
		unsigned int status;
		std::string reason;
		std::string body;
		std::string additional_data;
	};
	using GetCallback_t = std::function<void(GetResponse)>;

public:
	Http(asio::io_service &io_service, std::string token);
	~Http();

private:
	using Streambuf_t = beast::flat_buffer;
	using SharedStreambuf_t = std::shared_ptr<Streambuf_t>;
	using Response_t = beast::http::response<beast::http::dynamic_body>;
	using SharedResponse_t = std::shared_ptr<Response_t>;
	using Request_t = beast::http::request<beast::http::string_body>;
	using SharedRequest_t = std::shared_ptr<Request_t>;
	using ResponseCallback_t = std::function<void(Streambuf_t&, Response_t&)>;

private:
	asio::io_service &m_IoService; // TODO: don't use ioservice itself here
	asio::ssl::context m_SslContext;
	asio::ssl::stream<asio::ip::tcp::socket> m_HttpsStream;

	std::string m_Token;
	std::unordered_map<
		std::string,
		std::deque<std::tuple<SharedRequest_t, ResponseCallback_t>>>
		m_PathRateLimit;

private: // functions
	bool Connect();
	void Disconnect();
	void ReconnectRetry(SharedRequest_t request, ResponseCallback_t &&callback);

	SharedRequest_t PrepareRequest(beast::http::verb const method,
		std::string const &url, std::string const &content);
	void WriteRequest(SharedRequest_t request, ResponseCallback_t &&callback);
	void SendRequest(beast::http::verb const method, std::string const &url,
		std::string const &content, ResponseCallback_t &&callback);
	void SendRequest(SharedRequest_t request, ResponseCallback_t &&callback);

public: // functions
	void Get(std::string const &url, GetCallback_t &&callback);
	void Post(std::string const &url, std::string const &content);
};
