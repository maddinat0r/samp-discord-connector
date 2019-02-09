#include "User.hpp"
#include "Network.hpp"
#include "PawnDispatcher.hpp"
#include "Callback.hpp"
#include "CLog.hpp"
#include "utils.hpp"


User::User(UserId_t pawn_id, json &data) :
	m_PawnId(pawn_id)
{
	if (!utils::TryGetJsonValue(data, m_Id, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return;
	}

	Update(data);
}

void User::Update(json &data)
{
	_valid =
		utils::TryGetJsonValue(data, m_Username, "username") &&
		utils::TryGetJsonValue(data, m_Discriminator, "discriminator");

	if (!_valid)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't update user: invalid JSON: \"{}\"", data.dump());
		return;
	}

	utils::TryGetJsonValue(data, m_IsBot, "bot");
	utils::TryGetJsonValue(data, m_IsVerified, "verified");
}


void UserManager::Initialize()
{
	assert(m_Initialized != m_InitValue);

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::READY, [this](json &data)
	{
		if (!utils::IsValidJson(data, "user", json::value_t::object))
		{
			CLog::Get()->Log(LogLevel::FATAL,
				"invalid JSON: expected \"user\" in \"{}\"", data.dump());
			return;
		}
		AddUser(data["user"]); // that's our bot
		m_Initialized++;
	});

	Network::Get()->WebSocket().RegisterEvent(WebSocket::Event::USER_UPDATE, [](json &data)
	{
		Snowflake_t user_id;
		if (!utils::TryGetJsonValue(data, user_id, "id"))
		{
			CLog::Get()->Log(LogLevel::ERROR,
				"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		}

		PawnDispatcher::Get()->Dispatch([data, user_id]() mutable
		{
			auto const &user = UserManager::Get()->FindUserById(user_id);
			if (!user)
			{
				CLog::Get()->Log(LogLevel::ERROR,
					"can't update user: user id \"{}\" not cached", user_id);
				return;
			}

			user->Update(data);

			// forward DCC_OnUserUpdate(DCC_User:user);
			pawn_cb::Error error;
			pawn_cb::Callback::CallFirst(error, "DCC_OnUserUpdate", user->GetPawnId());
		});
	});
}

bool UserManager::IsInitialized()
{
	return m_Initialized == m_InitValue;
}

UserId_t UserManager::AddUser(json &data)
{
	Snowflake_t sfid;
	if (!utils::TryGetJsonValue(data, sfid, "id"))
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"invalid JSON: expected \"id\" in \"{}\"", data.dump());
		return INVALID_USER_ID;
	}

	User_t const &user = FindUserById(sfid);
	if (user)
		return user->GetPawnId();

	UserId_t id = 1;
	while (m_Users.find(id) != m_Users.end())
		++id;

	if (!m_Users.emplace(id, User_t(new User(id, data))).first->second)
	{
		CLog::Get()->Log(LogLevel::ERROR,
			"can't create user: duplicate key '{}'", id);
		return INVALID_USER_ID;
	}

	CLog::Get()->Log(LogLevel::INFO, "successfully created user with id '{}'", id);
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
