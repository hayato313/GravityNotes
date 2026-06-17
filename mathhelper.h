#pragma once
#include <algorithm>

template<typename T>
T Clamp(T value, T lo, T hi)
{
	return (std::max)(lo, (std::min)(hi, value));
}
