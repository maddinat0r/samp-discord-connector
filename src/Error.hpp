#pragma once

#include <string>
#include <fmt/format.h>

using std::string;


template<typename T>
class Error
{
	using Type = typename T::Error;
public:
	Error() :
		m_Type(Type::NONE)
	{ }
	Error(Type type, string &&msg) :
		m_Type(type),
		m_Message(std::move(msg))
	{ }
	template<typename... Args>
	Error(Type type, string &&format, Args &&...args) :
		m_Type(type),
		m_Message(fmt::format(format, std::forward<Args>(args)...))
	{ }
	~Error() = default;

	operator bool() const
	{
		return m_Type != Type::NONE;
	}

	const string &msg() const
	{
		return m_Message;
	}
	const Type type() const
	{
		return m_Type;
	}
	const string &module() const
	{
		return T::ModuleName;
	}

	operator Type() const
	{
		return type();
	}

	void set(Type type, string &&msg)
	{
		m_Type = type;
		m_Message.assign(std::move(msg));
	}
	template<typename... Args>
	void set(Type type, string &&format, Args &&...args)
	{
		m_Type = type;
		m_Message.assign(fmt::format(format, std::forward<Args>(args)...));
	}

private:
	Type m_Type;
	string m_Message;
};
