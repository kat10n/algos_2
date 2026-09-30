/*
 * ============================================================================
 *  09. AVL-ДЕРЕВО
 *  Кнут т.3 гл. 6.2.3, Седжвик гл. 13
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 09_avl.cpp -o avl && ./avl
 *  Сначала прочитай 08_bst.cpp (повороты и стиль "функция возвращает новый корень").
 * ============================================================================
 *
 *  AVL-дерево — BST, в котором для КАЖДОГО узла высоты левого и правого
 *  поддеревьев отличаются не больше чем на 1.
 *     balance factor: bf(v) = h(left) - h(right) ∈ {-1, 0, 1}
 *
 *  ТЕОРЕМА: высота AVL-дерева из n узлов h < 1.44 * log2(n + 2).
 *  Доказательство: пусть N(h) — МИНИМАЛЬНОЕ число узлов в AVL-дереве высоты h.
 *    N(1) = 1, N(2) = 2.
 *    У самого "тощего" дерева высоты h одно поддерево высоты h-1, другое h-2
 *    (меньше нельзя — нарушится баланс), и оба тоже минимальны:
 *       N(h) = N(h-1) + N(h-2) + 1
 *    Это как числа Фибоначчи: N(h) + 1 = F(h+2) (проверь: N(1)+1 = 2 = F3,
 *    N(2)+1 = 3 = F4). F(k) ≈ φ^k / √5, φ = 1.618...
 *    n >= N(h) ≈ φ^(h+2)/√5 - 1  =>  h <= log_φ(n) + O(1) = 1.44 * log2(n) + O(1). ∎
 *
 *  Поэтому поиск, вставка, удаление — O(log n).
 *
 *  Как поддерживать баланс: в каждом узле храним высоту. После вставки/удаления
 *  на обратном ходе рекурсии (снизу вверх) пересчитываем высоту и, если
 *  |bf| = 2, делаем повороты. 4 случая:
 *
 *   LL: перевес слева, у левого ребёнка перевес тоже слева (или 0)
 *       -> один правый поворот вокруг v
 *            v                  L
 *           / \               /   \
 *          L   C    ==>      A     v
 *         / \               (h+1) / \
 *        A   B                   B   C
 *       (h+1)(h)
 *
 *   LR: перевес слева, у левого ребёнка перевес СПРАВА
 *       -> левый поворот вокруг L (сводим к LL), потом правый вокруг v
 *       ("большой поворот")
 *            v               v                 B
 *           / \             / \              /   \
 *          L   D   ==>     B   D   ==>      L     v
 *         / \             / \              / \   / \
 *        A   B           L   c2           A  c1 c2  D
 *           / \         / \
 *          c1  c2      A   c1
 *
 *   RR, RL — зеркально.
 */

#include <algorithm>
#include <cassert>
#include <cmath>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;

struct Node {
    int key;
    int height = 1;          // высота поддерева с корнем в этом узле (лист = 1)
    Node* left = nullptr;
    Node* right = nullptr;
    Node(int k) : key(k) {}
};

// Высота пустого дерева = 0. Эта функция избавляет от проверок на nullptr везде.
int h(Node* t) { return t ? t->height : 0; }

// Пересчитать высоту по детям. Вызывать ПОСЛЕ изменения детей.
void update(Node* t) { t->height = 1 + max(h(t->left), h(t->right)); }

int bf(Node* t) { return h(t->left) - h(t->right); }

// Повороты как в 08_bst.cpp + пересчёт высот.
// ПОРЯДОК ВАЖЕН: сначала тот, кто стал ниже (y), потом новый корень (x),
// потому что высота x зависит от уже обновлённой высоты y.
Node* rotateRight(Node* y) {
    Node* x = y->left;
    y->left = x->right;
    x->right = y;
    update(y);
    update(x);
    return x;
}

Node* rotateLeft(Node* x) {
    Node* y = x->right;
    x->right = y->left;
    y->left = x;
    update(x);
    update(y);
    return y;
}

// Восстановить баланс в узле t (дети уже сбалансированы, |bf(t)| <= 2)
Node* balance(Node* t) {
    update(t);
    if (bf(t) == 2) {                     // перевес слева
        if (bf(t->left) < 0) {            // случай LR: сначала превращаем в LL
            t->left = rotateLeft(t->left);
        }
        return rotateRight(t);            // случай LL
    }
    if (bf(t) == -2) {                    // перевес справа
        if (bf(t->right) > 0) {           // случай RL
            t->right = rotateRight(t->right);
        }
        return rotateLeft(t);             // случай RR
    }
    return t;                             // баланс в порядке
}

// ============================================================================
// ВСТАВКА — как в BST, только на выходе из рекурсии вызываем balance.
// При вставке достаточно не более одного (одинарного или двойного) поворота,
// но вызывать balance на всём пути проще и асимптотически так же — O(log n).
// ============================================================================
Node* insert(Node* t, int key) {
    if (!t) return new Node(key);
    if (key < t->key)      t->left = insert(t->left, key);
    else if (key > t->key) t->right = insert(t->right, key);
    else return t;         // дубликат — дерево не менялось
    return balance(t);
}

Node* findMin(Node* t) {
    while (t->left) t = t->left;
    return t;
}

// Удалить минимальный узел из поддерева t (НЕ освобождая память — он переедет)
Node* removeMin(Node* t) {
    if (!t->left) return t->right;  // t — минимум, на его место встаёт правый ребёнок
    t->left = removeMin(t->left);
    return balance(t);
}

// ============================================================================
// УДАЛЕНИЕ. Случай с двумя детьми делаем "честно": не копируем ключ,
// а переподвешиваем узел-преемник на место удаляемого (так корректнее,
// если в узле хранятся ещё какие-то данные).
// При удалении может понадобиться O(log n) поворотов (по одному на уровень).
// ============================================================================
Node* erase(Node* t, int key) {
    if (!t) return nullptr;
    if (key < t->key) {
        t->left = erase(t->left, key);
    } else if (key > t->key) {
        t->right = erase(t->right, key);
    } else {
        Node* l = t->left;
        Node* r = t->right;
        delete t;
        if (!r) return l;              // нет правого — поднимаем левого (он лист или nullptr по AVL)
        Node* m = findMin(r);          // преемник
        m->right = removeMin(r);       // вынимаем его из правого поддерева
        m->left = l;                   // и ставим на место удалённого
        return balance(m);
    }
    return balance(t);
}

bool contains(Node* t, int key) {
    while (t && t->key != key) t = key < t->key ? t->left : t->right;
    return t != nullptr;
}

// ============================================================================
// Проверка всех инвариантов (для тестов). Возвращает настоящую высоту.
// lo, hi — допустимые границы ключей в этом поддереве (проверка свойства BST).
// ============================================================================
int checkAVL(Node* t, long long lo, long long hi) {
    if (!t) return 0;
    assert(lo < t->key && t->key < hi);
    int hl = checkAVL(t->left, lo, t->key);
    int hr = checkAVL(t->right, t->key, hi);
    assert(abs(hl - hr) <= 1);                 // AVL-свойство
    assert(t->height == 1 + max(hl, hr));      // высота хранится правильно
    return t->height;
}

void inorder(Node* t, vector<int>& out) {
    if (!t) return;
    inorder(t->left, out);
    out.push_back(t->key);
    inorder(t->right, out);
}

void clear(Node* t) {
    if (!t) return;
    clear(t->left);
    clear(t->right);
    delete t;
}

int main() {
    mt19937 rng(9);
    Node* root = nullptr;
    set<int> ref;

    for (int iter = 0; iter < 200000; ++iter) {
        int key = rng() % 5000;
        int op = rng() % 3;
        if (op == 0) {
            root = insert(root, key);
            ref.insert(key);
        } else if (op == 1) {
            root = erase(root, key);
            ref.erase(key);
        } else {
            assert(contains(root, key) == (ref.count(key) > 0));
        }
        if (iter % 10000 == 0) checkAVL(root, -1e18, 1e18);
    }
    checkAVL(root, -1e18, 1e18);
    vector<int> keys;
    inorder(root, keys);
    assert(keys == vector<int>(ref.begin(), ref.end()));
    cout << "AVL vs std::set: OK\n";
    clear(root);

    // Вставка по возрастанию — то, что убивало обычный BST
    root = nullptr;
    const int N = 1'000'000;
    for (int i = 0; i < N; ++i) root = insert(root, i);
    checkAVL(root, -1e18, 1e18);
    cout << "Высота после вставки 0.." << N - 1 << ": " << h(root)
         << " (log2(n) = " << log2(N) << ", оценка 1.44*log2(n+2) = " << 1.44 * log2(N + 2) << ")\n";
    clear(root);
}
