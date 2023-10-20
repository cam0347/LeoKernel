#include <include/assert.h>
#include <include/types.h>
#include <include/low_level.h>
#include <tty/include/tty.h>

void assert_true(bool test) {
    if (!ASSERT_ENABLED) {
        return;
    }

    if (!test) {
        fail("assertion failed");
        sys_hlt();
    }
}

void assert_false(bool test) {
    assert_true(!test);
}