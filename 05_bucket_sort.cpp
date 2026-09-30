/*
 * ============================================================================
 *  05. КАРМАННАЯ СОРТИРОВКА (bucket sort)
 *  Кормен 8.4
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 05_bucket_sort.cpp -o bucket && ./bucket
 * ============================================================================
 *
 *  Идея: диапазон значений делим на n равных "карманов" (bucket).
 *  Каждое число кидаем в свой карман за O(1), каждый карман сортируем
 *  (вставками — он маленький), потом склеиваем карманы по порядку.
 *
 *  Пример: 10 чисел из [0, 1), 10 карманов. Число 0.78 -> карман floor(0.78*10) = 7.
 *
 *  Сложность:
 *    - в среднем O(n), ЕСЛИ числа распределены РАВНОМЕРНО.
 *      Доказательство (Кормен 8.4): пусть n_i — размер i-го кармана,
 *      время = O(n) + sum O(n_i^2). n_i ~ Binomial(n, 1/n),
 *      E[n_i^2] = D[n_i] + (E[n_i])^2 = n*(1/n)*(1-1/n) + 1 = 2 - 1/n.
 *      Итого E[время] = O(n) + n * O(2 - 1/n) = O(n).
 *    - в худшем O(n^2): все числа попали в один карман и сортируются вставками.
 *      (Если внутри карманов использовать std::sort, худший станет O(n log n).)
 *
 *  Отличие от counting sort: counting — для ЦЕЛЫХ из маленького диапазона
 *  (один "карман" = одно значение). Bucket — для любых чисел (в т.ч. double),
 *  карман содержит целый поддиапазон значений.
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

// Вставками — в карманах в среднем по 1-2 элемента, для маленьких массивов это быстрее всего
template <typename T>   // работает и для double, и для long long (см. 00_cpp_basics п.11)
void insertionSort(vector<T>& a) {
    for (int i = 1; i < (int)a.size(); ++i) {
        T x = a[i];
        int j = i - 1;
        while (j >= 0 && a[j] > x) {
            a[j + 1] = a[j];
            --j;
        }
        a[j + 1] = x;
    }
}

// ============================================================================
// 1. Классика из Кормена: вещественные числа из [0, 1)
// ============================================================================
void bucketSort01(vector<double>& a) {
    int n = (int)a.size();
    if (n == 0) return;

    vector<vector<double>> buckets(n);  // n пустых карманов
    for (double x : a) {
        int idx = (int)(x * n);          // номер кармана: floor(x * n) в [0, n-1]
        if (idx >= n) idx = n - 1;       // защита, если вдруг x == 1.0
        buckets[idx].push_back(x);
    }

    int pos = 0;
    for (auto& b : buckets) {            // auto& — ссылка, чтобы сортировать сам карман, а не копию
        insertionSort(b);
        for (double x : b) a[pos++] = x;
    }
}

// ============================================================================
// 2. Для произвольных целых (в т.ч. отрицательных и больших)
// Карман для x: (x - mn) * n / (mx - mn + 1) — линейно отображаем [mn, mx] в [0, n-1].
// ВАЖНО: (x - mn) * n может быть огромным -> считаем в __int128 (128-битное целое GCC)
// или в long double. Для учебных задач обычно хватает long long.
// ============================================================================
void bucketSortInt(vector<long long>& a) {
    int n = (int)a.size();
    if (n == 0) return;
    long long mn = *min_element(a.begin(), a.end());
    long long mx = *max_element(a.begin(), a.end());
    if (mn == mx) return;  // все равны

    __int128 range = (__int128)mx - mn + 1;
    vector<vector<long long>> buckets(n);
    for (long long x : a) {
        int idx = (int)(((__int128)x - mn) * n / range);  // гарантированно в [0, n-1]
        buckets[idx].push_back(x);
    }

    int pos = 0;
    for (auto& b : buckets) {
        // std::sort вместо вставок: даже если распределение неравномерное,
        // худший случай будет O(n log n), а не O(n^2)
        sort(b.begin(), b.end());
        for (long long x : b) a[pos++] = x;
    }
}

int main() {
    mt19937 rng(1);
    uniform_real_distribution<double> uni(0.0, 1.0);  // равномерно на [0, 1)

    for (int iter = 0; iter < 300; ++iter) {
        int n = rng() % 300;
        vector<double> a(n);
        for (double& x : a) x = uni(rng);
        vector<double> e = a;
        sort(e.begin(), e.end());
        bucketSort01(a);
        assert(a == e);
    }
    cout << "bucketSort01: OK\n";

    uniform_int_distribution<long long> big(-1'000'000'000'000'000'000LL, 1'000'000'000'000'000'000LL);
    for (int iter = 0; iter < 300; ++iter) {
        int n = rng() % 300;
        vector<long long> a(n);
        for (long long& x : a) x = (iter % 2 ? big(rng) : (long long)(rng() % 10));  // и широкий, и узкий диапазон
        vector<long long> e = a;
        sort(e.begin(), e.end());
        bucketSortInt(a);
        assert(a == e);
    }
    cout << "bucketSortInt: OK\n";
}
