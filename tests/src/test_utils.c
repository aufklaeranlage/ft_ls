// Need to be included before <cmocka.h>
#include <stdarg.h>
#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>
#include <stdint.h>
#include <stdbool.h>

#include <cmocka.h>

#include <stdlib.h>
#include <string.h>
#include "utils.h"

static void test_perm_str(__attribute__((unused)) void **state)
{
	const char *ret;
	ret = perm_str(0);
	assert_string_equal(ret, "----------");
	ret = perm_str(S_IFBLK | 0777);
	assert_string_equal(ret, "brwxrwxrwx");
	ret = perm_str(S_IFSOCK | 07777);
	assert_string_equal(ret, "srwsrwsrwt");
	ret = perm_str(S_IFLNK | 07666);
	assert_string_equal(ret, "lrwSrwSrwT");
	ret = perm_str(S_IFDIR | 0666);
	assert_string_equal(ret, "drw-rw-rw-");
	ret = perm_str(S_IFCHR | 0555);
	assert_string_equal(ret, "cr-xr-xr-x");
	ret = perm_str(S_IFIFO | 0444);
	assert_string_equal(ret, "pr--r--r--");
	ret = perm_str(0333);
	assert_string_equal(ret, "--wx-wx-wx");
	ret = perm_str(0222);
	assert_string_equal(ret, "--w--w--w-");
	ret = perm_str(0111);
	assert_string_equal(ret, "---x--x--x");
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

typedef struct {
	struct stat stat_buf;
	FTSENT ent;
	char name_buf[256];
} MockFtsEnt;

static MockFtsEnt *create_mock_ftsent(const char *name, unsigned short info, time_t mtime, time_t mtime_nsec, time_t atime, time_t atime_nsec)
{
	MockFtsEnt *mock = malloc(sizeof(*mock));
	memset(mock, 0, sizeof(MockFtsEnt));
	mock->ent.fts_statp = &mock->stat_buf;
	mock->ent.fts_info = info;
	memcpy(mock->ent.fts_name, name, sizeof(mock->name_buf));
	mock->ent.fts_namelen = strlen(mock->name_buf);
	if (info != FTS_NS && info != FTS_NSOK && info != FTS_ERR) {
		mock->stat_buf.st_mtime = mtime;
		mock->stat_buf.st_mtime = atime;
#if defined(_POSIX_C_SOURCE) && _POSIX_C_SOURCE >= 200809L
		mock->stat_buf.st_mtim.tv_nsec = mtime_nsec;
		mock->stat_buf.st_atim.tv_nsec = atime_nsec;
#endif
	}
	return mock;
}

static void test_fts_alphasort_normal_behavior(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_alphasort(&ent_a, &ent_b);
    assert_true(result < 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_alphasort_case_insensitive(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("Backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_alphasort(&ent_a, &ent_b);
    assert_true(result < 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_alphasort_skip_dots(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("..archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_alphasort(&ent_a, &ent_b);
    assert_true(result < 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_alphasort_compare_length(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent(".", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("..", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_alphasort(&ent_a, &ent_b);
    assert_true(result < 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_mtimesort_normal_behavior(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_mtimesort(&ent_a, &ent_b);
    assert_true(result > 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_mtimesort_fallback_on_error(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 300000, 0, 600000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_ERR, 0, 0, 0, 0);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_mtimesort(&ent_a, &ent_b);
    assert_true(result > 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_atimesort_normal_behavior(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_atimesort(&ent_a, &ent_b);
    assert_true(result > 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_atimesort_fallback_on_error(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 300000, 0, 600000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_ERR, 0, 0, 0, 0);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_atimesort(&ent_a, &ent_b);
    assert_true(result > 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_rmtimesort_normal_behavior(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_rmtimesort(&ent_a, &ent_b);
    assert_true(result < 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_rmtimesort_fallback_on_error(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 300000, 0, 600000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_ERR, 0, 0, 0, 0);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_rmtimesort(&ent_a, &ent_b);
    assert_true(result > 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_ratimesort_normal_behavior(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 100000, 0, 102000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_F, 200000, 0, 208500, 150);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_ratimesort(&ent_a, &ent_b);
    assert_true(result < 0);
    free(mfts_a);
    free(mfts_b);
}

static void test_fts_ratimesort_fallback_on_error(__attribute__((unused)) void **state) {
    MockFtsEnt *mfts_a = create_mock_ftsent("archive.txt", FTS_F, 300000, 0, 600000, 100);
    MockFtsEnt *mfts_b = create_mock_ftsent("backup.txt", FTS_ERR, 0, 0, 0, 0);
		const FTSENT *ent_a = &mfts_a->ent, *ent_b = &mfts_b->ent;
    int result = fts_ratimesort(&ent_a, &ent_b);
    assert_true(result > 0);
    free(mfts_a);
    free(mfts_b);
}

int main(void)
{
	const struct CMUnitTest tests[] = {
		cmocka_unit_test(test_perm_str),
		cmocka_unit_test(test_make_path),
		cmocka_unit_test(test_fts_alphasort_normal_behavior),
		cmocka_unit_test(test_fts_alphasort_case_insensitive),
		cmocka_unit_test(test_fts_alphasort_skip_dots),
		cmocka_unit_test(test_fts_alphasort_compare_length),
		cmocka_unit_test(test_fts_mtimesort_normal_behavior),
		cmocka_unit_test(test_fts_mtimesort_fallback_on_error),
		cmocka_unit_test(test_fts_atimesort_normal_behavior),
		cmocka_unit_test(test_fts_atimesort_fallback_on_error),
		cmocka_unit_test(test_fts_rmtimesort_normal_behavior),
		cmocka_unit_test(test_fts_rmtimesort_fallback_on_error),
		cmocka_unit_test(test_fts_ratimesort_normal_behavior),
		cmocka_unit_test(test_fts_ratimesort_fallback_on_error),
	};

	return cmocka_run_group_tests_name("utils", tests, NULL, NULL);
}
