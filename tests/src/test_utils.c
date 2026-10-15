// Need to be included before <cmocka.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include <stdlib.h>
#include "utils.h"

static void test_perm_str(__attribute__((unused)) void **state)
{
	const char *ret;
	ret = perm_str(0, false);
	assert_string_equal(ret, "---------- ");
	ret = perm_str(S_IFBLK | 0777, true);
	assert_string_equal(ret, "brwxrwxrwx+");
	ret = perm_str(S_IFSOCK | 07777, false);
	assert_string_equal(ret, "srwsrwsrwt ");
	ret = perm_str(S_IFLNK | 07666, false);
	assert_string_equal(ret, "lrwSrwSrwT ");
	ret = perm_str(S_IFDIR | 0666, false);
	assert_string_equal(ret, "drw-rw-rw- ");
	ret = perm_str(S_IFCHR | 0555, false);
	assert_string_equal(ret, "cr-xr-xr-x ");
	ret = perm_str(S_IFIFO | 0444, false);
	assert_string_equal(ret, "pr--r--r-- ");
	ret = perm_str(0333, false);
	assert_string_equal(ret, "--wx-wx-wx ");
	ret = perm_str(0222, false);
	assert_string_equal(ret, "--w--w--w- ");
	ret = perm_str(0111, false);
	assert_string_equal(ret, "---x--x--x ");
}

static void test_make_path(__attribute__((unused)) void **state)
{
	char *ret;
	ret = make_path("dir", "file");
	assert_string_equal(ret, "dir/file");
	free(ret);
	ret = make_path("dir/", "file");
	assert_string_equal(ret, "dir/file");
	free(ret);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_perm_str),
		cmocka_unit_test(test_make_path),
	};

	return cmocka_run_group_tests_name("utils", tests, NULL, NULL);
}
