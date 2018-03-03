#include "User.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "PawnCallback.hpp"


User::User(UserId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	m_Id = data["id"].get<std::string>();
	Update(data);
}

void User::Update(json &data)
{
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

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		AddUser(data["user"]); // that's our bot
		m_Initialized++;
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::USER_UPDATE, [](json &data)
	{
		PawnDispatcher::Get()->Dispatch([data]() mutable
		{
			auto const &user = UserManager::Get()->FindUserById(data["id"].get<std::string>());
			if (!user)
				return; // TODO: error msg: user not cached (cache mismatch)

			user->Update(data);

			// forward DCC_OnUserUpdate(DCC_User:user);
			PawnCallbackManager::Get()->Call("DCC_OnUserUpdate", user->GetPawnId());
		});
	});
}

bool UserManager::WaitForInitialization()
{
	unsigned int const
		SLEEP_TIME_MS = 20,
		TIMEOUT_TIME_MS = 20 * 1000;
	unsigned int waited_time = 0;
	while (m_Initialized != m_InitValue)
	{
		std::this_thread::sleep_for(std::chrono::milliseconds(SLEEP_TIME_MS));
		waited_time += SLEEP_TIME_MS;
		if (waited_time > TIMEOUT_TIME_MS)
			return false;
	}
	return true;
}

UserId_t UserManager::AddUser(json &data)
{
	Snowflake_t sfid = data["id"].get<std::string>();
	User_t const &user = FindUserById(sfid);
	if (user)
		return INVALID_USER_ID; // TODO: error log: user already exists

	UserId_t id = 1;
	while (m_Users.find(id) != m_Users.end())
		++id;

	if (!m_Users.emplace(id, User_t(new User(id, data))).first->second)
		return INVALID_USER_ID; // TODO: error log: duplicate key

	return id;
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
