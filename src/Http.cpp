#include "Http.hpp"
#include "CLog.hpp"
#include "version.hpp"

#include <thread>

#include <boost/asio/system_timer.hpp>
#include <boost/spirit/include/qi.hpp>


Http::Http(asio::io_service &io_service, std::string token) :
	m_IoService(io_service),
	m_SslContext(asio::ssl::context::sslv23),
	m_HttpsStream(io_service, m_SslContext),
	m_Token(token)
{
	if (!Connect())
		return;
}

Http::~Http()
{

}

bool Http::Connect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::Connect");

	// connect to REST API
	asio::ip::tcp::resolver r{ m_IoService };
	boost::system::error_code error;
	auto target = r.resolve({ "discordapp.com", "https" }, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't resolve Discord API URL: {} ({})",
			error.message(), error.value());
		return false;
	}

	error.clear();
	asio::connect(m_HttpsStream.lowest_layer(), target, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	// SSL handshake
	error.clear();
	m_HttpsStream.set_verify_mode(asio::ssl::verify_none); // TODO error check
	m_HttpsStream.handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	return true;
}

void Http::Disconnect()
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::Disconnect");

	boost::system::error_code error;
	m_HttpsStream.shutdown(error);
	if (error && error != boost::asio::error::eof)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down SSL on HTTP connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_HttpsStream.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, error);
	if (error && error != boost::asio::error::eof)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down HTTP connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_HttpsStream.lowest_layer().close(error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while closing HTTP connection: {} ({})",
			error.message(), error.value());
	}
}
Http::SharedRequest_t Http::PrepareRequest(beast::http::verb const method,
	std::string const &url, std::string const &content)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::PrepareRequest");

	auto req = std::make_shared<Request_t>();
	req->method(method);
	req->target("/api/v5" + url);
	req->version = 11;
	req->insert("Host", "discordapp.com");
	req->insert("User-Agent", "DiscordBot (github.com/maddinat0r/samp-discord-connector, " PLUGIN_VERSION ")");
	if (!content.empty())
		req->insert("Content-Type", "application/json");
	req->insert("Authorization", "Bot " + m_Token);
	req->body = content;

	req->prepare();

	return req;
}

void Http::ReconnectRetry(SharedRequest_t request, ResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::ReconnectRetry");

	CLog::Get()->Log(LogLevel::INFO, "trying reconnect...");

	Disconnect();
	if (Connect())
	{
		CLog::Get()->Log(LogLevel::INFO, "reconnect succeeded, resending request");
		SendRequest(request, std::move(callback));
	}
	else
	{
		CLog::Get()->Log(LogLevel::WARNING, "reconnect failed, discarding request");
	}
}

void Http::WriteRequest(SharedRequest_t request, ResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::WriteRequest");

	boost::system::error_code error_code;
	beast::http::write(m_HttpsStream, *request, error_code);
	if (error_code)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Error while sending HTTP {} request to '{}': {}",
			request->method_string().to_string(), request->target().to_string(), error_code.message());
		ReconnectRetry(request, std::move(callback));
		return;
	}

	error_code.clear();

	Streambuf_t sb;
	Response_t response;
	beast::http::read(m_HttpsStream, sb, response, error_code);
	if (error_code)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Error while retrieving HTTP response: {}",
			error_code.message());
		ReconnectRetry(request, std::move(callback));
		return;
	}

	auto it = response.find("X-RateLimit-Remaining");
	if (it != response.end())
	{
		if (it->value().compare("0"))
		{
			std::string limited_url = request->target().to_string();
			auto lit = m_PathRateLimit.find(limited_url);
			if (lit != m_PathRateLimit.end())
			{
				// we sent the message to early, we're still rate-limited
				// hack in that message back into the queue and wait a little bit
				lit->second.push_front(std::make_tuple(request, std::move(callback)));
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				return;
			}

			it = response.find("X-RateLimit-Reset");
			if (it != response.end())
			{
				std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
				CLog::Get()->Log(LogLevel::DEBUG, "rate-limiting path {} until {} (current time: {})",
					request->target().to_string(), it->value().to_string(), std::chrono::duration_cast<std::chrono::seconds>(current_time.time_since_epoch()).count());
				m_PathRateLimit.insert({ limited_url, decltype(m_PathRateLimit)::mapped_type() }).first->second;

				string const &reset_time_str = it->value().to_string();
				long long reset_time_secs = 0;
				boost::spirit::qi::parse(reset_time_str.begin(), reset_time_str.end(),
					boost::spirit::qi::any_int_parser<long long>(),
					reset_time_secs);
				std::chrono::system_clock::time_point reset_time{ std::chrono::seconds(reset_time_secs) + std::chrono::seconds(1) };
				auto timer = std::make_shared<asio::system_timer>(m_IoService, reset_time);
				timer->async_wait([timer, this, limited_url](const boost::system::error_code &ec)
				{
					if (ec)
						return;

					CLog::Get()->Log(LogLevel::DEBUG, "removing rate-limit on path {}", limited_url);

					auto it = m_PathRateLimit.find(limited_url);
					if (it == m_PathRateLimit.end())
					{
						CLog::Get()->Log(LogLevel::ERROR, "attempted to remove rate-limit on path {}, but it doesn't exist", limited_url);
						return; // ????
					}

					auto &query = it->second;
					while (!query.empty())
					{
						auto req_data = query.front();
						query.pop_front();
						m_IoService.post([this, req_data]() mutable
						{
							SendRequest(std::get<0>(req_data), std::move(std::get<1>(req_data)));
						});
					}
					m_PathRateLimit.erase(it);
				});
			}
		}
	}

	if (callback)
		callback(sb, response);

}


void Http::SendRequest(beast::http::verb const method,
	std::string const &url, std::string const &content, ResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::SendRequest");

	SharedRequest_t req = PrepareRequest(method, url, content);
	m_IoService.dispatch([this, req, callback]() mutable
	{
		SendRequest(req, std::move(callback));
	});
}

void Http::SendRequest(SharedRequest_t request, ResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::SendRequest({}) (actual send)", request->target().to_string());
	// check if this URL path is currently rate-limited
	auto it = m_PathRateLimit.find(request->target().to_string());
	if (it != m_PathRateLimit.end())
	{
		// yes, it is; queue request
		it->second.push_back(std::make_tuple(request, std::move(callback)));
	}
	else
	{
		// no it isn't, write/execute request
		WriteRequest(request, std::move(callback));
	}
}

void Http::Get(std::string const &url, GetCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::Get");

	SendRequest(beast::http::verb::get, url, "", [callback](Streambuf_t &sb, Response_t &resp)
	{
		std::stringstream ss_body, ss_data;
		ss_body << beast::buffers(resp.body.data());
		ss_data << beast::buffers(sb.data());
		callback({ resp.result_int(), resp.reason().to_string(), ss_body.str(), ss_data.str() });
	});
}

void Http::Post(std::string const &url, std::string const &content)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::Post");

	SendRequest(beast::http::verb::post, url, content, nullptr);
}
