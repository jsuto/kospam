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

typedef struct {
    const char *input;
    const char *input2;
    const char *expected;
} TestCaseStrStrStr;

typedef struct {
    const char *input;
    const bool expected;
} TestCaseStrBool;

typedef struct {
    const char *input;
    const char c;
    const int expected;
} TestCaseStrCharInt;

struct config cfg;

void aaa(struct node *xhash[], char *s, size_t slen){
    size_t pos=0;

    memset(s, 0, slen);

    for(int i=0;i<MAXHASH;i++) {
        struct node *q = xhash[i];
        while(q != NULL) {
            size_t len = strlen((char*)q->str);
            if(slen > pos + len + 2) {
                memcpy(s+pos, (char*)q->str, len);
                pos += len;
                memcpy(s+pos, " ", 1);
                pos++;
            }

            q = q->r;
        }
    }
}

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
          "<html><body><script>alert(0);</script><aaa>My text &lt;&gt;</aaa><img src=\"/aaa.jpg\" /> aaaa<zzz attr=\"aaa\"></zzz>aaa",
          "     My text <>   aaaa  aaa"
       },
       {
          "<html>\n<body>\n<script>alert(0);</script>\n<aaa>My text &amp;</aaa>\n<img src=\"/aaa.jpg\" /> aaaa<zzz attr=\"aaa\"></zzz>aaa\n",
          " \n \n  \n My text & \n  aaaa  aaa\n"
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

void test_utf8_tolower() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        {
          "VÁLTOZÁS TÖRTÉNT AZ IPARŰZÉSI ADÓBAN BEVEZETTÉK AZ EGYSZERŰSÍTETT TÉTELES VAGY SÁVOS IPARŰZÉSI ADÓT",
          "változás történt az iparŰzési adóban bevezették az egyszerŰsített tételes vagy sávos iparŰzési adót"
        },
        {
          "ADÓBEVALLÁS GYAKORLATIAS KITÖLTÉSI ÚTMUTATÓJÁT TARtalmaZZA",
          "adóbevallás gyakorlatias kitöltési útmutatóját tartalmazza"
        },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        utf8_tolower(s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}

void test_generate_tokens_from_string() {
    TEST_HEADER();

    TestCaseStrStrStr tests[] = {
        {
          "Dear Friend! Let me give you a hint, someverylongextremelybigstring att*thisisanameforalongattachmentname.jpg for aos-es stuff, please see https://example.com/aaa/bbb and https://aaa/b?=aa or https://aaa.bbb.fu/ for more info.",
          "",
          "for+aos-es see+and aaa you+a a+hint URL*example.com me+give att*thisisanameforalongattachmentname.jpg Friend+Let URL*aaa Friend more+info Let+me stuff and+aaa att*thisisanameforalongattachmentname.jpg+for please+see please hint aaa+or you for URL*bbb.fu stuff+please or+aaa Let give+you and aos-es+stuff aos-es give info for+more Dear+Friend see hint+att*thisisanameforalongattachmentname.jpg aaa+for Dear more ",
        },
        {
          "Dear friend! See this someverylongextremelybigstring?",
          "SUBJ*",
          "SUBJ*Dear SUBJ*this+SUBJ*someverylongextremelybigstring SUBJ*friend SUBJ*See SUBJ*See+SUBJ*this SUBJ*Dear+SUBJ*friend SUBJ*this SUBJ*friend+SUBJ*See SUBJ*someverylongextremelybigstring "
        }
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStrStr);

    for (int i = 0; i < num_tests; i++) {
        struct parser_state state;
        init_state(&state);

        generate_tokens_from_string(&state, tests[i].input, (char*)tests[i].input2, &cfg);

        char s[SMALLBUFSIZE];
        aaa(state.token_hash, &s[0], sizeof(s));
        //printf("a=*%s*\n", s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);

        clearhash(state.token_hash);
    }

    TEST_FOOTER();
}


void test_extract_url_token() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "https://example.com/aaa/bbb", "URL*example.com" },
        { "https://example/b?a=1", "URL*example" },
        { "https://example.com", "URL*example.com" },
        { "https://www.example.com/", "URL*example.com" },
        { "https://www.example.com", "URL*example.com" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE], result[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", &(tests[i].input[8]));
        extract_url_token(s, &result[0], sizeof(result));
        ASSERT(strcmp(result, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}


void test_is_item_on_list() {
    TEST_HEADER();

    TestCaseStrBool tests[] = {
        { "10.1.1.1", true },
        { "1.1.1.2", false },
        { "127.0.0.1", true },
        { "123.123", true },
        { "123.123.256.256", false },
    };

    char list[SMALLBUFSIZE];
    sprintf(list, "127.,10.,192.168.,172.16.,123.123$,xxx.xxx");

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        bool result = is_item_on_list((char *)tests[i].input, list);
        ASSERT(result == tests[i].expected, tests[i].input);
    }

    TEST_FOOTER();
}


void test_count_character_in_buffer() {
    TEST_HEADER();

    TestCaseStrCharInt tests[] = {
        { "aaaaa", 'a', 5 },
        { "aaaaa", 'b', 0 },
        { "aaaaa", '\0', 0 },
        { "ababababab", 'a', 5 },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrCharInt);

    for (int i = 0; i < num_tests; i++) {
        int result = count_character_in_buffer((char *)tests[i].input, tests[i].c);
        ASSERT(result == tests[i].expected, tests[i].input);
    }

    TEST_FOOTER();
}


void test_extract_name_from_headers() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "Content-Type: image/jpeg;\n  name=\"ubaxxezoeycayr.jpeg\"", "ubaxxezoeycayr.jpeg" },
        { "Content-Type: image/jpeg;\nContent-Transfer-Encoding: base64\nContent-Disposition: attachment;\n   name=\"aaa.jpg\"", "aaa.jpg" },
        { "Content-Type: image/jpeg;\nContent-Transfer-Encoding: base64\n", "" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        extract_name_from_headers((char *)tests[i].input, s, sizeof(s));
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}


void test_fixup_encoded_header() {
    TEST_HEADER();

    TestCaseStrStr tests[] = {
        { "=?UTF-8?B?SG9neWFuIMOtcmp1bmsgcGFuYXN6bGV2ZWxldD8=?=", "Hogyan írjunk panaszlevelet?" },
        { "Happy New Year! =?utf-8?q?=F0=9F=8E=86?=", "Happy New Year! 🎆" },
        { "=?utf-8?Q?=C3=81ram=C3=BCgyint=C3=A9z=C3=A9s_=C3=BAj_online_=C3=BCgyf?=\n =?utf-8?Q?=C3=A9lszolg=C3=A1laton_=C3=A9s_applik=C3=A1ci=C3=B3ban!?=", "Áramügyintézés új online ügyfélszolgálaton és applikációban!" },
        { "=?UTF-8?B?RndkOiDimqDvuI8gRW1sw6lrZXp0ZXTFkTogTcOpZyBuZW0gesOhcnRhZCBsZSBmacOzaw==?=\n =?UTF-8?B?b2RiYW4gYSAyMDI0LWVzIMOpdmV0?=", "Fwd: ⚠️ Emlékeztető: Még nem zártad le fiókodban a 2024-es évet" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        fixup_encoded_header(s, strlen(s));
        //printf("s=*%s*\n", s);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}


void test_extract_header_value() {
    TEST_HEADER();

    const char *headers =
        "Return-Path: <yzpodzc@winskersd.de>\n"
        "Received: from antispam.localhost (localhost [127.0.0.1])\n"
        "    by mail.example.com (Postfix) with ESMTP id 8C7823ECCE\n"
        "    for <bela@acts.hu>; Thu, 20 Feb 2025 20:46:59 +0100 (CET)\n"
        "Received: from xn--g1agd.058.xn--p1acf (xn--g1agd.058.xn--p1acf [62.173.138.106])\n"
        "    by mail.example.com (Postfix) with ESMTP id 30BF03EA57\n"
        "    for <bela@acts.hu>; Thu, 20 Feb 2025 20:46:59 +0100 (CET)\n"
        "Received: from winskersd.de (osoban.my [185.170.209.70])\n"
        "    by xn--g1agd.058.xn--p1acf (Postfix) with ESMTPA id BF3BD26FDAD;\n"
        "    Thu, 20 Feb 2025 21:15:22 +0200 (EET)\n"
        "Message-ID: <54064738T83184233L67511567U06028373O@id.yzpodzc.winskersd.de>\n"
        "From: \"Nuubu Detox\" <yzpodzc@winskersd.de>\n"
        "To: <fake.address@example.com>\n"
        "Subject: =?utf-8?B?MTAgbmFwb3MgbcOpcmVndGVsZW7DrXTFkSBwcm9ncmFtIE51dWJ1?=\n"
        "Date: Thu, 20 Feb 2025 21:15:24 +0200\n"
        "MIME-Version: 1.0\n"
        "Content-Type: multipart/related;\n"
        "    type=\"multipart/alternative\";\n"
        "    boundary=\"----=_NextPart_000_0006_01DB83DC.627F1120\"\n"
        "X-Clapf-spamicity: 4000000067b786bd1d415b6400d6955091e5\n"
        "X-Clapf-spamicity: 1.0000\n"
        "X-Clapf-spamicity: Yes\n";

    size_t hdr_len = strlen(headers);

    TestCaseStrStr tests[] = {
       { "From:", "\"Nuubu Detox\" <yzpodzc@winskersd.de>" },
       { "To:", "<fake.address@example.com>" },
       { "Subject:", "10 napos méregtelenítő program Nuubu" },
       { "Content-Type:", "multipart/related;"
         "    type=\"multipart/alternative\";"
         "    boundary=\"----=_NextPart_000_0006_01DB83DC.627F1120\"" },
       { "Message-ID:", "<54064738T83184233L67511567U06028373O@id.yzpodzc.winskersd.de>" },
       { "Received:", "from antispam.localhost (localhost [127.0.0.1])"
         "    by mail.example.com (Postfix) with ESMTP id 8C7823ECCE"
         "    for <bela@acts.hu>; Thu, 20 Feb 2025 20:46:59 +0100 (CET)" },
    };

    int num_tests = sizeof(tests) / sizeof(TestCaseStrStr);

    for (int i = 0; i < num_tests; i++) {
        char s[SMALLBUFSIZE];
        snprintf(s, sizeof(s)-1, "%s", tests[i].input);
        extract_header_value(headers, hdr_len, tests[i].input, strlen(tests[i].input), s, sizeof(s)-1);
        ASSERT(strcmp(s, tests[i].expected) == 0, tests[i].input);
    }

    TEST_FOOTER();
}


int main() {
    cfg = read_config("../tests/kospam.conf");

    test_xxh3_64();
    test_chop_newlines();
    test_normalize_buffer();
    test_split();
    test_decodeQP();
    test_base64_decode();
    test_digest_string();
    test_decode_html_entities_utf8_inplace();
    test_normalize_html();
    test_utf8_tolower();
    test_generate_tokens_from_string();
    test_extract_url_token();
    test_is_item_on_list();
    test_count_character_in_buffer();
    test_extract_name_from_headers();
    test_fixup_encoded_header();
    test_extract_header_value();
}
