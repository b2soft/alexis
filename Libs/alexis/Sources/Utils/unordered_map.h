#pragma once

#include <unordered_map>
#include <string>
#include <string_view>

namespace alexis
{
	// metashit by
	// Artem Bakri Al-Sarmini 3sz3tt+tg@gmail.com

	struct detector;

	template <typename C, typename T, typename A>
	struct basic_string_hash
	{
		using transparent_key_equal = std::equal_to<>;
		using hash_type = std::hash<std::basic_string_view<C, T>>;
		std::size_t operator()(std::basic_string_view<C, T> txt) const { return hash_type{}(txt); }
		std::size_t operator()(const std::basic_string<C, T, A>& txt) const { return hash_type{}(txt); }
		std::size_t operator()(const C* txt) const { return hash_type{}(txt); }
	};

	template <typename K, typename V, typename H, typename E, typename A>
	struct choose_map
	{
		using type = std::unordered_map<K, V, H, E, A>;
	};

	template <typename K, typename V, typename E, typename A>
	struct choose_map<K, V, detector, E, A>
	{
		using type = std::unordered_map<K, V>;
	};

	template <typename C, typename T, typename SA, typename V, typename E, typename A>
	struct choose_map<std::basic_string<C, T, SA>, V, detector, E, A>
	{
		using type = std::unordered_map<
			std::basic_string<C, T, SA>,
			V,
			basic_string_hash<C, T, SA>,
			std::equal_to<>>;
	};

	template<
		class K,
		class V,
		class H = detector,
		class E = std::equal_to<K>,
		class A = std::allocator<std::pair<const K, V>>>
		using unordered_map = typename choose_map<K, V, H, E, A>::type;
}