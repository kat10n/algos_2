/*
 * ============================================================================
 *  07. БИНАРНЫЙ ПОИСК
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 07_binary_search.cpp -o bs && ./bs
 * ============================================================================
 *
 *  Главная идея, которая избавляет от ошибок с +1/-1:
 *
 *  Есть МОНОТОННЫЙ предикат good(i): сначала false, потом true:
 *        i:     0  1  2  3  4  5  6
 *     good:     F  F  F  T  T  T  T
 *  Ищем первый true. Держим два индекса с ИНВАРИАНТОМ:
 *     good(l) == false,  good(r) == true
 *  Изначально l = -1, r = n ("фиктивные": считаем good(-1)=F, good(n)=T).
 *  Пока r - l > 1: смотрим середину m, если good(m) -> r = m, иначе l = m.
 *  В конце r = l + 1, и r — первый true (или n, если true нет вообще).
 *
 *  Каждый шаг интервал уменьшается вдвое -> O(log n).
 *  Никаких "l <= r", "m + 1", "m - 1" — только этот шаблон.
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

// ============================================================================
// 1. lower_bound: первый индекс i, для которого a[i] >= x. Если нет — n.
//    good(i) = (a[i] >= x) — монотонно, т.к. a отсортирован.
// ============================================================================
int lowerBound(const vector<int>& a, int x) {
    int l = -1, r = (int)a.size();
    while (r - l > 1) {
        int m = l + (r - l) / 2;   // m всегда строго между l и r -> a[m] существует
        if (a[m] >= x) r = m;
        else l = m;
    }
    return r;
}

// upper_bound: первый i, для которого a[i] > x
int upperBound(const vector<int>& a, int x) {
    int l = -1, r = (int)a.size();
    while (r - l > 1) {
        int m = l + (r - l) / 2;
        if (a[m] > x) r = m;
        else l = m;
    }
    return r;
}

// Производные операции
bool contains(const vector<int>& a, int x) {
    int i = lowerBound(a, x);
    return i < (int)a.size() && a[i] == x;
}
int countEqual(const vector<int>& a, int x) { return upperBound(a, x) - lowerBound(a, x); }
// количество элементов на отрезке значений [lo, hi]
int countInRange(const vector<int>& a, int lo, int hi) { return upperBound(a, hi) - lowerBound(a, lo); }

// ============================================================================
// 2. То же через STL (на контестах пишут так)
// ============================================================================
void stlExamples() {
    vector<int> a = {1, 3, 3, 3, 7, 9};
    // lower_bound возвращает ИТЕРАТОР; минус a.begin() -> индекс
    int lb = lower_bound(a.begin(), a.end(), 3) - a.begin();  // 1
    int ub = upper_bound(a.begin(), a.end(), 3) - a.begin();  // 4
    bool has = binary_search(a.begin(), a.end(), 7);          // true
    assert(lb == 1 && ub == 4 && has);

    // Наибольший элемент < x: элемент перед lower_bound
    auto it = lower_bound(a.begin(), a.end(), 7);
    if (it != a.begin()) {
        --it;
        assert(*it == 3);
    }
}

// ============================================================================
// 3. БИНПОИСК ПО ОТВЕТУ
// Если ответ — число, и можно быстро проверить "ответ <= X?" (монотонно по X),
// то ищем ответ бинпоиском по X, даже если никакого массива нет.
// ============================================================================

// 3a. Целый корень: наибольшее x с x*x <= n (n до 1e18)
long long isqrt(long long n) {
    // инвариант: l*l <= n (good), r*r > n (bad)
    long long l = 0, r = 2'000'000'001LL;   // (2e9)^2 = 4e18 > 1e18
    while (r - l > 1) {
        long long m = l + (r - l) / 2;
        // m * m может переполнить long long при m ~ 2e9 -> проверяем через деление
        if (m <= n / m) l = m;
        else r = m;
    }
    return l;
}

// 3b. "Агрессивные коровы" (классика): в стойлах на координатах pos (отсортированы)
// расставить k коров так, чтобы МИНИМАЛЬНОЕ расстояние между ними было максимальным.
// canPlace(d) = "можно ли расставить так, чтобы все расстояния >= d".
// Монотонно: если можно с d, то можно и с меньшим d. true...true false...false
// Ищем последний true.
bool canPlace(const vector<int>& pos, int k, int d) {
    int placed = 1;          // первую ставим в самое левое стойло (жадно — это оптимально)
    int last = pos[0];
    for (int i = 1; i < (int)pos.size(); ++i) {
        if (pos[i] - last >= d) {
            ++placed;
            last = pos[i];
        }
    }
    return placed >= k;
}

int aggressiveCows(const vector<int>& pos, int k) {  // k >= 2, pos отсортирован
    int l = 0;                           // d = 0 — точно можно (good)
    int r = pos.back() - pos[0] + 1;     // больше максимального расстояния — точно нельзя (bad)
    while (r - l > 1) {
        int m = l + (r - l) / 2;
        if (canPlace(pos, k, m)) l = m;
        else r = m;
    }
    return l;
}

// 3c. Шаблон с лямбдой: первое x в [lo, hi], где pred(x) == true.
// Предполагается pred(hi) == true. Лямбды — 00_cpp_basics п.9.
template <typename Pred>
long long firstTrue(long long lo, long long hi, Pred pred) {
    long long l = lo - 1, r = hi;
    while (r - l > 1) {
        long long m = l + (r - l) / 2;
        if (pred(m)) r = m;
        else l = m;
    }
    return r;
}

// ============================================================================
// 4. ВЕЩЕСТВЕННЫЙ БИНПОИСК
// Решить x^3 + x = c (функция возрастает). Вместо условия r - l > eps
// делаем фиксированное число итераций: 100 итераций уменьшают отрезок в 2^100 раз —
// точнее, чем умеет double. И нет риска зациклиться из-за погрешностей.
// ============================================================================
double solveCubic(double c) {
    double l = 0, r = max(1.0, c);  // f(0) = 0 <= c, f(max(1,c)) >= c
    for (int it = 0; it < 100; ++it) {
        double m = (l + r) / 2;
        if (m * m * m + m < c) l = m;
        else r = m;
    }
    return l;
}

int main() {
    mt19937 rng(5);
    for (int iter = 0; iter < 2000; ++iter) {
        int n = rng() % 30;
        vector<int> a(n);
        for (int& x : a) x = rng() % 20;
        sort(a.begin(), a.end());
        int x = (int)(rng() % 24) - 2;
        assert(lowerBound(a, x) == lower_bound(a.begin(), a.end(), x) - a.begin());
        assert(upperBound(a, x) == upper_bound(a.begin(), a.end(), x) - a.begin());
        assert(contains(a, x) == binary_search(a.begin(), a.end(), x));
        assert(countEqual(a, x) == count(a.begin(), a.end(), x));
    }
    cout << "lower/upper bound: OK\n";

    stlExamples();

    assert(isqrt(0) == 0 && isqrt(15) == 3 && isqrt(16) == 4);
    assert(isqrt(1'000'000'000'000'000'000LL) == 1'000'000'000LL);
    cout << "isqrt: OK\n";

    assert(aggressiveCows({1, 2, 4, 8, 9}, 3) == 3);  // коровы в 1, 4, 8 (или 1, 4, 9)
    cout << "aggressiveCows: OK\n";

    // первое x, для которого x^2 >= 50 -> 8
    long long ans = firstTrue(0, 100, [](long long x) { return x * x >= 50; });
    assert(ans == 8);
    cout << "firstTrue: OK\n";

    double x = solveCubic(10);  // x = 2: 8 + 2 = 10
    assert(fabs(x - 2) < 1e-9);
    cout << "solveCubic: OK\n";
}
