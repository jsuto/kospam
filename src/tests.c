#include <stdio.h>
#include <stdint.h>
#include <string.h>
#include <stdlib.h>
#include <test.h>

typedef struct {
    const char *input;
    uint64 expected_hash;
} TestCase;

typedef struct {
    const char *input;
    const char *expected;
} TestCaseStrStr;

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

void test_chop_newlines() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "aaaa", "aaaa" },
        { "aaaa\naaaa\n", "aaaa\naaaa" },
        { "aaaa\nbbbb\r", "aaaa\nbbbb" },
        { "aaaa\naaaa\r\n", "aaaa\naaaa" },
        { "aaaa\naaaa\r\n\n\n", "aaaa\naaaa" },
        { "aaaa\naaaa\n\n\n", "aaaa\naaaa" },
        { "", "" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        chop_newlines(s, strlen(s));
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_normalize_buffer() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "aaaa", "aaaa" },
        { "aaaa,aaaa", "aaaa aaaa" },
        { "aaaa\naaaa", "aaaa aaaa" },
        { "aaaa\naaaa\n", "aaaa aaaa" },
        { "aaaa\naaaa,bbbb\n", "aaaa aaaa bbbb" },
        { "aaaa\naaaa;bbbb\n", "aaaa aaaa bbbb" },
        { "aaaa\naaaa!\n", "aaaa aaaa" },
        { "aaaa\naaaa?\n", "aaaa aaaa" },
        { "aaaa\n(aaaa)\n", "aaaa aaaa" },
        { "", "" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        normalize_buffer(s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_split() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "aaaa", "aaaa" },
        { "aaaa,bbbb", "aaaabbbb" },
        { "aaaa,cccc,", "aaaacccc" },
        { "", "" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        memset(s, 0, sizeof(s));
        int pos = 0;

        char *p = (char*)tests[i].input;
        while (p) {
           char v[SMALLBUFSIZE];
           int result;
           p = split(p, ',', v, sizeof(v)-1, &result);
           memcpy(s+pos, v, strlen(v));
           pos += strlen(v);
        }
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_decodeQP() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "Kedves kor=C3=A1bbi v=C3=A1s=C3=A1rl=C3=B3nk!", "Kedves korábbi vásárlónk!" },
        { "kit=C3=B6lt=C3=A9si =C3=BAtmutat=C3=B3j=C3=A1t tartalmazza k=C3=A9perny=C5=\n=91k=C3=A9pekkel.", "kitöltési útmutatóját tartalmazza képernyőképekkel." },
        { "aaaa cccc,", "aaaa cccc," },
        { "", "" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        decodeQP(s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_base64_decode() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "RGVhciBTaXJzLA0KDQpHbGFkIHRvIGhlYXIgdGhhdCB5b3UncmUgb24gdGhlIGZpbHRyYXRpb24g", "Dear Sirs,\r\n\r\nGlad to hear that you're on the filtration " },
        { "w6lyxZEgYSB2w6lybnlvbcOhcyBlZ3lzemVyxbEgw6lzIG1lZ2LDrXpoYXTDsyBtw6lyw6lzw6ly", "érő a vérnyomás egyszerű és megbízható mérésér" },
        { "", "" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        base64_decode(s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_digest_string() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "aaaabbbbccc", "5e39810052af4e60cd48c8169725730a8b06345ddcaa5c50cdf1545ee051f099" },
        { "", "e3b0c44298fc1c149afbf4c8996fb92427ae41e4649b934ca495991b7852b855" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        digest_string("sha256", (char*)tests[i].input, &s[0]);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}


void test_decode_html_entities_utf8_inplace() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "szexu&#225;lis &#233;lete cs&#250;cs&#225;n &#233;rezheti mag&#225;t", "szexuális élete csúcsán érezheti magát" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        decode_html_entities_utf8_inplace(s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_normalize_html() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
       {
          "<html><body><script>alert(0);</script><aaa>My text</aaa><img src=\"/aaa.jpg\" /> aaaa<zzz attr=\"aaa\"></zzz>aaa",
          "     My text   aaaa  aaa"
       },
       {
          "<html>\n<body>\n<script>alert(0);</script>\n<aaa>My text</aaa>\n<img src=\"/aaa.jpg\" /> aaaa<zzz attr=\"aaa\"></zzz>aaa\n",
          " \n \n  \n My text \n  aaaa  aaa\n"
       },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        normalize_html(s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

int main() {
    test_xxh3_64();
    test_chop_newlines();
    test_normalize_buffer();
    test_split();
    test_decodeQP();
    test_base64_decode();
    test_digest_string();
    test_decode_html_entities_utf8_inplace();
    test_normalize_html();
}
