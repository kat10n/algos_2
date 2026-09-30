/*
 * ============================================================================
 *  11. ДВОИЧНАЯ КУЧА (binary heap) И HEAP SORT
 *  Кормен гл. 6, Седжвик гл. 9
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 11_heap.cpp -o heap && ./heap
 * ============================================================================
 *
 *  Min-куча — полное двоичное дерево (все уровни заполнены, последний — слева
 *  направо), где каждый родитель <= своих детей. Минимум — в корне.
 *
 *  Хранится в МАССИВЕ, без указателей (индексы с 0):
 *     дети i:    2i+1 и 2i+2
 *     родитель:  (i-1)/2
 *
 *             1               индекс:  0  1  2  3  4  5
 *           /   \             значение: 1  3  2  7  4  5
 *          3     2
 *         / \   /
 *        7   4 5
 *
 *  Высота = floor(log2 n) -> push/pop за O(log n), top за O(1).
 *
 *  Операции:
 *   siftUp(i)   — элемент меньше родителя? меняем с родителем, повторяем выше.
 *   siftDown(i) — элемент больше меньшего из детей? меняем с ним, повторяем ниже.
 *   push(x)     — кладём в конец массива, siftUp.
 *   pop()       — последний элемент ставим в корень, удаляем конец, siftDown(0).
 *
 *  ПОСТРОЕНИЕ ЗА O(n) (важный факт, спрашивают):
 *   Вызываем siftDown для i = n/2-1, ..., 0 (листья и так кучи).
 *   siftDown узла на высоте h стоит O(h). Узлов на высоте h не больше n/2^(h+1).
 *   Итого sum_{h>=0} n/2^(h+1) * h = n/2 * sum h/2^h = n/2 * 2 = O(n).
 *   (Ряд sum h/2^h = 2.) А n push-ей подряд — O(n log n).
 *
 *  HEAP SORT: строим MAX-кучу на массиве за O(n). Потом n-1 раз:
 *   меняем корень (максимум) с последним элементом кучи, уменьшаем размер
 *   кучи на 1, siftDown(0). Максимумы по очереди встают в конец.
 *   O(n log n) всегда, O(1) доп. памяти, НЕ стабильна.
 */

#include <algorithm>
#include <cassert>
#include <functional>
#include <iostream>
#include <queue>
#include <random>
#include <vector>

using namespace std;

// ============================================================================
// 1. MIN-КУЧА
// ============================================================================
class MinHeap {
public:
    MinHeap() = default;  // конструктор по умолчанию: пустая куча

    // Построение из массива за O(n).
    // explicit — запрещает неявное превращение vector в MinHeap (защита от случайностей)
    explicit MinHeap(const vector<int>& a) : h(a) {
        for (int i = (int)h.size() / 2 - 1; i >= 0; --i) siftDown(i);
    }

    void push(int x) {
        h.push_back(x);
        siftUp((int)h.size() - 1);
    }

    int top() const { return h[0]; }  // const — метод не меняет кучу

    void pop() {
        h[0] = h.back();
        h.pop_back();
        if (!h.empty()) siftDown(0);
    }

    int size() const { return (int)h.size(); }
    bool empty() const { return h.empty(); }

private:
    vector<int> h;

    void siftUp(int i) {
        while (i > 0) {
            int p = (i - 1) / 2;
            if (h[p] <= h[i]) break;  // порядок в норме
            swap(h[p], h[i]);
            i = p;
        }
    }

    void siftDown(int i) {
        int n = (int)h.size();
        while (true) {
            int l = 2 * i + 1, r = 2 * i + 2;
            int smallest = i;
            if (l < n && h[l] < h[smallest]) smallest = l;
            if (r < n && h[r] < h[smallest]) smallest = r;
            if (smallest == i) break;  // i меньше обоих детей — стоп
            swap(h[i], h[smallest]);
            i = smallest;
        }
    }
};

// ============================================================================
// 2. HEAP SORT (in-place, max-куча прямо в исходном массиве)
// ============================================================================
// siftDown для max-кучи, которая занимает a[0..n)
void siftDownMax(vector<int>& a, int i, int n) {
    while (true) {
        int l = 2 * i + 1, r = 2 * i + 2;
        int largest = i;
        if (l < n && a[l] > a[largest]) largest = l;
        if (r < n && a[r] > a[largest]) largest = r;
        if (largest == i) break;
        swap(a[i], a[largest]);
        i = largest;
    }
}

void heapSort(vector<int>& a) {
    int n = (int)a.size();
    for (int i = n / 2 - 1; i >= 0; --i) siftDownMax(a, i, n);  // build max-heap, O(n)
    for (int end = n - 1; end > 0; --end) {
        swap(a[0], a[end]);        // максимум — в конец
        siftDownMax(a, 0, end);    // куча теперь a[0..end)
    }
}

// ============================================================================
// 3. std::priority_queue и типичные задачи
// ============================================================================

// k наименьших элементов за O(n log k): держим MAX-кучу размера k.
// Если новый элемент меньше максимума кучи — он вытесняет максимум.
vector<int> kSmallest(const vector<int>& a, int k) {
    priority_queue<int> pq;  // max-куча
    for (int x : a) {
        if ((int)pq.size() < k) {
            pq.push(x);
        } else if (k > 0 && x < pq.top()) {
            pq.pop();
            pq.push(x);
        }
    }
    vector<int> res;
    while (!pq.empty()) {
        res.push_back(pq.top());
        pq.pop();
    }
    reverse(res.begin(), res.end());  // были по убыванию
    return res;
}

// Медиана потока: две кучи. lo — max-куча нижней половины, hi — min-куча верхней.
// Поддерживаем lo.size() == hi.size() или lo.size() == hi.size() + 1.
// Медиана (нижняя) — lo.top(). Каждая операция O(log n).
class RunningMedian {
public:
    void add(int x) {
        if (lo.empty() || x <= lo.top()) lo.push(x);
        else hi.push(x);
        // перебалансировка
        if (lo.size() > hi.size() + 1) {
            hi.push(lo.top());
            lo.pop();
        } else if (hi.size() > lo.size()) {
            lo.push(hi.top());
            hi.pop();
        }
    }
    int median() const { return lo.top(); }

private:
    priority_queue<int> lo;                              // max-куча
    priority_queue<int, vector<int>, greater<int>> hi;   // min-куча
};

int main() {
    mt19937 rng(11);

    // MinHeap: push/pop в перемешку против priority_queue
    MinHeap heap;
    priority_queue<int, vector<int>, greater<int>> ref;
    for (int iter = 0; iter < 100000; ++iter) {
        if (ref.empty() || rng() % 3) {
            int x = rng() % 1000;
            heap.push(x);
            ref.push(x);
        } else {
            assert(heap.top() == ref.top());
            heap.pop();
            ref.pop();
        }
        assert(heap.size() == (int)ref.size());
    }
    cout << "MinHeap: OK\n";

    // Построение за O(n) и извлечение по порядку = сортировка
    vector<int> a(1000);
    for (int& x : a) x = rng() % 100;
    MinHeap built(a);
    vector<int> out;
    while (!built.empty()) {
        out.push_back(built.top());
        built.pop();
    }
    vector<int> s = a;
    sort(s.begin(), s.end());
    assert(out == s);
    cout << "MinHeap(build O(n)): OK\n";

    for (int iter = 0; iter < 500; ++iter) {
        int n = rng() % 300;
        vector<int> b(n);
        for (int& x : b) x = (int)(rng() % 100) - 50;
        vector<int> e = b;
        sort(e.begin(), e.end());
        heapSort(b);
        assert(b == e);
    }
    cout << "heapSort: OK\n";

    assert((kSmallest({5, 1, 9, 3, 7, 2}, 3) == vector<int>{1, 2, 3}));
    cout << "kSmallest: OK\n";

    RunningMedian rm;
    vector<int> seen;
    for (int iter = 0; iter < 2000; ++iter) {
        int x = rng() % 1000;
        rm.add(x);
        seen.push_back(x);
        vector<int> t = seen;
        sort(t.begin(), t.end());
        assert(rm.median() == t[(t.size() - 1) / 2]);
    }
    cout << "RunningMedian: OK\n";
}
