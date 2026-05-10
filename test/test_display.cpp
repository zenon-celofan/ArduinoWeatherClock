#include "display_utils.h"
#include <cstdio>
#include <cstring>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

static bool text_eq(const DisplayDecision &d, const char *expected) {
    return strcmp(d.text, expected) == 0;
}

int main() {
    puts("\n=== decideDisplayContent Tests ===\n");

    // --- AP mode ---
    {
        auto d = decideDisplayContent(true, false, 0, 0, 0, true, 12, 34, 23.0f);
        RUN_TEST("AP mode text", text_eq(d, "AP"));
        RUN_TEST("AP mode centered", d.centered == true);
    }

    // --- WiFi stale, cycle < 1000 -> "wifi" centered ---
    {
        // wifiDisconnectedSince = 1000, currentMillis = 62000 => stale=true,
        // cycle = (62000-1000) % 10000 = 1000, NOT < 1000
        // Try again: 61999 gives cycle 999
        auto d = decideDisplayContent(false, true, 1000, 61999, 0, true, 12, 34, 0.0f);
        RUN_TEST("wifi stale cycle<1000 text", text_eq(d, "wifi"));
        RUN_TEST("wifi stale cycle<1000 centered", d.centered == true);
    }

    // --- WiFi stale, cycle >= 1000 -> formatted time, right ---
    {
        auto d = decideDisplayContent(false, true, 1000, 62000, 0, true, 12, 34, 0.0f);
        RUN_TEST("wifi stale cycle>=1000 text", text_eq(d, "1234"));
        RUN_TEST("wifi stale cycle>=1000 right", d.centered == false);
    }

    // --- NOT stale: wifiDisconnectedSince=0 means never disconnected (no duration info) ---
    {
        auto d = decideDisplayContent(false, true, 0, 60001, 0, true, 9, 5, 0.0f);
        RUN_TEST("wifi never disconnected shows time", text_eq(d, "905"));
    }

    // --- NOT stale, displayMode=1 (time only) -> formatted time, right ---
    {
        auto d = decideDisplayContent(false, true, 0, 1000, 1, true, 8, 5, 0.0f);
        RUN_TEST("mode=1 time only text", text_eq(d, "805"));
        RUN_TEST("mode=1 time only right", d.centered == false);
    }

    // --- NOT stale, displayMode=2 (temp only) -> formatted temp, centered ---
    {
        auto d = decideDisplayContent(false, true, 0, 1000, 2, false, 0, 0, -5.0f);
        RUN_TEST("mode=2 temp only text", text_eq(d, "-5"));
        RUN_TEST("mode=2 temp only centered", d.centered == true);
    }

    // --- NOT stale, displayMode=0, showTime=true -> formatted time, right ---
    {
        auto d = decideDisplayContent(false, true, 0, 1000, 0, true, 14, 30, 10.0f);
        RUN_TEST("mode=0 showTime text", text_eq(d, "1430"));
        RUN_TEST("mode=0 showTime right", d.centered == false);
    }

    // --- NOT stale, displayMode=0, showTime=false -> formatted temp, centered ---
    {
        auto d = decideDisplayContent(false, true, 0, 1000, 0, false, 14, 30, 23.7f);
        RUN_TEST("mode=0 showTemp text", text_eq(d, "+24"));
        RUN_TEST("mode=0 showTemp centered", d.centered == true);
    }

    // --- stale=false when wifiWasEverConnected=false ---
    {
        auto d = decideDisplayContent(false, false, 5000, 200000, 0, true, 10, 0, 0.0f);
        RUN_TEST("not stale: no prior connection -> time", text_eq(d, "1000"));
    }

    // --- stale=false when wifiDisconnectedSince=0 ---
    {
        auto d = decideDisplayContent(false, true, 0, 200000, 0, true, 10, 0, 0.0f);
        RUN_TEST("not stale: disconnectedSince=0 -> time", text_eq(d, "1000"));
    }

    // --- stale=false when not yet > 60s ---
    {
        auto d = decideDisplayContent(false, true, 150000, 200000, 0, true, 10, 0, 0.0f);
        RUN_TEST("not stale: only 50s elapsed -> time", text_eq(d, "1000"));
    }

    // --- temp with positive values ---
    {
        auto d = decideDisplayContent(false, true, 0, 1000, 2, false, 0, 0, 0.0f);
        RUN_TEST("temp=0.0 displays as '0'", text_eq(d, "0"));
    }

    puts("\n=== detectDisplayChange Tests ===\n");

    {
        String last = "";
        RUN_TEST("first call detects change", detectDisplayChange("hello", last) == true);
        RUN_TEST("first call updates last", last == "hello");
    }
    {
        String last = "hello";
        RUN_TEST("same text no change", detectDisplayChange("hello", last) == false);
        RUN_TEST("same text last unchanged", last == "hello");
    }
    {
        String last = "hello";
        RUN_TEST("different text detects change", detectDisplayChange("world", last) == true);
        RUN_TEST("different text updates last", last == "world");
    }
    {
        String last = "1234";
        RUN_TEST("change to wifi", detectDisplayChange("wifi", last) == true);
        RUN_TEST("no change after update", detectDisplayChange("wifi", last) == false);
    }

    puts("\n---\nAll display tests passed!");
    return 0;
}
