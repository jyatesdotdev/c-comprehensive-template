/**
 * @file test_str_path.c
 * @brief Tests for string views (containers/str.h) and paths (systems/path.h).
 */
#include "containers/str.h"
#include "systems/path.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CHECK(cond)                                                                    \
    do {                                                                               \
        if (!(cond)) {                                                                 \
            fprintf(stderr, "CHECK failed at %s:%d: %s\n", __FILE__, __LINE__, #cond); \
            exit(1);                                                                   \
        }                                                                              \
    } while (0)

static void test_sv_basics(void) {
    StrView sv = sv_from("hello world");
    CHECK(sv.len == 11);
    CHECK(sv_eq_cstr(sv, "hello world"));
    CHECK(!sv_eq_cstr(sv, "hello"));
    CHECK(sv_from(NULL).len == 0);
    CHECK(sv_eq(sv_from(NULL), sv_from("")));

    CHECK(sv_starts_with(sv, "hello"));
    CHECK(!sv_starts_with(sv, "world"));
    CHECK(sv_starts_with(sv, ""));
    CHECK(sv_ends_with(sv, "world"));
    CHECK(!sv_ends_with(sv, "hello"));
    CHECK(!sv_starts_with(sv, "hello world plus more"));

    CHECK(sv_eq_cstr(sv_slice(sv, 0, 5), "hello"));
    CHECK(sv_eq_cstr(sv_slice(sv, 6, 11), "world"));
    CHECK(sv_slice(sv, 8, 4).len == 0);              /* start > end clamps */
    CHECK(sv_eq_cstr(sv_slice(sv, 6, 99), "world")); /* end clamps */

    CHECK(sv_eq_cstr(sv_trim(sv_from("  \t hi \n ")), "hi"));
    CHECK(sv_trim(sv_from("   ")).len == 0);
    CHECK(sv_eq_cstr(sv_trim(sv_from("hi")), "hi"));
}

static void test_sv_split(void) {
    StrView rest = sv_from("a,b,,c");
    StrView tok;
    CHECK(sv_split_next(&rest, ',', &tok) && sv_eq_cstr(tok, "a"));
    CHECK(sv_split_next(&rest, ',', &tok) && sv_eq_cstr(tok, "b"));
    CHECK(sv_split_next(&rest, ',', &tok) && tok.len == 0); /* empty field */
    CHECK(sv_split_next(&rest, ',', &tok) && sv_eq_cstr(tok, "c"));
    CHECK(!sv_split_next(&rest, ',', &tok)); /* exhausted */

    /* Single token, no delimiter */
    rest = sv_from("alone");
    CHECK(sv_split_next(&rest, ',', &tok) && sv_eq_cstr(tok, "alone"));
    CHECK(!sv_split_next(&rest, ',', &tok));

    /* Count words in a sentence */
    rest = sv_from("the quick brown fox");
    int words = 0;
    while (sv_split_next(&rest, ' ', &tok)) words++;
    CHECK(words == 4);
}

static void test_sv_convert(void) {
    char buf[8];
    CHECK(sv_to_cstr(sv_from("hello"), buf, sizeof(buf)) == ERR_OK);
    CHECK(strcmp(buf, "hello") == 0);
    CHECK(sv_to_cstr(sv_from("too long!!"), buf, sizeof(buf)) == ERR_OVERFLOW);
    CHECK(sv_to_cstr(sv_from(""), buf, 1) == ERR_OK && buf[0] == '\0');

    long v = 0;
    CHECK(sv_parse_long(sv_from("42"), &v) == ERR_OK && v == 42);
    CHECK(sv_parse_long(sv_from("-17"), &v) == ERR_OK && v == -17);
    CHECK(sv_parse_long(sv_from("+8"), &v) == ERR_OK && v == 8);
    CHECK(sv_parse_long(sv_from(""), &v) == ERR_INVALID_ARG);
    CHECK(sv_parse_long(sv_from("12abc"), &v) == ERR_INVALID_ARG);
    CHECK(sv_parse_long(sv_from("abc"), &v) == ERR_INVALID_ARG);
    CHECK(sv_parse_long(sv_from("999999999999999999999999"), &v) == ERR_OVERFLOW);
}

static void test_path_lexical(void) {
    char out[256];
    CHECK(path_join("a", "b", out, sizeof(out)) == ERR_OK && strcmp(out, "a/b") == 0);
    CHECK(path_join("a/", "b", out, sizeof(out)) == ERR_OK && strcmp(out, "a/b") == 0);
    CHECK(path_join("a", "/abs", out, sizeof(out)) == ERR_OK && strcmp(out, "/abs") == 0);
    CHECK(path_join("", "b", out, sizeof(out)) == ERR_OK && strcmp(out, "b") == 0);
    CHECK(path_join("a", "b", out, 3) == ERR_OVERFLOW);

    char p1[] = "/a/./b//c/../d/";
    CHECK(path_normalize(p1) == ERR_OK && strcmp(p1, "/a/b/d") == 0);
    char p2[] = "a/../../b";
    CHECK(path_normalize(p2) == ERR_OK && strcmp(p2, "../b") == 0);
    char p3[] = "/..";
    CHECK(path_normalize(p3) == ERR_OK && strcmp(p3, "/") == 0);
    char p4[] = "./";
    CHECK(path_normalize(p4) == ERR_OK && strcmp(p4, ".") == 0);
    char p5[] = "a/b/../..";
    CHECK(path_normalize(p5) == ERR_OK && strcmp(p5, ".") == 0);

    CHECK(strcmp(path_basename("/a/b/file.txt"), "file.txt") == 0);
    CHECK(strcmp(path_basename("file.txt"), "file.txt") == 0);
    CHECK(strcmp(path_basename("/a/b/"), "") == 0);

    CHECK(path_dirname("/a/b/file.txt", out, sizeof(out)) == ERR_OK);
    CHECK(strcmp(out, "/a/b") == 0);
    CHECK(path_dirname("file.txt", out, sizeof(out)) == ERR_OK && strcmp(out, ".") == 0);
    CHECK(path_dirname("/root", out, sizeof(out)) == ERR_OK && strcmp(out, "/") == 0);
    CHECK(path_dirname("/a/b/f", out, 3) == ERR_OVERFLOW);

    CHECK(strcmp(path_ext("file.txt"), ".txt") == 0);
    CHECK(strcmp(path_ext("/a.b/file"), "") == 0);
    CHECK(strcmp(path_ext(".hidden"), "") == 0);
    CHECK(strcmp(path_ext("archive.tar.gz"), ".gz") == 0);
}

static void test_path_fs(void) {
    const char *root = "test_path_tmp";
    char        nested[256];
    CHECK(path_join(root, "deep/er/est", nested, sizeof(nested)) == ERR_OK);

    CHECK(!path_exists(nested));
    CHECK(path_mkdirs(nested, 0755) == ERR_OK);
    CHECK(path_exists(nested));
    CHECK(path_is_dir(nested));
    CHECK(path_mkdirs(nested, 0755) == ERR_OK); /* idempotent */

    /* A file blocking the path is an error, not silent success */
    char file_path[256];
    CHECK(path_join(root, "blocker", file_path, sizeof(file_path)) == ERR_OK);
    FILE *f = fopen(file_path, "w");
    CHECK(f && fclose(f) == 0);
    CHECK(path_exists(file_path));
    CHECK(!path_is_dir(file_path));
    CHECK(path_mkdirs(file_path, 0755) == ERR_IO);

    /* Cleanup */
    CHECK(remove(file_path) == 0);
    char lvl2[256], lvl1[256];
    CHECK(path_dirname(nested, lvl2, sizeof(lvl2)) == ERR_OK);
    CHECK(path_dirname(lvl2, lvl1, sizeof(lvl1)) == ERR_OK);
    CHECK(rmdir(nested) == 0);
    CHECK(rmdir(lvl2) == 0);
    CHECK(rmdir(lvl1) == 0);
    CHECK(rmdir(root) == 0);
}

int main(void) {
    test_sv_basics();
    test_sv_split();
    test_sv_convert();
    test_path_lexical();
    test_path_fs();
    printf("All str/path tests passed.\n");
    return 0;
}
