#pragma once

#include <WString.h>
#include <cstdint>

enum UpdateBootAction : uint8_t {
    UPDATE_BOOT_CLEAR = 0,
    UPDATE_BOOT_DISABLE = 1,
    UPDATE_BOOT_RETRY = 2,
};

bool parseGitHubRelease(const String &jsonBody, String &latestTag, String &downloadUrl, const String &currentVersion);

UpdateBootAction evaluateUpdateBoot(uint8_t pending, uint8_t attempts, uint8_t maxAttempts);
