#pragma once

#include "Singleton.hpp"
#include "types.hpp"

#include <string>
#include <atomic>

#include <json.hpp>


using json = nlohmann::json;


class User
{
public:
	User(UserId_t pawn_id, json const &data);
	~User() = default;

private:
	const UserId_t m_PawnId;

	Snowflake_t m_Id;

	std::string
		m_Username,
		m_Discriminator;

	bool
		m_IsBot = false,
		m_IsVerified = false;

	bool _valid = false;

public:
	inline UserId_t GetPawnId() const
	{
		return m_PawnId;
	}
	inline Snowflake_t const &GetId() const
	{
		return m_Id;
	}
	inline std::string const &GetUsername() const
	{
		return m_Username;
	}
	inline std::string const &GetDiscriminator() const
	{
		return m_Discriminator;
	}
	inline bool IsBot() const
	{
		return m_IsBot;
	}
	inline bool IsVerified() const
	{
		return m_IsVerified;
	}

	inline bool IsValid() const
	{
		return _valid;
	}
	inline operator bool() const
	{
		return IsValid();
	}

	void Update(json const &data);
};


class UserManager : public Singleton<UserManager>
{
	friend class Singleton<UserManager>;
private:
	UserManager() = default;
	~UserManager() = default;

private:
	const unsigned int m_InitValue = 1;
	std::atomic<unsigned int> m_Initialized{ 0 };

	std::map<UserId_t, User_t> m_Users; //PAWN user-id to actual channel map


public:
	void Initialize();
	bool IsInitialized();

	UserId_t AddUser(json const &data);

	User_t const &FindUser(UserId_t id);
	User_t const &FindUserByName(std::string const &name, std::string const &discriminator);
	User_t const &FindUserById(Snowflake_t const &sfid);
};
