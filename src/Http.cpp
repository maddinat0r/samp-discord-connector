#include "Http.hpp"
#include "Logger.hpp"
#include "misc.hpp"
#include "version.hpp"

#include <boost/asio/system_timer.hpp>
#include <date/date.h>


Http::Http(std::string token) :
	m_SslContext(asio::ssl::context::tlsv12_client),
	m_Token(token),
	m_NetworkThreadRunning(true),
	m_NetworkThread(std::bind(&Http::NetworkThreadFunc, this))
{
}

Http::~Http()
{
	m_NetworkThreadRunning = false;
	m_NetworkThread.join();

	// drain requests queue
	QueueEntry *entry;
	while (m_Queue.pop(entry))
		delete entry;
}

void Http::NetworkThreadFunc()
{
	std::unordered_map<std::string, TimePoint_t> path_ratelimit;
	unsigned int retry_counter = 0;
	unsigned int const MaxRetries = 3;
	bool skip_entry = false;

	if (!Connect())
		return;

	while (m_NetworkThreadRunning)
	{
		TimePoint_t current_time = std::chrono::steady_clock::now();
		std::list<QueueEntry*> skipped_entries;

		QueueEntry *entry;
		while (m_Queue.pop(entry))
		{
			// check if we're rate-limited
			auto pr_it = path_ratelimit.find(entry->Request->target().to_string());
			if (pr_it != path_ratelimit.end())
			{
				// rate-limit for this path exists
				// are we still within the rate-limit timepoint?
				if (current_time < pr_it->second)
				{
					// yes, ignore this request for now
					skipped_entries.push_back(entry);
					continue;
				}

				// no, delete rate-limit and go on
				path_ratelimit.erase(pr_it);
				Logger::Get()->Log(LogLevel::DEBUG, "rate-limit on path '{}' lifted",
					entry->Request->target().to_string());
			}

			boost::system::error_code error_code;
			Response_t response;
			Streambuf_t sb;
			retry_counter = 0;
			skip_entry = false;
			do
			{
				bool do_reconnect = false;
				beast::http::write(*m_SslStream, *entry->Request, error_code);
				if (error_code)
				{
					Logger::Get()->Log(LogLevel::ERROR, "Error while sending HTTP {} request to '{}': {}",
						entry->Request->method_string().to_string(),
						entry->Request->target().to_string(),
						error_code.message());

					do_reconnect = true;
				}
				else
				{
					beast::http::read(*m_SslStream, sb, response, error_code);
					if (error_code)
					{
						Logger::Get()->Log(LogLevel::ERROR, "Error while retrieving HTTP {} response from '{}': {}",
							entry->Request->method_string().to_string(),
							entry->Request->target().to_string(),
							error_code.message());

						do_reconnect = true;
					}
				}

				if (do_reconnect)
				{
					if (retry_counter++ >= MaxRetries || !ReconnectRetry())
					{
						// we failed to reconnect, discard this request
						Logger::Get()->Log(LogLevel::WARNING, "Failed to send request, discarding");
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
						Logger::Get()->Log(LogLevel::ERROR,
							"Error while processing rate-limit: already rate-limited path '{}'",
							limited_url);

						// skip this request, we'll re-add it to the queue to retry later
						skipped_entries.push_back(entry);
						continue;
					}

					it_r = response.find("X-RateLimit-Reset");
					if (it_r != response.end())
					{
						auto date_str = response.find(boost::beast::http::field::date)->value().to_string();
						std::istringstream date_ss{ date_str };
						date::sys_seconds date_utc;
						date_ss >> date::parse("%a, %d %b %Y %T %Z", date_utc); // RFC2616 HTTP header date format

						std::chrono::seconds timepoint_now = date_utc.time_since_epoch();
						Logger::Get()->Log(LogLevel::DEBUG, "rate-limiting path {} until {} (current time: {})",
							limited_url,
							it_r->value().to_string(),
							timepoint_now.count());

						string const &reset_time_str = it_r->value().to_string();
						long long reset_time_secs = 0;
						ConvertStrToData(reset_time_str, reset_time_secs);
						TimePoint_t reset_time = std::chrono::steady_clock::now()
							+ std::chrono::seconds(reset_time_secs - timepoint_now.count() + 1); // add a buffer of 1 second

						path_ratelimit.insert({ limited_url, reset_time });
					}
				}
			}

			if (entry->Callback)
				entry->Callback(sb, response);

			delete entry;
			entry = nullptr;
		}

		// add skipped entries back to queue
		for (auto e : skipped_entries)
			m_Queue.push(e);

		std::this_thread::sleep_for(std::chrono::milliseconds(50));
	}

	Disconnect();
}

bool Http::Connect()
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Connect");

	// connect to REST API
	asio::ip::tcp::resolver r{ m_IoService };
	boost::system::error_code error;
	auto target = r.resolve("discordapp.com", "443", error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Can't resolve Discord API URL: {} ({})",
			error.message(), error.value());
		return false;
	}

	m_SslStream.reset(new SslStream_t(m_IoService, m_SslContext));
	asio::connect(m_SslStream->lowest_layer(), target, error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Can't connect to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	// SSL handshake
	m_SslStream->set_verify_mode(asio::ssl::verify_peer, error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR,
			"Can't configure SSL stream peer verification mode for Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	m_SslStream->handshake(asio::ssl::stream_base::client, error);
	if (error)
	{
		Logger::Get()->Log(LogLevel::ERROR, "Can't establish secured connection to Discord API: {} ({})",
			error.message(), error.value());
		return false;
	}

	return true;
}

void Http::Disconnect()
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Disconnect");

	boost::system::error_code error;
	m_SslStream->shutdown(error);
	if (error && error != boost::asio::error::eof && error != boost::asio::ssl::error::stream_truncated)
	{
		Logger::Get()->Log(LogLevel::WARNING, "Error while shutting down SSL on HTTP connection: {} ({})",
			error.message(), error.value());
	}
}

bool Http::ReconnectRetry()
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::ReconnectRetry");

	unsigned int reconnect_counter = 0;
	do
	{
		Logger::Get()->Log(LogLevel::INFO, "trying reconnect #{}...", reconnect_counter + 1);

		Disconnect();
		if (Connect())
		{
			Logger::Get()->Log(LogLevel::INFO, "reconnect succeeded, resending request");
			return true;
		}
		else
		{
			unsigned int seconds_to_wait = static_cast<unsigned int>(std::pow(2U, reconnect_counter));
			Logger::Get()->Log(LogLevel::WARNING, "reconnect failed, waiting {} seconds...", seconds_to_wait);
			std::this_thread::sleep_for(std::chrono::seconds(seconds_to_wait));
		}
	} while (++reconnect_counter < 3);

	Logger::Get()->Log(LogLevel::ERROR, "Could not reconnect to Discord");
	return false;
}

Http::SharedRequest_t Http::PrepareRequest(beast::http::verb const method,
	std::string const &url, std::string const &content)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::PrepareRequest");

	auto req = std::make_shared<Request_t>();
	req->method(method);
	req->target("/api/v6" + url);
	req->version(11);
	req->insert(beast::http::field::connection, "keep-alive");
	req->insert(beast::http::field::host, "discordapp.com");
	req->insert(beast::http::field::user_agent,
		"DiscordBot (github.com/maddinat0r/samp-discord-connector, " PLUGIN_VERSION ")");
	if (!content.empty())
		req->insert(beast::http::field::content_type, "application/json");
	req->insert(beast::http::field::authorization, "Bot " + m_Token);
	req->body() = content;

	req->prepare_payload();

	return req;
}

void Http::SendRequest(beast::http::verb const method, std::string const &url,
	std::string const &content, ResponseCallback_t &&callback)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::SendRequest");

	SharedRequest_t req = PrepareRequest(method, url, content);

	if (callback == nullptr && Logger::Get()->IsLogLevel(LogLevel::DEBUG))
	{
		callback = CreateResponseCallback([=](Response r)
		{
			Logger::Get()->Log(LogLevel::DEBUG, "{:s} {:s} [{:s}] --> {:d} {:s}: {:s}",
				beast::http::to_string(method).to_string(), url, content, r.status, r.reason, r.body);
		});
	}

	m_Queue.push(new QueueEntry(req, std::move(callback)));
}

Http::ResponseCallback_t Http::CreateResponseCallback(ResponseCb_t &&callback)
{
	if (callback == nullptr)
		return nullptr;

	return [callback](Streambuf_t &sb, Response_t &resp)
	{
		callback({ resp.result_int(), resp.reason().to_string(),
			beast::buffers_to_string(resp.body().data()), beast::buffers_to_string(sb.data()) });
	};
}


void Http::Get(std::string const &url, ResponseCb_t &&callback)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Get");

	SendRequest(beast::http::verb::get, url, "",
		CreateResponseCallback(std::move(callback)));
}

void Http::Post(std::string const &url, std::string const &content,
	ResponseCb_t &&callback /*= nullptr*/)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Post");

	SendRequest(beast::http::verb::post, url, content,
		CreateResponseCallback(std::move(callback)));
}

void Http::Delete(std::string const &url)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Delete");

	SendRequest(beast::http::verb::delete_, url, "", nullptr);
}

void Http::Put(std::string const &url)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Put");

	SendRequest(beast::http::verb::put, url, "", nullptr);
}

void Http::Patch(std::string const &url, std::string const &content)
{
	Logger::Get()->Log(LogLevel::DEBUG, "Http::Patch");

	SendRequest(beast::http::verb::patch, url, content, nullptr);
}
