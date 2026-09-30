/*
 * ============================================================================
 *  08. БИНАРНОЕ ДЕРЕВО ПОИСКА (BST)
 *  Кормен гл. 12, Седжвик гл. 12
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 08_bst.cpp -o bst && ./bst
 * ============================================================================
 *
 *  Свойство BST: для каждого узла v
 *     все ключи в ЛЕВОМ поддереве  <  v.key  <  все ключи в ПРАВОМ поддереве
 *  (здесь ключи уникальные, как в std::set)
 *
 *            8
 *          /   \
 *         3     10
 *        / \      \
 *       1   6      14
 *          / \     /
 *         4   7   13
 *
 *  Обход in-order (левое, корень, правое) выдаёт ключи ПО ВОЗРАСТАНИЮ:
 *  1 3 4 6 7 8 10 13 14
 *
 *  Все операции работают за O(h), где h — высота дерева.
 *  Проблема: если вставлять 1, 2, 3, ..., n, дерево выродится в "палку",
 *  h = n, операции O(n). Решение — сбалансированные деревья (AVL, КЧД, декартово),
 *  которые поддерживают h = O(log n) с помощью ПОВОРОТОВ.
 *
 *  Стиль реализации: рекурсивные функции принимают корень поддерева
 *  и ВОЗВРАЩАЮТ новый корень поддерева:
 *       t->left = insert(t->left, key);
 *  Так вставка в пустое место (nullptr) и перестройка после поворотов
 *  обрабатываются одинаково. Этот же стиль в AVL (09) и декартовом (12).
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;

struct Node {
    int key;
    Node* left = nullptr;
    Node* right = nullptr;
    Node(int k) : key(k) {}
};

// ============================================================================
// ПОИСК: идём влево, если ключ меньше, вправо — если больше. O(h)
// ============================================================================
Node* find(Node* t, int key) {
    while (t != nullptr && t->key != key) {
        t = (key < t->key) ? t->left : t->right;  // тернарный оператор: условие ? если_да : если_нет
    }
    return t;  // nullptr, если не нашли
}

// Минимум — самый левый узел, максимум — самый правый
Node* findMin(Node* t) {
    while (t->left) t = t->left;   // t->left != nullptr можно писать короче: t->left
    return t;
}
Node* findMax(Node* t) {
    while (t->right) t = t->right;
    return t;
}

// ============================================================================
// ВСТАВКА: спускаемся как при поиске, в пустое место ставим новый узел
// ============================================================================
Node* insert(Node* t, int key) {
    if (!t) return new Node(key);                 // дошли до пустого места — создаём узел
    if (key < t->key)      t->left = insert(t->left, key);
    else if (key > t->key) t->right = insert(t->right, key);
    // key == t->key — уже есть, ничего не делаем
    return t;                                     // корень поддерева не изменился
}

// ============================================================================
// УДАЛЕНИЕ — 3 случая:
//   1) у узла нет детей          -> просто удаляем
//   2) один ребёнок              -> ребёнок встаёт на место узла
//   3) два ребёнка               -> находим преемника (минимум в правом
//      поддереве — следующий по величине ключ), копируем его ключ в узел,
//      а самого преемника удаляем из правого поддерева (у него нет левого
//      ребёнка, т.к. он минимум -> для него случай 1 или 2)
//   Случаи 1 и 2 объединяются: если нет левого ребёнка — возвращаем правого
//   (который может быть nullptr).
// ============================================================================
Node* erase(Node* t, int key) {
    if (!t) return nullptr;  // ключа нет
    if (key < t->key) {
        t->left = erase(t->left, key);
    } else if (key > t->key) {
        t->right = erase(t->right, key);
    } else {
        if (!t->left) {                 // случаи 1 и 2 (нет левого)
            Node* r = t->right;
            delete t;
            return r;
        }
        if (!t->right) {                // случай 2 (нет правого)
            Node* l = t->left;
            delete t;
            return l;
        }
        Node* succ = findMin(t->right); // случай 3
        t->key = succ->key;
        t->right = erase(t->right, succ->key);
    }
    return t;
}

// ============================================================================
// ПРЕЕМНИК: наименьший ключ > key (key может отсутствовать в дереве).
// Спускаемся от корня: если текущий ключ > key — он кандидат, идём влево
// искать кандидата поменьше; иначе идём вправо.
// Возвращает nullptr, если такого нет.
// ============================================================================
Node* successor(Node* t, int key) {
    Node* best = nullptr;
    while (t) {
        if (t->key > key) {
            best = t;
            t = t->left;
        } else {
            t = t->right;
        }
    }
    return best;
}

// Предшественник: наибольший ключ < key — симметрично
Node* predecessor(Node* t, int key) {
    Node* best = nullptr;
    while (t) {
        if (t->key < key) {
            best = t;
            t = t->right;
        } else {
            t = t->left;
        }
    }
    return best;
}

/*
============================================================================
ПОВОРОТЫ — меняют структуру, сохраняя свойство BST. O(1).

  Правый поворот вокруг y:            Левый поворот вокруг x:

        y                x                  x                  y
       / \              / \                / \                / \
      x   C    ==>     A   y              A   y     ==>      x   C
     / \                  / \                / \            / \
    A   B                B   C              B   C          A   B

  Порядок A < x < B < y < C сохраняется! Меняются только 2 указателя.
  Высота левой стороны уменьшается на 1, правой — увеличивается.
  Это кирпичик для всех сбалансированных деревьев.
============================================================================
*/
Node* rotateRight(Node* y) {
    Node* x = y->left;
    y->left = x->right;  // поддерево B переходит к y
    x->right = y;
    return x;            // x — новый корень этого поддерева
}

Node* rotateLeft(Node* x) {
    Node* y = x->right;
    x->right = y->left;  // поддерево B переходит к x
    y->left = x;
    return y;
}

// ============================================================================
// Обходы и служебное
// ============================================================================
void inorder(Node* t, vector<int>& out) {  // out по ссылке — накапливаем в один вектор
    if (!t) return;
    inorder(t->left, out);
    out.push_back(t->key);
    inorder(t->right, out);
}

int height(Node* t) {
    if (!t) return 0;
    return 1 + max(height(t->left), height(t->right));
}

// Освобождаем память (post-order: сначала детей, потом сам узел)
void clear(Node* t) {
    if (!t) return;
    clear(t->left);
    clear(t->right);
    delete t;
}

int main() {
    mt19937 rng(8);

    // Случайные операции, сравниваем с std::set
    Node* root = nullptr;
    set<int> ref;
    for (int iter = 0; iter < 20000; ++iter) {
        int key = rng() % 500;
        int op = rng() % 3;
        if (op == 0) {
            root = insert(root, key);
            ref.insert(key);
        } else if (op == 1) {
            root = erase(root, key);
            ref.erase(key);
        } else {
            assert((find(root, key) != nullptr) == (ref.count(key) > 0));
            Node* s = successor(root, key);
            auto it = ref.upper_bound(key);  // первый > key
            assert((s == nullptr) == (it == ref.end()));
            if (s) assert(s->key == *it);
        }
    }
    vector<int> keys;
    inorder(root, keys);
    assert(keys == vector<int>(ref.begin(), ref.end()));
    cout << "BST vs std::set: OK\n";

    // Повороты сохраняют in-order
    if (root && root->left) {
        root = rotateRight(root);
        vector<int> after;
        inorder(root, after);
        assert(after == keys);
        cout << "rotateRight сохраняет порядок: OK\n";
    }
    clear(root);

    // Вырождение: вставка по возрастанию
    Node* bad = nullptr;
    for (int i = 0; i < 2000; ++i) bad = insert(bad, i);
    cout << "Высота после вставки 0..1999 по порядку: " << height(bad)
         << "  (палка! поэтому нужна балансировка)\n";
    clear(bad);
}
