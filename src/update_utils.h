#pragma once

#include <WString.h>

bool parseGitHubRelease(const String &jsonBody, String &latestTag, String &downloadUrl, const String &currentVersion);
