#include "update_utils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== parseGitHubRelease Tests ===\n");

    String latest, url;

    // --- Newer version found ---
    {
        String body = "{\"tag_name\":\"v1.2.3\",\"assets\":[{\"browser_download_url\":\"http://example.com/fw.bin\"}]}";
        RUN_TEST("newer version found", parseGitHubRelease(body, latest, url, "v1.0.0") == true);
        RUN_TEST("newer: latestTag set", latest == "v1.2.3");
        RUN_TEST("newer: downloadUrl set", url == "http://example.com/fw.bin");
    }

    // --- Same version ---
    {
        String body = "{\"tag_name\":\"v1.0.0\",\"assets\":[]}";
        RUN_TEST("same version", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Older version ---
    {
        String body = "{\"tag_name\":\"v0.9.0\",\"assets\":[]}";
        RUN_TEST("older version", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Invalid JSON ---
    {
        String body = "not json at all";
        RUN_TEST("invalid JSON", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Missing tag_name ---
    {
        String body = "{\"other\":\"value\"}";
        RUN_TEST("missing tag_name", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- Empty tag_name ---
    {
        String body = "{\"tag_name\":\"\"}";
        RUN_TEST("empty tag_name", parseGitHubRelease(body, latest, url, "v1.0.0") == false);
    }

    // --- No assets (no download URL) but newer version ---
    {
        String body = "{\"tag_name\":\"v2.0.0\"}";
        RUN_TEST("no assets but newer", parseGitHubRelease(body, latest, url, "v1.0.0") == true);
        RUN_TEST("no assets: tag set", latest == "v2.0.0");
    }

    // --- Version without v prefix ---
    {
        String body = "{\"tag_name\":\"2.0.0\"}";
        RUN_TEST("no v prefix", parseGitHubRelease(body, latest, url, "1.0.0") == true);
        RUN_TEST("no v prefix tag", latest == "2.0.0");
    }

    // --- Truncated JSON ---
    {
        String body = "{\"tag_name\":\"v1.0.0\"";
        RUN_TEST("truncated JSON", parseGitHubRelease(body, latest, url, "v0.0.9") == false);
    }

    // --- Multiple assets, picks first ---
    {
        String body = "{\"tag_name\":\"v1.5.0\",\"assets\":[{\"browser_download_url\":\"http://first.bin\"},{\"browser_download_url\":\"http://second.bin\"}]}";
        RUN_TEST("multiple assets picks first", parseGitHubRelease(body, latest, url, "v1.0.0") == true);
        RUN_TEST("first asset url", url == "http://first.bin");
    }

    puts("\n=== evaluateUpdateBoot Tests ===\n");

    // --- No pending update ---
    {
        RUN_TEST("no pending, attempts=0 -> CLEAR", evaluateUpdateBoot(0, 0, 3) == UPDATE_BOOT_CLEAR);
        RUN_TEST("no pending, attempts=5 -> CLEAR", evaluateUpdateBoot(0, 5, 3) == UPDATE_BOOT_CLEAR);
    }

    // --- Pending, attempts within limit ---
    {
        RUN_TEST("pending 1, attempts=0, max=2 -> RETRY", evaluateUpdateBoot(1, 0, 2) == UPDATE_BOOT_RETRY);
        RUN_TEST("pending 1, attempts=1, max=2 -> RETRY", evaluateUpdateBoot(1, 1, 2) == UPDATE_BOOT_RETRY);
    }

    // --- Pending, attempts at or above limit ---
    {
        RUN_TEST("pending 1, attempts=2, max=2 -> DISABLE", evaluateUpdateBoot(1, 2, 2) == UPDATE_BOOT_DISABLE);
        RUN_TEST("pending 1, attempts=3, max=2 -> DISABLE", evaluateUpdateBoot(1, 3, 2) == UPDATE_BOOT_DISABLE);
        RUN_TEST("pending 1, attempts=0, max=0 -> DISABLE", evaluateUpdateBoot(1, 0, 0) == UPDATE_BOOT_DISABLE);
    }

    // --- Different max values ---
    {
        RUN_TEST("pending 1, attempts=4, max=5 -> RETRY", evaluateUpdateBoot(1, 4, 5) == UPDATE_BOOT_RETRY);
        RUN_TEST("pending 1, attempts=5, max=5 -> DISABLE", evaluateUpdateBoot(1, 5, 5) == UPDATE_BOOT_DISABLE);
    }

    puts("\n---\nAll update tests passed!");
    return 0;
}
