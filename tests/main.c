/*
 * tests/main.c
 * © suhas pai
 */

extern void test_convert();
extern void test_format();
extern void test_time();
extern void test_avltree();
extern void test_bitmap();
extern void test_hashmap();
extern void test_path();
extern void test_range();
extern void test_redblacktree();

int main() {
    test_convert();
    test_format();
    test_time();
    test_avltree();
    test_bitmap();
    test_hashmap();
    test_path();
    test_range();
    test_redblacktree();

    return 0;
}