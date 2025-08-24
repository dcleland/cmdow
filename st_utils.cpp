//!
// st_utils.cpp - Implementation of string utility functions
//
// Copyright (c) 2025 David Cleland
//
#include "st_utils.hpp"

#include <algorithm>

bool stInsensitiveCmp(const std::string_view& str1, const std::string_view& str2)
 {
    return str1.size() == str2.size() &&
           std::equal(str1.begin(), str1.end(), str2.begin(),
                      [](char a, char b) { return std::tolower(a) == std::tolower(b); });
}

std::string stToLower(std::string_view str)
{
    std::string lowerStr = std::string(str);
    std::transform(lowerStr.begin(), lowerStr.end(), lowerStr.begin(), [](unsigned char c) { return std::tolower(c); });
    return lowerStr;
}