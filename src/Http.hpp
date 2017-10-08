#pragma once

#include <string>
#include <functional>
#include <memory>
#include <unordered_map>
#include <thread>
#include <atomic>
#include <mutex>
#include <list>

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
	Http(std::string token);
	~Http();

private:
	using Streambuf_t = beast::flat_buffer;
	using SharedStreambuf_t = std::shared_ptr<Streambuf_t>;
	using Response_t = beast::http::response<beast::http::dynamic_body>;
	using SharedResponse_t = std::shared_ptr<Response_t>;
	using Request_t = beast::http::request<beast::http::string_body>;
	using SharedRequest_t = std::shared_ptr<Request_t>;
	using ResponseCallback_t = std::function<void(Streambuf_t&, Response_t&)>;
	using TimePoint_t = std::chrono::system_clock::time_point;

	struct QueueEntry
	{
		QueueEntry(SharedRequest_t req, ResponseCallback_t &&cb) :
			Request(req),
			Callback(cb)
		{ }
		SharedRequest_t Request;
		ResponseCallback_t Callback;
	};
	using QueueEntry_t = std::shared_ptr<QueueEntry>;

private:
	asio::io_service m_IoService;
	asio::ssl::context m_SslContext;
	using SslStream_t = asio::ssl::stream<asio::ip::tcp::socket>;
	std::unique_ptr<SslStream_t> m_SslStream;

	std::string m_Token;

	std::mutex m_QueueMutex;
	std::list<QueueEntry_t> m_Queue;
	std::atomic<bool> m_NetworkThreadRunning;
	std::thread m_NetworkThread;

private: // functions
	void NetworkThreadFunc();

	bool Connect();
	void Disconnect();
	bool ReconnectRetry();

	SharedRequest_t PrepareRequest(beast::http::verb const method,
		std::string const &url, std::string const &content);
	void SendRequest(beast::http::verb const method, std::string const &url,
		std::string const &content, ResponseCallback_t &&callback);

public: // functions
	void Get(std::string const &url, GetCallback_t &&callback);
	void Post(std::string const &url, std::string const &content);
};
