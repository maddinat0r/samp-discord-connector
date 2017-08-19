#include "Http.hpp"
#include "CLog.hpp"
#include "version.hpp"

#include <boost/asio/system_timer.hpp>
#include <boost/spirit/include/qi.hpp>


Http::Http(std::string token) :
	m_SslContext(asio::ssl::context::sslv23),
	m_SslStream(m_IoService, m_SslContext),
	m_Token(token),
	m_NetworkThreadRunning(true),
	m_NetworkThread(std::bind(&Http::NetworkThreadFunc, this))
{
	if (!Connect())
		return;
}

Http::~Http()
{
	m_NetworkThreadRunning = false;
	m_NetworkThread.join();
}

void Http::NetworkThreadFunc()
{
	std::unordered_map<std::string, TimePoint_t> path_ratelimit;
	unsigned int retry_counter = 0;
	unsigned int const MaxRetries = 3;
	bool skip_entry = false;

	while (m_NetworkThreadRunning)
	{
		TimePoint_t current_time = std::chrono::system_clock::now();
		
		m_QueueMutex.lock(); 
		auto it = m_Queue.begin();
		while (it != m_Queue.end())
		{
			auto const &entry = *it;

			// check if we're rate-limited
			auto pr_it = path_ratelimit.find(entry->Request->target().to_string());
			if (pr_it != path_ratelimit.end())
			{
				// rate-limit for this path exists
				// are we still within the rate-limit timepoint?
				if (current_time < pr_it->second)
				{
					// yes, ignore this request for now
					it++;
					continue;
				}

				// no, delete rate-limit and go on
				path_ratelimit.erase(pr_it);
			}

			boost::system::error_code error_code;
			retry_counter = 0;
			skip_entry = false;
			do
			{
				beast::http::write(m_SslStream, *entry->Request, error_code);
				if (error_code)
				{
					CLog::Get()->Log(LogLevel::ERROR, "Error while sending HTTP {} request to '{}': {}",
						entry->Request->method_string().to_string(),
						entry->Request->target().to_string(),
						error_code.message());

					// try reconnecting
					if (retry_counter++ >= MaxRetries || !ReconnectRetry())
					{
						// we failed to reconnect, discard this request
						CLog::Get()->Log(LogLevel::WARNING, "Failed to send request, discarding");
						it = m_Queue.erase(it);
						skip_entry = true;
						break; // break out of do-while loop
					}
				}
			} while (error_code);
			if (skip_entry)
				continue; // continue queue loop

			Streambuf_t sb;
			Response_t response;
			retry_counter = 0;
			skip_entry = false;
			do
			{
				beast::http::read(m_SslStream, sb, response, error_code);
				if (error_code)
				{
					CLog::Get()->Log(LogLevel::ERROR, "Error while retrieving HTTP {} response from '{}': {}",
						entry->Request->method_string().to_string(),
						entry->Request->target().to_string(),
						error_code.message());

					// try reconnecting
					if (retry_counter++ >= MaxRetries || !ReconnectRetry())
					{
						// we failed to reconnect, discard this request
						CLog::Get()->Log(LogLevel::WARNING, "Failed to read response, discarding");
						it = m_Queue.erase(it);
						skip_entry = true;
						break; // break out of do-while loop
					}
				}
			} while (error_code);
			if (skip_entry)
				continue; // continue queue loop

			auto it_r = response.find("X-RateLimit-Remaining");
			if (it_r != response.end())
			{
				if (it_r->value().compare("0") == 0)
				{
					// we're now officially rate-limited
					// the next call to this path will fail
					std::string limited_url = entry->Request->target().to_string();
					auto lit = path_ratelimit.find(limited_url);
					if (lit != path_ratelimit.end())
					{
						CLog::Get()->Log(LogLevel::ERROR, 
							"Error while processing rate-limit: already rate-limited path '{}'",
							limited_url);
						return; // TODO: return is evil
					}

					it_r = response.find("X-RateLimit-Reset");
					if (it_r != response.end())
					{
						TimePoint_t current_time = std::chrono::system_clock::now();
						CLog::Get()->Log(LogLevel::DEBUG, "rate-limiting path {} until {} (current time: {})",
							limited_url,
							it_r->value().to_string(),
							std::chrono::duration_cast<std::chrono::seconds>(
								current_time.time_since_epoch()).count());

						string const &reset_time_str = it_r->value().to_string();
						long long reset_time_secs = 0;
						boost::spirit::qi::parse(reset_time_str.begin(), reset_time_str.end(),
							boost::spirit::qi::any_int_parser<long long>(),
							reset_time_secs);
						TimePoint_t reset_time{ std::chrono::seconds(reset_time_secs) };

						path_ratelimit.insert({ limited_url, reset_time });
					}
				}
			}

			m_QueueMutex.unlock(); // allow requests to be queued from within callback
			if (entry->Callback)
				entry->Callback(sb, response);
			m_QueueMutex.lock();

			it = m_Queue.erase(it);
		}
		m_QueueMutex.unlock();

		std::this_thread::sleep_for(std::chrono::milliseconds(10));
	}

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
	asio::connect(m_SslStream.lowest_layer(), target, error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::ERROR, "Can't connect to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	// SSL handshake
	error.clear();
	m_SslStream.set_verify_mode(asio::ssl::verify_none); // TODO error check
	m_SslStream.handshake(asio::ssl::stream_base::client, error);
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
	m_SslStream.shutdown(error);
	if (error && error != boost::asio::error::eof)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down SSL on HTTP connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_SslStream.lowest_layer().shutdown(asio::ip::tcp::socket::shutdown_both, error);
	if (error && error != boost::asio::error::eof)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while shutting down HTTP connection: {} ({})",
			error.message(), error.value());
	}

	error.clear();
	m_SslStream.lowest_layer().close(error);
	if (error)
	{
		CLog::Get()->Log(LogLevel::WARNING, "Error while closing HTTP connection: {} ({})",
			error.message(), error.value());
	}
}

bool Http::ReconnectRetry()
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::ReconnectRetry");

	unsigned int reconnect_counter = 0;
	do
	{
		CLog::Get()->Log(LogLevel::INFO, "trying reconnect #{}...", reconnect_counter + 1);

		Disconnect();
		if (Connect())
		{
			CLog::Get()->Log(LogLevel::INFO, "reconnect succeeded, resending request");
			return true;
		}
		else
		{
			unsigned int seconds_to_wait = static_cast<unsigned int>(std::pow(2U, reconnect_counter));
			CLog::Get()->Log(LogLevel::WARNING, "reconnect failed, waiting {} seconds...", seconds_to_wait);
			std::this_thread::sleep_for(std::chrono::seconds(seconds_to_wait));
		}
	} while (++reconnect_counter < 3);
	
	CLog::Get()->Log(LogLevel::ERROR, "Could not reconnect to Discord");
	return false;
}

Http::SharedRequest_t Http::PrepareRequest(beast::http::verb const method,
	std::string const &url, std::string const &content)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::PrepareRequest");

	auto req = std::make_shared<Request_t>();
	req->method(method);
	req->target("/api/v6" + url);
	req->version = 11;
	req->insert("Host", "discordapp.com");
	req->insert("User-Agent", "DiscordBot (github.com/maddinat0r/samp-discord-connector, " PLUGIN_VERSION ")");
	if (!content.empty())
		req->insert("Content-Type", "application/json");
	req->insert("Authorization", "Bot " + m_Token);
	req->body = content;

	req->prepare_payload();

	return req;
}

void Http::SendRequest(beast::http::verb const method, std::string const &url, 
	std::string const &content, ResponseCallback_t &&callback)
{
	CLog::Get()->Log(LogLevel::DEBUG, "Http::SendRequest");

	SharedRequest_t req = PrepareRequest(method, url, content);

	std::lock_guard<std::mutex> lock_guard(m_QueueMutex);
	m_Queue.push_back(std::make_shared<QueueEntry>(req, std::move(callback)));
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
