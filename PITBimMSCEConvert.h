#pragma once

#include <regex>

namespace PIT
{
	template<typename T>
	std::basic_string<T, std::char_traits<T>, std::allocator<T> > ReplaceString(const T* text, const T* old, const T* _new) {
		std::basic_regex<T> pattern(old);
		std::basic_string<T, std::char_traits<T>, std::allocator<T> > rep(_new);
		std::basic_string<T, std::char_traits<T>, std::allocator<T> > text1(text);
		std::basic_string<T, std::char_traits<T>, std::allocator<T> > tmp = std::regex_replace(text1, pattern, rep);
		return tmp;
	}
	struct ConvertToElement
	{
		static bool SubEntityToElement(EditElementHandleR eeh, ISubEntityPtr entity, DgnModelRefP modelRef);
	};

	struct ConvertElement
	{

	};

	struct RebarFuction
	{
		static void AutoSetWallRebarCodes(vector<string>& teststring, std::map<std::string, IDandModelref>& mapidAndmodel);
	};
}
