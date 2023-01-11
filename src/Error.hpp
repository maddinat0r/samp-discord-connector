#pragma once

#include <string>
#include <fmt/format.h>

using std::string;


template<typename T>
class CallbackError
{
	using Type = typename T::Error;
public:
	CallbackError() :
		m_Type(Type::NONE)
	{ }
	CallbackError(Type type, string &&msg) :
		m_Type(type),
		m_Message(std::move(msg))
	{ }
	template<typename... Args>
	CallbackError(Type type, string &&format, Args &&...args) :
		m_Type(type),
		m_Message(fmt::format(format, std::forward<Args>(args)...))
	{ }
	~CallbackError() = default;

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
