/* Quoted path so a syntax check without -I still finds the public header. */
#include "../include/patmat.h"

#include <stdio.h>
#include <string.h>

static int failures = 0;
static int tests_run = 0;
static int tests_failed = 0;
static const char *current_test = NULL;

static void expect(int condition, const char *file, int line, const char *expr)
{
    if (!condition) {
        printf("FAIL %s %s:%d: %s\n",
               current_test != NULL ? current_test : "?",
               file, line, expr);
        fflush(stdout);
        failures++;
    }
}

#define EXPECT(cond) expect((cond), __FILE__, __LINE__, #cond)

static int semver_major_minor(const char *version)
{
    int parts = 0;

    if (version == NULL) {
        return 0;
    }

    while (parts < 2) {
        if (*version < '0' || *version > '9') {
            return 0;
        }
        while (*version >= '0' && *version <= '9') {
            version++;
        }
        parts++;
        if (parts == 2) {
            return 1;
        }
        if (*version != '.') {
            return 0;
        }
        version++;
    }
    return 0;
}

static void expect_result(const char *text, const char *pattern, int want)
{
    int matched = 9;
    int rc = patmat_match(text, pattern, &matched);

    EXPECT(rc == PATMAT_OK);
    EXPECT(matched == want);
}

static void expect_bad(const char *text, const char *pattern)
{
    int matched = 9;

    EXPECT(patmat_match(text, pattern, &matched) == PATMAT_BADPAT);
    EXPECT(matched == 9);
}

static void test_version_string_is_semver(void)
{
    EXPECT(semver_major_minor(PATMAT_VERSION_STRING));
}

static void test_invalid_arguments(void)
{
    int matched = 4;

    EXPECT(patmat_match(NULL, "a", &matched) == PATMAT_INVAL);
    EXPECT(matched == 4);
    EXPECT(patmat_match("a", NULL, &matched) == PATMAT_INVAL);
    EXPECT(matched == 4);
    EXPECT(patmat_match("a", "a", NULL) == PATMAT_INVAL);
    EXPECT(matched == 4);
}

static void test_literals_and_case(void)
{
    char high[] = { (char)0xC9, '\0' };
    char other[] = { (char)0xE9, '\0' };

    expect_result("", "", 1);
    expect_result("hello", "", 0);
    expect_result("", "hello", 0);
    expect_result("hello", "hello", 1);
    expect_result("Hello", "hello", 1);
    expect_result("hello", "HELLO", 1);
    expect_result("hello", "hell", 0);
    expect_result("hello", "ell", 0);
    expect_result("|", "|", 1);
    expect_result("}", "}", 1);
    expect_result("*", "\\*", 1);
    expect_result("a", "\\*", 0);
    expect_result("\\", "\\\\", 1);
    expect_result(high, other, 0);
    expect_result(high, high, 1);
}

static void test_wildcards(void)
{
    expect_result("", "*", 1);
    expect_result("hello", "*", 1);
    expect_result("", "?", 0);
    expect_result("a", "?", 1);
    expect_result("ab", "?", 0);
    expect_result("ab", "??", 1);
    expect_result("ab", "*?", 1);
    expect_result("", "*?", 0);
    expect_result("hello", "h*o", 1);
    expect_result("hello", "*ell*", 1);
    expect_result("hello", "h?llo", 1);
    expect_result("hello", "*a", 0);
    expect_result("xa", "*a", 1);
    expect_result("ab", "a*", 1);
    expect_result("ab", "*a*a", 0);
    expect_result("aa", "*a*a", 1);
    expect_result("Nick!user@host", "*!*@*", 1);
    expect_result("a!b@c.edu", "*!*@*.edu", 1);
    expect_result("a!b@c.com", "*!*@*.edu", 0);
}

static void test_classes(void)
{
    expect_result("b", "[abc]", 1);
    expect_result("d", "[abc]", 0);
    expect_result("B", "[abc]", 1);
    expect_result("b", "[a-c]", 1);
    expect_result("d", "[a-c]", 0);
    expect_result("b", "[A-C]", 1);
    expect_result("b", "[c-a]", 0);
    expect_result("a", "[]", 0);
    expect_result("", "[]", 0);
    expect_result("a", "[^]", 1);
    expect_result("", "[^]", 0);
    expect_result("b", "[^a]", 1);
    expect_result("a", "[^a]", 0);
    expect_result("A", "[^a]", 0);
    expect_result("-", "[-]", 1);
    expect_result("a]", "[a]]", 1);
    expect_result("x", "[a-]x]", 1);
    expect_result("xxb", "*[abc]", 1);
    expect_result("xx", "*[abc]", 0);
    expect_result("xxay", "*[a]*", 1);
    expect_result("q", "[a\\]b]", 0);
    expect_result("]", "[a\\]b]", 1);
}

static void test_words(void)
{
    expect_result("foo", "{foo|bar}", 1);
    expect_result("bar", "{foo|bar}", 1);
    expect_result("BAR", "{foo|bar}", 1);
    expect_result("baz", "{foo|bar}", 0);
    expect_result("foo bar", "{foo}", 0);
    expect_result("foo", "{f*}", 1);
    expect_result("hello", "{*}", 1);
    expect_result("hello world", "{*}", 0);
    expect_result("say bar", "*{bar}", 1);
    expect_result("xx bar yy", "*{bar}*", 1);
    expect_result("foobar", "*{bar}*", 0);
    expect_result("foo bar", "{foo} {bar}", 1);
    expect_result("foo bar", "{foo}{bar}", 0);
    expect_result("foo\tbar", "{foo}*", 1);
    expect_result("foo\nbar", "*{bar}*", 1);
    expect_result("a", "{?}", 1);
    expect_result(" ", "{?}", 0);
    expect_result("foo|bar", "{foo\\|bar}", 1);
    expect_result("foo", "{foo\\|bar}", 0);
}

static void test_bad_patterns(void)
{
    expect_bad("hello", "[");
    expect_bad("hello", "{foo");
    expect_bad("hello", "{foo{bar}}");
    expect_bad("hello", "foo\\");
    expect_bad("hello", "[a-]");
    expect_bad("hello", "[--]");
    expect_bad("a", "[a-\\");
}

static void test_depth_limit(void)
{
    char pattern[161];
    char text[81];
    int matched = 5;
    int i;
    char *cursor = pattern;

    for (i = 0; i < 80; i++) {
        *cursor++ = '*';
        *cursor++ = 'a';
        text[i] = 'a';
    }
    *cursor = '\0';
    text[80] = '\0';
    EXPECT(patmat_match(text, pattern, &matched) == PATMAT_BADPAT);
    EXPECT(matched == 5);
    expect_result("aaa", "*a*a*a", 1);
}

static void run_test(const char *name, void (*fn)(void))
{
    int before = failures;

    current_test = name;
    fn();
    tests_run++;
    if (failures == before) {
        printf("tests/test_patmat.c::%s PASSED\n", name);
    } else {
        tests_failed++;
        printf("tests/test_patmat.c::%s FAILED\n", name);
    }
    fflush(stdout);
}

int main(void)
{
    int passed;

    run_test("test_version_string_is_semver", test_version_string_is_semver);
    run_test("test_invalid_arguments", test_invalid_arguments);
    run_test("test_literals_and_case", test_literals_and_case);
    run_test("test_wildcards", test_wildcards);
    run_test("test_classes", test_classes);
    run_test("test_words", test_words);
    run_test("test_bad_patterns", test_bad_patterns);
    run_test("test_depth_limit", test_depth_limit);

    passed = tests_run - tests_failed;
    if (tests_failed != 0) {
        printf("%d failed, %d passed, %d assertion(s) failed\n",
               tests_failed, passed, failures);
        return 1;
    }
    printf("%d passed\n", passed);
    return 0;
}
