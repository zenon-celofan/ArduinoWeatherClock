#include "semver_compare.h"
#include <cstdio>
#include <cstdlib>

#define RUN_TEST(name, expr) do {                                    \
    printf("  TEST: %s ... ", name);                                 \
    if (!(expr)) { printf("FAIL\n"); return 1; }                     \
    printf("PASS\n");                                                \
} while(0)

int main() {
    puts("\n=== semverCompare Tests ===\n");

    // Equal
    RUN_TEST("equal versions",               semverCompare("v0.1.22", "v0.1.22") == 0);
    RUN_TEST("equal, no v prefix",           semverCompare("0.1.22", "0.1.22") == 0);
    RUN_TEST("equal, mixed v prefix",        semverCompare("v0.1.22", "0.1.22") == 0);
    RUN_TEST("equal, zeroes",                semverCompare("v0.0.0", "v0.0.0") == 0);

    // Newer
    RUN_TEST("newer major",                  semverCompare("v1.0.0", "v0.9.9") == 1);
    RUN_TEST("newer minor",                  semverCompare("v0.2.0", "v0.1.22") == 1);
    RUN_TEST("newer patch",                  semverCompare("v0.1.23", "v0.1.22") == 1);
    RUN_TEST("newer all three",              semverCompare("v2.3.4", "v1.2.3") == 1);

    // Older
    RUN_TEST("older major",                  semverCompare("v0.9.9", "v1.0.0") == -1);
    RUN_TEST("older minor",                  semverCompare("v0.1.22", "v0.2.0") == -1);
    RUN_TEST("older patch",                  semverCompare("v0.1.22", "v0.1.23") == -1);
    RUN_TEST("older all three",              semverCompare("v1.2.3", "v2.3.4") == -1);

    // Edge
    RUN_TEST("mixed prefix, newer",          semverCompare("0.1.22", "v0.1.23") == -1);
    RUN_TEST("large numbers",                semverCompare("v99.99.99", "v100.0.0") == -1);
    RUN_TEST("patch vs minor tiebreaker",    semverCompare("v0.1.1", "v0.1.0") == 1);

    puts("\n---\nAll semver tests passed!");
    return 0;
}
