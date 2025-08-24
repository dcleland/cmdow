//!
// st_utils.hpp - String utility functions
//
// Copyright (c) 2025 David Cleland
// 
#pragma once
#include <string>

// Convert string to lowercase
std::string stToLower(std::string_view str);

// Case-insensitive string comparison
bool stInsensitiveCmp(const std::string_view& str1, const std::string_view& str2);
