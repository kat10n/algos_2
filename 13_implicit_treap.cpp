/*
 * ============================================================================
 *  13. ДЕКАРТОВО ДЕРЕВО ПО НЕЯВНОМУ КЛЮЧУ ("массив на стероидах")
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 13_implicit_treap.cpp -o itreap && ./itreap
 *  Сначала 12_treap.cpp.
 * ============================================================================
 *
 *  Храним МАССИВ. Ключ узла — его индекс в массиве, но явно его НЕ храним:
 *  индекс = (сколько узлов левее) = вычисляется через size при спуске.
 *  Поэтому вставка в середину не требует сдвигать индексы остальных.
 *
 *  Всё за O(log n) в среднем:
 *    - вставка/удаление элемента в любую позицию
 *    - сумма (min, max, ...) на отрезке
 *    - РАЗВОРОТ отрезка (ленивая пометка rev)
 *    - вырезать кусок и вставить в другое место
 *
 *  split(t, k) -> (первые k элементов, остальные)
 *  merge — тот же, что в 12_treap.cpp.
 *
 *  Ленивый разворот: вместо того чтобы сразу разворачивать всё поддерево,
 *  ставим в корне флаг rev. Перед тем как спуститься в узел, вызываем push:
 *  меняем местами детей и передаём флаг им. Сумма от разворота не меняется.
 *
 *  На контестах очень часто: "дан массив, запросы: развернуть [l, r],
 *  сумма на [l, r], вставить x на позицию i" — это как раз оно.
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <vector>

using namespace std;

mt19937 rng(13);

struct Node {
    long long val;
    long long sum;       // сумма по поддереву
    uint32_t prior;
    int size = 1;
    bool rev = false;    // "поддерево надо развернуть, но детям ещё не передано"
    Node* l = nullptr;
    Node* r = nullptr;
    Node(long long v) : val(v), sum(v), prior(rng()) {}
};

int sz(Node* t) { return t ? t->size : 0; }
long long sm(Node* t) { return t ? t->sum : 0; }

void upd(Node* t) {
    if (!t) return;
    t->size = 1 + sz(t->l) + sz(t->r);
    t->sum = t->val + sm(t->l) + sm(t->r);
}

// Протолкнуть ленивую пометку вниз
void push(Node* t) {
    if (t && t->rev) {
        swap(t->l, t->r);
        if (t->l) t->l->rev ^= true;  // ^= true — переключить флаг (два разворота = ничего)
        if (t->r) t->r->rev ^= true;
        t->rev = false;
    }
}

// Первые k элементов -> первое дерево, остальные -> второе
pair<Node*, Node*> split(Node* t, int k) {
    if (!t) return {nullptr, nullptr};
    push(t);                              // перед тем как смотреть на детей!
    if (sz(t->l) < k) {                   // корень попадает в левую часть
        auto [a, b] = split(t->r, k - sz(t->l) - 1);  // в правом поддереве берём оставшиеся
        t->r = a;
        upd(t);
        return {t, b};
    } else {
        auto [a, b] = split(t->l, k);
        t->l = b;
        upd(t);
        return {a, t};
    }
}

Node* merge(Node* L, Node* R) {
    if (!L) return R;
    if (!R) return L;
    if (L->prior > R->prior) {
        push(L);
        L->r = merge(L->r, R);
        upd(L);
        return L;
    } else {
        push(R);
        R->l = merge(L, R->l);
        upd(R);
        return R;
    }
}

// ============================================================================
// Удобная обёртка-класс над корнем
// ============================================================================
class ImplicitTreap {
public:
    ~ImplicitTreap() { clear(root); }

    int size() const { return sz(root); }

    // вставить val так, чтобы он оказался на позиции pos (0 <= pos <= size)
    void insert(int pos, long long val) {
        auto [L, R] = split(root, pos);
        root = merge(merge(L, new Node(val)), R);
    }

    void pushBack(long long val) { root = merge(root, new Node(val)); }

    void erase(int pos) {
        auto [L, R] = split(root, pos);
        auto [M, R2] = split(R, 1);   // M — ровно один элемент
        delete M;
        root = merge(L, R2);
    }

    // Шаблон "вырезать отрезок [l, r], что-то с ним сделать, вклеить обратно":
    long long sum(int l, int r) {     // сумма на [l, r] включительно
        auto [A, B] = split(root, l);           // A = [0, l)
        auto [M, C] = split(B, r - l + 1);      // M = [l, r], C = (r, end)
        long long res = sm(M);
        root = merge(merge(A, M), C);
        return res;
    }

    void reverse(int l, int r) {      // развернуть [l, r]
        auto [A, B] = split(root, l);
        auto [M, C] = split(B, r - l + 1);
        if (M) M->rev ^= true;        // O(1)! реальный разворот — лениво
        root = merge(merge(A, M), C);
    }

    long long get(int pos) {
        Node* t = root;
        while (true) {
            push(t);
            int ls = sz(t->l);
            if (pos < ls) t = t->l;
            else if (pos == ls) return t->val;
            else { pos -= ls + 1; t = t->r; }
        }
    }

    vector<long long> toVector() {
        vector<long long> out;
        collect(root, out);
        return out;
    }

private:
    Node* root = nullptr;

    static void collect(Node* t, vector<long long>& out) {  // static — метод не использует поля объекта
        if (!t) return;
        push(t);
        collect(t->l, out);
        out.push_back(t->val);
        collect(t->r, out);
    }

    static void clear(Node* t) {
        if (!t) return;
        clear(t->l);
        clear(t->r);
        delete t;
    }
};

int main() {
    ImplicitTreap tr;
    vector<long long> ref;  // наивный массив для сравнения

    for (int iter = 0; iter < 20000; ++iter) {
        int op = rng() % 5;
        int n = (int)ref.size();
        if (op == 0 || n == 0) {
            int pos = rng() % (n + 1);
            long long v = (long long)(rng() % 1000) - 500;
            tr.insert(pos, v);
            ref.insert(ref.begin() + pos, v);
        } else if (op == 1) {
            int pos = rng() % n;
            tr.erase(pos);
            ref.erase(ref.begin() + pos);
        } else if (op == 2) {
            int l = rng() % n, r = rng() % n;
            if (l > r) swap(l, r);
            long long s = 0;
            for (int i = l; i <= r; ++i) s += ref[i];
            assert(tr.sum(l, r) == s);
        } else if (op == 3) {
            int l = rng() % n, r = rng() % n;
            if (l > r) swap(l, r);
            tr.reverse(l, r);
            std::reverse(ref.begin() + l, ref.begin() + r + 1);  // std:: — чтобы не спутать с методом
        } else {
            int pos = rng() % n;
            assert(tr.get(pos) == ref[pos]);
        }
        assert(tr.size() == (int)ref.size());
    }
    assert(tr.toVector() == ref);
    cout << "ImplicitTreap (insert/erase/sum/reverse): OK\n";
}
