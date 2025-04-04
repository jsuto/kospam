#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <test.h>

typedef struct {
    const char *input;
    uint64 expected_hash;
} TestCase;

void test_xxh3_64() {
    TEST_HEADER();

    TestCase tests[] = {
        { "hello", 8327191403669471562ULL },
        { "beautiful", 9895763730955755674ULL },
        { "meeting", 18127804334972412130ULL },
        { "morning+sunshine", 17884616789552791443ULL },
        { "", 13315799867417602922ULL },
        { "this is just a test", 2322547424642634624ULL },
        { "A", 9742785211322600093ULL },
        { "minimalexampleforaverylongword", 10632318926443905654ULL },
    };

    int num_tests = sizeof(tests) / sizeof(TestCase);

    for (int i = 0; i < num_tests; i++) {
        uint64 hash = xxh3_64(tests[i].input);
        ASSERT(hash == tests[i].expected_hash, tests[i].input);
    }

    TEST_FOOTER();
}

int main() {
    test_xxh3_64();
}
