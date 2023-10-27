/*
 * tests/main.c
 * © suhas pai
 */

extern void test_convert();
extern void test_format();
extern void test_time();
extern void test_avltree();
extern void test_bitmap();

int main() {
    test_convert();
    test_format();
    test_time();
    test_avltree();
    test_bitmap();

    return 0;
}