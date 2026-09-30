/*
 * ============================================================================
 *  02. СОРТИРОВКИ СРАВНЕНИЕМ
 *  Кормен гл. 2, 7; Седжвик гл. 2
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 02_comparison_sorts.cpp -o sorts && ./sorts
 * ============================================================================
 *
 *  "Сортировка сравнением" узнаёт порядок элементов ТОЛЬКО через вопросы
 *  "a < b?". Для таких сортировок доказана нижняя граница Ω(n log n)
 *  (см. 01_complexity.md, п.4).
 *
 *  Что внутри:
 *    1. bubbleSort       O(n^2)       — только для понимания
 *    2. selectionSort    O(n^2)
 *    3. insertionSort    O(n^2), но O(n) на почти отсортированных; используется
 *                        внутри быстрых сортировок для маленьких кусков
 *    4. mergeSort        O(n log n) всегда, стабильная, O(n) памяти
 *    5. countInversions  подсчёт инверсий через merge sort — частая задача
 *    6. quickSort        O(n log n) в среднем, 3-way partition
 *    7. quickSelect      k-я порядковая статистика за O(n) в среднем
 *    8. как пользоваться std::sort / stable_sort / nth_element
 */

#include <algorithm>
#include <cassert>
#include <string>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

// Генератор случайных чисел. Фиксированный seed -> одинаковые результаты при каждом запуске.
mt19937 rng(12345);

// ============================================================================
// 1. ПУЗЫРЁК
// Идея: проходим по массиву и меняем соседей, стоящих в неправильном порядке.
// После 1-го прохода максимум "всплывает" в конец, после 2-го — второй максимум и т.д.
// ============================================================================
// vector<int>& a — ссылка: сортируем САМ массив, а не копию (см. 00_cpp_basics.md п.4)
void bubbleSort(vector<int>& a) {
    int n = (int)a.size();  // (int) — явно переводим size_t в int, чтобы не было проблем с n-1 при n=0
    for (int pass = 0; pass < n - 1; ++pass) {
        bool swapped = false;
        // последние pass элементов уже на своих местах — их не трогаем
        for (int j = 0; j + 1 < n - pass; ++j) {
            if (a[j] > a[j + 1]) {
                swap(a[j], a[j + 1]);
                swapped = true;
            }
        }
        if (!swapped) break;  // обменов не было -> массив отсортирован (поэтому лучший случай O(n))
    }
}

// ============================================================================
// 2. ВЫБОРОМ
// Идея: на шаге i находим минимум на [i, n) и ставим его на место i.
// Всегда ровно n(n-1)/2 сравнений. Нестабильна (swap может перепрыгнуть равный элемент).
// ============================================================================
void selectionSort(vector<int>& a) {
    int n = (int)a.size();
    for (int i = 0; i < n; ++i) {
        int best = i;  // индекс минимума
        for (int j = i + 1; j < n; ++j) {
            if (a[j] < a[best]) best = j;
        }
        swap(a[i], a[best]);
    }
}

// ============================================================================
// 3. ВСТАВКАМИ
// Идея: как сортируют карты в руке. Префикс a[0..i) уже отсортирован,
// берём a[i] и сдвигаем вправо все большие элементы, пока не найдём место.
// Число операций = n + (число инверсий) -> на почти отсортированном массиве быстро.
// ============================================================================
void insertionSort(vector<int>& a) {
    int n = (int)a.size();
    for (int i = 1; i < n; ++i) {
        int x = a[i];      // запоминаем, т.к. a[i] сейчас затрём сдвигом
        int j = i - 1;
        // строго ">" -> равные не перепрыгиваем -> сортировка стабильна
        while (j >= 0 && a[j] > x) {
            a[j + 1] = a[j];  // сдвигаем больший элемент вправо
            --j;
        }
        a[j + 1] = x;
    }
}

// ============================================================================
// 4. СЛИЯНИЕМ (merge sort) — "разделяй и властвуй"
//   1) делим массив пополам
//   2) рекурсивно сортируем половины
//   3) сливаем два отсортированных массива за O(n): два указателя,
//      каждый раз берём меньший из текущих элементов
//   T(n) = 2T(n/2) + O(n) = O(n log n) (мастер-теорема, случай 2)
//
// Работаем с полуинтервалами [l, r) — как в STL. Так меньше +1/-1 ошибок:
// длина = r - l, середина m делит на [l, m) и [m, r).
// ============================================================================

// Сливает отсортированные a[l..m) и a[m..r) в a[l..r). buf — общий вспомогательный массив,
// выделяем его ОДИН раз, а не на каждом уровне рекурсии (это заметно быстрее).
void mergeRange(vector<int>& a, int l, int m, int r, vector<int>& buf) {
    int i = l, j = m, k = l;
    while (i < m && j < r) {
        // "<=" : при равенстве берём из ЛЕВОЙ половины -> стабильность
        if (a[i] <= a[j]) buf[k++] = a[i++];   // buf[k++] = ... : записали в buf[k], потом k += 1
        else              buf[k++] = a[j++];
    }
    while (i < m) buf[k++] = a[i++];  // дописываем остаток левой половины
    while (j < r) buf[k++] = a[j++];  // или правой (выполнится только один из циклов)
    for (int t = l; t < r; ++t) a[t] = buf[t];
}

void mergeSortRec(vector<int>& a, int l, int r, vector<int>& buf) {
    if (r - l <= 1) return;           // 0 или 1 элемент — уже отсортировано
    int m = l + (r - l) / 2;          // так пишут, чтобы l + r не переполнилось (для int близких к 2e9)
    mergeSortRec(a, l, m, buf);
    mergeSortRec(a, m, r, buf);
    mergeRange(a, l, m, r, buf);
}

void mergeSort(vector<int>& a) {
    vector<int> buf(a.size());
    mergeSortRec(a, 0, (int)a.size(), buf);
}

// ============================================================================
// 5. ЧИСЛО ИНВЕРСИЙ (пары i < j, где a[i] > a[j]) за O(n log n)
// Частая задача на контестах. Модифицируем слияние: когда берём элемент из
// ПРАВОЙ половины (a[j] < a[i]), он меньше всех оставшихся в левой половине,
// а их ровно (m - i) штук -> все они образуют с ним инверсии.
// Ответ может быть до n(n-1)/2 ~ 5e11 при n = 1e6 -> нужен long long.
// ============================================================================
long long mergeCount(vector<int>& a, int l, int r, vector<int>& buf) {
    if (r - l <= 1) return 0;
    int m = l + (r - l) / 2;
    long long inv = mergeCount(a, l, m, buf) + mergeCount(a, m, r, buf);
    int i = l, j = m, k = l;
    while (i < m && j < r) {
        if (a[i] <= a[j]) {
            buf[k++] = a[i++];
        } else {
            inv += m - i;          // a[j] меньше a[i], a[i+1], ..., a[m-1]
            buf[k++] = a[j++];
        }
    }
    while (i < m) buf[k++] = a[i++];
    while (j < r) buf[k++] = a[j++];
    for (int t = l; t < r; ++t) a[t] = buf[t];
    return inv;
}

long long countInversions(vector<int> a) {  // по значению (копия), т.к. сортировка портит массив
    vector<int> buf(a.size());
    return mergeCount(a, 0, (int)a.size(), buf);
}

// ============================================================================
// 6. БЫСТРАЯ СОРТИРОВКА (quick sort)
//   1) выбираем опорный элемент pivot (СЛУЧАЙНЫЙ — иначе на отсортированном
//      массиве или специальном тесте будет O(n^2))
//   2) разбиваем массив на три части: < pivot | == pivot | > pivot
//   3) рекурсивно сортируем первую и третью части
//
// Почему 3 части (3-way partition, "задача голландского флага")?
// Обычное разбиение на 2 части на массиве из одинаковых элементов работает O(n^2).
// Здесь все равные pivot сразу оказываются на месте и больше не трогаются.
//
// Инвариант во время разбиения отрезка [l, r):
//   [l, lt)   < pivot
//   [lt, i)  == pivot
//   [i, gt)     ещё не просмотрены
//   [gt, r)   > pivot
// ============================================================================
void quickSortRec(vector<int>& a, int l, int r) {
    // while вместо рекурсии для большей части — см. ниже
    while (r - l > 1) {
        int pivot = a[l + rng() % (r - l)];  // случайный индекс из [l, r)
        int lt = l, i = l, gt = r;
        while (i < gt) {
            if (a[i] < pivot) {
                swap(a[lt], a[i]);
                ++lt; ++i;
            } else if (a[i] > pivot) {
                --gt;
                swap(a[i], a[gt]);  // i НЕ увеличиваем: на место i пришёл непросмотренный элемент
            } else {
                ++i;
            }
        }
        // Рекурсивно идём в МЕНЬШУЮ часть, а большую обрабатываем следующей итерацией while.
        // Так глубина рекурсии гарантированно O(log n) (каждый раз размер хотя бы вдвое меньше).
        // Можно проще: quickSortRec(a, l, lt); quickSortRec(a, gt, r); return;
        if (lt - l < r - gt) {
            quickSortRec(a, l, lt);
            l = gt;
        } else {
            quickSortRec(a, gt, r);
            r = lt;
        }
    }
}

void quickSort(vector<int>& a) {
    quickSortRec(a, 0, (int)a.size());
}

// ============================================================================
// 7. K-Я ПОРЯДКОВАЯ СТАТИСТИКА (quickselect) — k-й по величине элемент (с 0)
// То же разбиение, но идём только в ТУ часть, где лежит k.
// В среднем n + n/2 + n/4 + ... = O(n). (Кормен гл. 9)
// ============================================================================
int quickSelect(vector<int> a, int k) {  // копия, т.к. массив переставляется
    int l = 0, r = (int)a.size();
    while (true) {
        int pivot = a[l + rng() % (r - l)];
        int lt = l, i = l, gt = r;
        while (i < gt) {
            if (a[i] < pivot) swap(a[lt++], a[i++]);
            else if (a[i] > pivot) swap(a[i], a[--gt]);
            else ++i;
        }
        if (k < lt) r = lt;          // ответ среди меньших
        else if (k >= gt) l = gt;    // ответ среди больших
        else return pivot;           // k попал в блок равных pivot
    }
}

// ============================================================================
// 8. СТАНДАРТНАЯ БИБЛИОТЕКА (на контестах пишут именно это)
// ============================================================================
struct Student {
    string name;
    int grade;
};

void stdExamples() {
    vector<int> a = {5, 2, 8, 1, 9, 3};

    sort(a.begin(), a.end());                    // по возрастанию, introsort: O(n log n) гарантированно
    sort(a.begin(), a.end(), greater<int>());    // по убыванию

    // Своё правило сравнения через лямбду: сначала чётные, потом нечётные, внутри — по возрастанию
    sort(a.begin(), a.end(), [](int x, int y) {
        if (x % 2 != y % 2) return x % 2 == 0;   // чётное "меньше" нечётного
        return x < y;
    });

    // stable_sort — стабильная (merge sort внутри). Нужна, когда важен порядок равных.
    vector<Student> st = {{"Аня", 5}, {"Боря", 4}, {"Вика", 5}, {"Гоша", 4}};
    stable_sort(st.begin(), st.end(), [](const Student& x, const Student& y) {
        return x.grade > y.grade;               // по убыванию оценки
    });
    // Результат: Аня 5, Вика 5, Боря 4, Гоша 4 — внутри оценки порядок исходный

    // nth_element — quickselect из STL: после вызова на месте k стоит k-й элемент,
    // слева все <= него, справа все >=. O(n) в среднем.
    vector<int> b = {9, 1, 8, 2, 7, 3};
    nth_element(b.begin(), b.begin() + 2, b.end());
    assert(b[2] == 3);  // 0-й: 1, 1-й: 2, 2-й: 3
}

// ============================================================================
// ТЕСТЫ
// ============================================================================
// SortFn — "указатель на функцию", принимающую vector<int>& и ничего не возвращающую.
// Позволяет передать функцию сортировки как параметр.
using SortFn = void (*)(vector<int>&);

void testSort(SortFn sortFn, const string& name, int maxN) {
    for (int iter = 0; iter < 300; ++iter) {
        int n = rng() % (maxN + 1);
        vector<int> a(n);
        // маленький диапазон значений -> много повторов (проверяем и этот случай)
        for (int& x : a) x = (int)(rng() % 21) - 10;
        vector<int> expected = a;
        sort(expected.begin(), expected.end());
        sortFn(a);
        assert(a == expected);
    }
    cout << name << ": OK\n";
}

int main() {
    testSort(bubbleSort, "bubbleSort", 100);
    testSort(selectionSort, "selectionSort", 100);
    testSort(insertionSort, "insertionSort", 100);
    testSort(mergeSort, "mergeSort", 5000);
    testSort(quickSort, "quickSort", 5000);

    // Инверсии: сравниваем с наивным O(n^2)
    for (int iter = 0; iter < 200; ++iter) {
        int n = rng() % 200;
        vector<int> a(n);
        for (int& x : a) x = rng() % 50;
        long long naive = 0;
        for (int i = 0; i < n; ++i)
            for (int j = i + 1; j < n; ++j)
                if (a[i] > a[j]) ++naive;
        assert(countInversions(a) == naive);
    }
    cout << "countInversions: OK\n";

    for (int iter = 0; iter < 200; ++iter) {
        int n = rng() % 200 + 1;
        vector<int> a(n);
        for (int& x : a) x = rng() % 30;
        vector<int> s = a;
        sort(s.begin(), s.end());
        int k = rng() % n;
        assert(quickSelect(a, k) == s[k]);
    }
    cout << "quickSelect: OK\n";

    // Худшие случаи для наивного quicksort: отсортированный и все одинаковые — у нас быстро
    vector<int> sorted(1'000'000), same(1'000'000, 7);
    for (int i = 0; i < (int)sorted.size(); ++i) sorted[i] = i;
    quickSort(sorted);
    quickSort(same);
    cout << "quickSort на 1e6 отсортированных/одинаковых: OK\n";

    stdExamples();
    cout << "std examples: OK\n";
}
