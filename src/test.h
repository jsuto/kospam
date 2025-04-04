#include <assert.h>
#include <kospam.h>

#define ASSERT(expr, value) if (!(expr)) { printf("assert failed: '%s'\n", value); abort(); } else { printf("."); }
#define TEST_HEADER() printf("%s() ", __func__);
#define TEST_FOOTER() printf(" OK\n");
