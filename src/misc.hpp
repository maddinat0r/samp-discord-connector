#pragma once

#include <string>
#include <type_traits>

#ifdef _MSC_VER
#pragma warning(push)
#pragma warning(disable: 4459)
#endif

#include <boost/spirit/include/qi_parse.hpp>
#include <boost/spirit/include/qi_int.hpp>
#include <boost/spirit/include/qi_real.hpp>
#include <boost/spirit/include/qi_bool.hpp>
#include <boost/spirit/include/karma_generate.hpp>
#include <boost/spirit/include/karma_int.hpp>
#include <boost/spirit/include/karma_uint.hpp>
#include <boost/spirit/include/karma_real.hpp>
#include <boost/spirit/include/karma_bool.hpp>

#ifdef _MSC_VER
#pragma warning(pop)
#endif

namespace qi = boost::spirit::qi;
namespace karma = boost::spirit::karma;


template<typename T>
bool ConvertStrToData(const std::string &src, T &dest)
{
	return qi::parse(src.begin(), src.end(),
		typename std::conditional<
			std::is_floating_point<T>::value, 
				qi::real_parser<T>, 
				qi::int_parser<T>
		>::type(), 
		dest);
}

template<typename T>
bool ConvertStrToData(const char *src, T &dest)
{
	return src != nullptr && qi::parse(src, src + strlen(src),
		typename std::conditional<
			std::is_floating_point<T>::value,
				qi::real_parser<T>,
				qi::int_parser<T>
		>::type(),
		dest);
}


template<typename T, unsigned int B = 10U>
bool ConvertDataToStr(T src, std::string &dest)
{
	return karma::generate(std::back_inserter(dest), 
		typename std::conditional<
			std::is_floating_point<T>::value,
				karma::real_generator<T>,
				typename std::conditional<
					std::is_signed<T>::value,
						karma::int_generator<T, B>, 
						karma::uint_generator<T, B>
				>::type
		>::type(),
		src);
}

template<> //bool specialization
inline bool ConvertStrToData(const std::string &src, bool &dest)
{
	return qi::parse(src.begin(), src.end(), qi::bool_, dest);
}

template<> //bool specialization
inline bool ConvertDataToStr(bool src, std::string &dest)
{
	return karma::generate(std::back_inserter(dest), karma::bool_, src);
}
