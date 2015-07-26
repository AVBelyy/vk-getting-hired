#include <stddef.h>
#include <stdio.h>

#define BSEARCH_NOT_FOUND (-1)

#define COUNTOF(x) (sizeof(x) / sizeof(*x))
#define SHOULD_BE(arr, x, ans) if ((bs_ans = bsearch(arr, COUNTOF(arr), x)) == ans) {\
                                   printf("OK\n"); \
                               } else { \
                                   printf("FAILED (expected %d, but was %d)\n", ans, bs_ans); \
                               }

int bsearch(int * arr, size_t n, int x) {
    if (n == 0) {
        // Array is empty.
        return BSEARCH_NOT_FOUND;
    }

    if (x < arr[0]) {
        // `x` is smaller than any element in the array.
        return 0;
    }

    if (x >= arr[n - 1]) {
        // `x` is larger than any element in the array.
        return BSEARCH_NOT_FOUND;
    }

    size_t l = 0, r = n - 1;
    size_t m;
    while (r - l > 1) {
        m = l + (r - l) / 2;
        if (x < arr[m]) {
            r = m;
        } else {
            l = m;
        }
    }

    return r;
}

int main() {
    int bs_ans;
    int arr[] = {1, 1, 2, 4, 5, 6};
    int x[] = {0, 1, 2, 3, 4, 5, 6, 7};
    int ans[] = {0, 2, 3, 3, 4, 5, -1, -1};

    for (size_t i = 0; i < COUNTOF(ans); i++) {
        SHOULD_BE(arr, x[i], ans[i]);
    }

    return 0;
}
