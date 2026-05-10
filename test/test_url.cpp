#include "url_utils.h"
#include <cstdio>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== URL Utils Tests ===\n");

    // --- buildLokiUrl ---
    RUN_TEST("Loki URL basic",
        buildLokiUrl("192.168.1.100", "3100")
        == "http://192.168.1.100:3100/loki/api/v1/push");

    RUN_TEST("Loki URL different port",
        buildLokiUrl("10.0.0.1", "9090")
        == "http://10.0.0.1:9090/loki/api/v1/push");

    RUN_TEST("Loki URL empty port",
        buildLokiUrl("127.0.0.1", "")
        == "http://127.0.0.1:/loki/api/v1/push");

    // --- buildOpenMeteoUrl ---
    RUN_TEST("Open-Meteo URL basic",
        buildOpenMeteoUrl("50.140", "16.955")
        == "http://api.open-meteo.com/v1/forecast?latitude=50.140&longitude=16.955&current_weather=true&timezone=auto");

    RUN_TEST("Open-Meteo URL negative coords",
        buildOpenMeteoUrl("-33.8688", "151.2093")
        == "http://api.open-meteo.com/v1/forecast?latitude=-33.8688&longitude=151.2093&current_weather=true&timezone=auto");

    RUN_TEST("Open-Meteo URL equator",
        buildOpenMeteoUrl("0", "0")
        == "http://api.open-meteo.com/v1/forecast?latitude=0&longitude=0&current_weather=true&timezone=auto");

    puts("\n---\nAll URL tests passed!");
    return 0;
}
