#pragma once

static inline int floor_div(int a, int b) { // b > 0
    int q = a / b;
    int r = a % b;
    if (r && a < 0) --q;
    return q;
}

static inline int floor_mod(int a, int b) { // b > 0
    int m = a % b;
    if (m < 0) m += b;
    return m;
}

