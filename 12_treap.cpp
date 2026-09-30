/*
 * ============================================================================
 *  12. ДЕКАРТОВО ДЕРЕВО (treap = tree + heap)
 *  Seidel, Aragon "Randomized Search Trees"; Кормен задача 13-4
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 12_treap.cpp -o treap && ./treap
 * ============================================================================
 *
 *  В каждом узле две величины: ключ key и СЛУЧАЙНЫЙ приоритет prior.
 *   - по key это дерево поиска: левые < key <= правые
 *   - по prior это КУЧА: приоритет родителя больше приоритетов детей
 *
 *  Если нарисовать узлы как точки (key, prior) на плоскости — отсюда "декартово":
 *  корень — точка с максимальным prior; все точки левее — левое поддерево,
 *  правее — правое, и рекурсивно.
 *
 *  Факт: при заданных (различных) ключах и приоритетах дерево ОДНОЗНАЧНО.
 *  Оно такое же, как BST, в который вставляли ключи в порядке убывания приоритета.
 *  Приоритеты случайные -> это BST со вставкой в случайном порядке ->
 *  ожидаемая глубина узла O(log n) (≈ 2 ln n), ожидаемая высота O(log n).
 *  Доказательство (Seidel-Aragon / Кормен 12.4): узел x — предок y тогда и только
 *  тогда, когда у x наибольший приоритет среди ключей между x и y включительно.
 *  Вероятность этого 1/(|rank(x)-rank(y)|+1). Сумма по x -> глубина y ≈ 2 H_n = O(log n).
 *
 *  ДВЕ БАЗОВЫЕ ОПЕРАЦИИ, через которые выражается ВСЁ:
 *   split(t, k) -> (L, R): L — все ключи < k, R — все ключи >= k.     O(log n)
 *   merge(L, R) -> t:      склеивает, если все ключи L < всех ключей R. O(log n)
 *
 *  Через них:
 *   insert(k): (L, R) = split(t, k); t = merge(merge(L, node(k)), R)
 *   erase(k):  (L, R) = split(t, k); (M, R) = split(R, k+1);  // M — все узлы с ключом k
 *              выкидываем один узел из M; склеиваем обратно
 *
 *  Плюс храним size — размер поддерева. Это даёт k-ю порядковую статистику
 *  и "сколько ключей < x" за O(log n) (std::set так не умеет!).
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <iostream>
#include <random>
#include <utility>
#include <vector>

using namespace std;

mt19937 rng(12);

struct Node {
    int key;
    uint32_t prior;        // случайный приоритет
    int size = 1;          // число узлов в поддереве
    Node* l = nullptr;
    Node* r = nullptr;
    Node(int k) : key(k), prior(rng()) {}
};

int sz(Node* t) { return t ? t->size : 0; }

// пересчитать size по детям — вызывать каждый раз, когда поменяли детей
void upd(Node* t) {
    if (t) t->size = 1 + sz(t->l) + sz(t->r);
}

// ============================================================================
// SPLIT(t, k): разрезать дерево на (ключи < k) и (ключи >= k)
//
// Если t->key < k: корень и всё его ЛЕВОЕ поддерево идут в L.
//   Правое поддерево ещё надо разрезать: split(t->r, k) = (a, b).
//   a (ключи < k) становится новым правым поддеревом t. Ответ (t, b).
// Иначе симметрично: корень и правое поддерево идут в R.
//
// Рекурсия идёт по одному пути вниз -> O(высота) = O(log n) в среднем.
// Возвращаем пару деревьев; auto [a, b] = ... — распаковка (00_cpp_basics п.8).
// ============================================================================
pair<Node*, Node*> split(Node* t, int k) {
    if (!t) return {nullptr, nullptr};
    if (t->key < k) {
        auto [a, b] = split(t->r, k);
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

// ============================================================================
// MERGE(L, R): все ключи L меньше всех ключей R.
// Корнем результата должен стать узел с большим приоритетом (свойство кучи):
//   корень L -> его левое поддерево остаётся, правое = merge(L->r, R)
//   корень R -> его правое остаётся, левое = merge(L, R->l)
// ============================================================================
Node* merge(Node* L, Node* R) {
    if (!L) return R;
    if (!R) return L;
    if (L->prior > R->prior) {
        L->r = merge(L->r, R);
        upd(L);
        return L;
    } else {
        R->l = merge(L, R->l);
        upd(R);
        return R;
    }
}

// Вставка (дубликаты разрешены — получается мультимножество)
Node* insert(Node* t, int key) {
    auto [L, R] = split(t, key);
    return merge(merge(L, new Node(key)), R);
}

// Удалить ОДНО вхождение key (если есть)
// Замечание: key + 1 переполнится, если key == INT_MAX — для учебных задач не важно.
Node* eraseOne(Node* t, int key) {
    auto [L, R] = split(t, key);        // L: < key,  R: >= key
    auto [M, R2] = split(R, key + 1);   // M: == key, R2: > key
    if (M) {
        Node* old = M;
        M = merge(M->l, M->r);          // выкидываем корень M (одно вхождение key)
        delete old;
    }
    return merge(merge(L, M), R2);
}

bool contains(Node* t, int key) {
    while (t) {
        if (t->key == key) return true;
        t = key < t->key ? t->l : t->r;
    }
    return false;
}

// k-й по возрастанию (с 0). Требуется 0 <= k < size.
// Слева sz(t->l) элементов: если k меньше — ответ слева; если равно — корень;
// иначе ищем (k - sz(l) - 1)-й справа.
int kth(Node* t, int k) {
    while (true) {
        int ls = sz(t->l);
        if (k < ls) {
            t = t->l;
        } else if (k == ls) {
            return t->key;
        } else {
            k -= ls + 1;
            t = t->r;
        }
    }
}

// Сколько ключей < key
int countLess(Node* t, int key) {
    int res = 0;
    while (t) {
        if (t->key < key) {
            res += sz(t->l) + 1;  // весь левый + корень меньше key
            t = t->r;
        } else {
            t = t->l;
        }
    }
    return res;
}

// ============================================================================
// ВСТАВКА ЧЕРЕЗ ПОВОРОТЫ — как в статье Seidel-Aragon.
// Вставляем как в обычный BST (в лист), потом на обратном ходе рекурсии,
// если у ребёнка приоритет больше, чем у родителя, — поворачиваем, поднимая
// ребёнка (как siftUp в куче). Повороты — как в 08_bst.cpp + пересчёт size.
// Ожидаемое число поворотов на вставку < 2.
// ============================================================================
Node* rotateRight(Node* y) {
    Node* x = y->l;
    y->l = x->r;
    x->r = y;
    upd(y);
    upd(x);
    return x;
}

Node* rotateLeft(Node* x) {
    Node* y = x->r;
    x->r = y->l;
    y->l = x;
    upd(x);
    upd(y);
    return y;
}

Node* insertRot(Node* t, int key) {
    if (!t) return new Node(key);
    if (key < t->key) {
        t->l = insertRot(t->l, key);
        if (t->l->prior > t->prior) t = rotateRight(t);
    } else {
        t->r = insertRot(t->r, key);
        if (t->r->prior > t->prior) t = rotateLeft(t);
    }
    upd(t);
    return t;
}

// ============================================================================
// Служебное
// ============================================================================
void inorder(Node* t, vector<int>& out) {
    if (!t) return;
    inorder(t->l, out);
    out.push_back(t->key);
    inorder(t->r, out);
}

// Проверка: BST по ключу, куча по приоритету, size верный
int check(Node* t) {
    if (!t) return 0;
    if (t->l) assert(t->l->key <= t->key && t->l->prior <= t->prior);
    if (t->r) assert(t->r->key >= t->key && t->r->prior <= t->prior);
    int s = 1 + check(t->l) + check(t->r);
    assert(s == t->size);
    return s;
}

int height(Node* t) { return t ? 1 + max(height(t->l), height(t->r)) : 0; }

void clear(Node* t) {
    if (!t) return;
    clear(t->l);
    clear(t->r);
    delete t;
}

int main() {
    Node* root = nullptr;
    vector<int> ref;  // отсортированный вектор как эталон мультимножества

    for (int iter = 0; iter < 30000; ++iter) {
        int key = rng() % 300;
        int op = rng() % 4;
        if (op == 0) {
            root = insert(root, key);
            ref.insert(upper_bound(ref.begin(), ref.end(), key), key);
        } else if (op == 1) {
            root = eraseOne(root, key);
            auto it = lower_bound(ref.begin(), ref.end(), key);
            if (it != ref.end() && *it == key) ref.erase(it);
        } else if (op == 2) {
            assert(contains(root, key) == binary_search(ref.begin(), ref.end(), key));
            assert(countLess(root, key) == lower_bound(ref.begin(), ref.end(), key) - ref.begin());
        } else if (!ref.empty()) {
            int k = rng() % ref.size();
            assert(kth(root, k) == ref[k]);
        }
    }
    check(root);
    vector<int> keys;
    inorder(root, keys);
    assert(keys == ref);
    cout << "Treap (split/merge) vs эталон: OK\n";
    clear(root);

    // Вставка через повороты, отсортированные ключи — высота всё равно логарифмическая
    Node* r2 = nullptr;
    const int N = 1'000'000;
    for (int i = 0; i < N; ++i) r2 = insertRot(r2, i);
    check(r2);
    cout << "insertRot 0.." << N - 1 << ": высота " << height(r2)
         << " (средняя глубина ~2*ln(n) ~ 28, высота обычно 40-60)\n";
    clear(r2);

    Node* r3 = nullptr;
    for (int i = 0; i < N; ++i) r3 = insert(r3, i);
    cout << "split/merge 0.." << N - 1 << ": высота " << height(r3) << '\n';
    clear(r3);
}
