#pragma once

#include <Strsafe.h>
#include <string>

// Convert last WIN32 error into a text string
std::string GetLastErrorText();
