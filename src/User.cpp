#include "User.hpp"
#include "Network.hpp"


User::User(UserId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();
	m_Username = data["username"].get<std::string>();
	m_Discriminator = data["discriminator"].get<std::string>();

	auto it = data.find("bot");
	if (it != data.end())
		m_IsBot = it->get<bool>();

	it = data.find("verified");
	if (it != data.end())
		m_IsVerified = it->get<bool>();

	it = data.find("email");
	if (it != data.end() && !it->is_null())
		m_Email = it->get<std::string>();
}


void UserManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	/*Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_MEMBER_ADD, [](json &data)
	{
		UserManager::Get()->AddUser(data["user"]);
	});*/

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		AddUser(data["user"]); // that's our bot
		m_Initialized++;
	});

	/*Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::GUILD_CREATE, [this](json &data)
	{
		for (json &c : data["members"])
		{
			AddUser(c["user"]);
		}
		m_Initialized++;
	});*/

	// PAWN callbacks
	//Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::MESSAGE_CREATE, [this](json &data)
	//{
	//	Message msg(data);
	//	User_t const &channel = FindUserById(msg.GetUserId());
	//	if (channel)
	//	{
	//		CDispatcher::Get()->Dispatch([this, msg, &channel]()
	//		{
	//			// forward DCC_OnUserMessage(DCC_User:channel, const author[], const message[]);
	//			PawnCallbackManager::Get()->Call("DCC_OnUserMessage",
	//				channel->GetPawnId(), msg.GetAuthor(), msg.GetContent());
	//		});
	//	}
	//});
}

void UserManager::WaitForInitialization()
{
	while (m_Initialized != m_InitValue)
		std::this_thread::sleep_for(std::chrono::milliseconds(20));
}

User_t const &UserManager::AddUser(json &data)
{
	Snowflake_t sfid = data["id"].get<std::string>();
	User_t const &user = FindUserById(sfid);
	if (user)
		return user; // user already exists

	UserId_t id = 1;
	while (m_Users.find(id) != m_Users.end())
		++id;

	return m_Users.emplace(id, User_t(new User(id, data))).first->second;
}

User_t const &UserManager::FindUser(UserId_t id)
{
	static User_t invalid_user;
	auto it = m_Users.find(id);
	if (it == m_Users.end())
		return invalid_user;
	return it->second;
}

User_t const &UserManager::FindUserByName(
	std::string const &name, std::string const &discriminator)
{
	static User_t invalid_user;
	for (auto const &u : m_Users)
	{
		User_t const &user = u.second;
		if (user->GetUsername().compare(name) == 0
			&& user->GetDiscriminator().compare(discriminator) == 0)
		{
			return user;
		}
	}
	return invalid_user;
}

User_t const &UserManager::FindUserById(Snowflake_t const &sfid)
{
	static User_t invalid_user;
	for (auto const &u : m_Users)
	{
		User_t const &user = u.second;
		if (user->GetId().compare(sfid) == 0)
			return user;
	}
	return invalid_user;
}
