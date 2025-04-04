#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <test.h>

typedef struct {
    const char *input;
    size_t len;
    uint64 expected_hash;
} TestCase;

void test_xxh3_64() {
    TEST_HEADER();

    TestCase tests[] = {
        { "hello", 5, 8327191403669471562ULL },
        { "beautiful", 9, 9895763730955755674ULL },
        { "meeting", 7, 18127804334972412130ULL },
        { "morning+sunshine", 16, 17884616789552791443ULL },
        { "", 0, 13315799867417602922ULL },
        { "this is just a test", 19, 2322547424642634624ULL },
        { "A", 1, 9742785211322600093ULL },
        { "minimalexampleforaverylongword", 30, 10632318926443905654ULL },
    };

    int num_tests = sizeof(tests) / sizeof(TestCase);

    for (int i = 0; i < num_tests; i++) {
        uint64 hash = xxh3_64(tests[i].input, tests[i].len);
        ASSERT(hash == tests[i].expected_hash, tests[i].input);
    }

    TEST_FOOTER();
}

int main() {
    test_xxh3_64();
}
