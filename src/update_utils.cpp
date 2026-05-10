#include "update_utils.h"
#include "semver_compare.h"
#include <ArduinoJson.h>
#include <cstring>

UpdateBootAction evaluateUpdateBoot(uint8_t pending, uint8_t attempts, uint8_t maxAttempts) {
    if (pending == 0) return UPDATE_BOOT_CLEAR;
    if (attempts >= maxAttempts) return UPDATE_BOOT_DISABLE;
    return UPDATE_BOOT_RETRY;
}

bool parseGitHubRelease(const String &jsonBody, String &latestTag, String &downloadUrl, const String &currentVersion) {
    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, jsonBody.c_str());
    if (err) return false;

    const char *tag = doc["tag_name"];
    if (!tag || strlen(tag) == 0) return false;
    latestTag = tag;

    if (doc["assets"].size() > 0) {
        const char *url = doc["assets"][0]["browser_download_url"];
        if (url) downloadUrl = url;
    }

    return semverCompare(latestTag, currentVersion) > 0;
}
