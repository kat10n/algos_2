/*
 * ============================================================================
 *  10. КРАСНО-ЧЁРНОЕ ДЕРЕВО (КЧД)
 *  Кормен гл. 13 (код ниже — ровно по Кормену, названия случаев те же)
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 10_red_black.cpp -o rb && ./rb
 * ============================================================================
 *
 *  СВОЙСТВА КЧД:
 *   1. Каждый узел красный или чёрный.
 *   2. Корень чёрный.
 *   3. Все листья (NIL — фиктивные пустые узлы) чёрные.
 *   4. У красного узла оба ребёнка чёрные (нет двух красных подряд).
 *   5. Для каждого узла все пути от него вниз до листьев NIL содержат
 *      одинаковое число чёрных узлов = ЧЁРНАЯ ВЫСОТА bh(x) (сам x не считаем).
 *
 *  ТЕОРЕМА: высота КЧД с n внутренними узлами h <= 2 * log2(n + 1).
 *  Доказательство:
 *   Лемма: поддерево с корнем x содержит >= 2^bh(x) - 1 внутренних узлов.
 *     Индукция по высоте x. Если высота 0, x = NIL: 2^0 - 1 = 0. Верно.
 *     Иначе у x два ребёнка, у каждого чёрная высота >= bh(x) - 1
 *     (bh(x) - 1, если ребёнок чёрный; bh(x), если красный). По предположению
 *     в каждом >= 2^(bh(x)-1) - 1 узлов. Всего >= 2*(2^(bh(x)-1) - 1) + 1 = 2^bh(x) - 1.
 *   По свойству 4 хотя бы половина узлов на любом пути от корня — чёрные,
 *     значит bh(root) >= h/2.
 *   n >= 2^(h/2) - 1  =>  h <= 2*log2(n + 1). ∎
 *
 *  Сравнение с AVL: КЧД сбалансировано слабее (h <= 2 log n против 1.44 log n),
 *  зато при вставке не больше 2 поворотов, при удалении не больше 3.
 *  std::set и std::map внутри — это КЧД.
 *
 *  РЕАЛИЗАЦИЯ: храним указатель на родителя (fixup идёт снизу вверх без рекурсии).
 *  Вместо nullptr используем один общий чёрный узел-страж NIL — тогда можно
 *  писать x->color, не проверяя x на nullptr (у NIL цвет BLACK).
 */

#include <algorithm>
#include <cassert>
#include <iostream>
#include <random>
#include <set>
#include <vector>

using namespace std;

enum Color { RED, BLACK };  // enum — тип с перечисленными значениями

struct Node {
    int key;
    Color color;
    Node* left;
    Node* right;
    Node* parent;
};

// class — как struct, но по умолчанию всё private (недоступно снаружи).
// Используем класс, чтобы спрятать внутри root и NIL.
class RBTree {
public:
    RBTree() {
        NIL = new Node{0, BLACK, nullptr, nullptr, nullptr};
        NIL->left = NIL->right = NIL->parent = NIL;
        root = NIL;
    }

    ~RBTree() {  // деструктор — вызывается автоматически, когда объект уничтожается
        clear(root);
        delete NIL;
    }

    bool contains(int key) const { return find(key) != NIL; }  // const — метод не меняет дерево

    // ------------------------------------------------------------------------
    // ВСТАВКА: обычная вставка в BST, новый узел КРАСНЫЙ.
    // Красный узел не меняет чёрные высоты (свойство 5 цело),
    // но может нарушить свойство 4 (если родитель красный) или 2 (если он корень).
    // Это чинит insertFixup.
    // ------------------------------------------------------------------------
    void insert(int key) {
        Node* y = NIL;     // будущий родитель
        Node* x = root;
        while (x != NIL) {
            y = x;
            if (key == x->key) return;  // уже есть
            x = key < x->key ? x->left : x->right;
        }
        Node* z = new Node{key, RED, NIL, NIL, y};
        if (y == NIL) root = z;
        else if (key < y->key) y->left = z;
        else y->right = z;
        insertFixup(z);
    }

    // ------------------------------------------------------------------------
    // УДАЛЕНИЕ (Кормен 13.4)
    // y — узел, который физически исчезает из своей позиции:
    //   если у z меньше двух детей — сам z; иначе преемник z (он встаёт на место z
    //   и перекрашивается в цвет z, так что "исчезает" цвет преемника).
    // x — узел, который встаёт на старое место y.
    // Если исчез ЧЁРНЫЙ узел — на путях через x стало на 1 чёрный меньше.
    // Считаем, что x несёт "дополнительную чернoту"; eraseFixup её перераспределяет.
    // ------------------------------------------------------------------------
    void erase(int key) {
        Node* z = find(key);
        if (z == NIL) return;

        Node* y = z;
        Color yOriginalColor = y->color;
        Node* x;

        if (z->left == NIL) {
            x = z->right;
            transplant(z, z->right);
        } else if (z->right == NIL) {
            x = z->left;
            transplant(z, z->left);
        } else {
            y = minimum(z->right);          // преемник, у него нет левого ребёнка
            yOriginalColor = y->color;
            x = y->right;
            if (y->parent == z) {
                x->parent = y;              // важно, даже если x == NIL (fixup смотрит на x->parent)
            } else {
                transplant(y, y->right);    // вынимаем y, на его место — его правый ребёнок
                y->right = z->right;
                y->right->parent = y;
            }
            transplant(z, y);               // y встаёт на место z
            y->left = z->left;
            y->left->parent = y;
            y->color = z->color;
        }
        delete z;
        if (yOriginalColor == BLACK) eraseFixup(x);
    }

    // Проверка всех свойств (для тестов). Возвращает чёрную высоту.
    int check() const {
        assert(root->color == BLACK);                   // свойство 2
        assert(NIL->color == BLACK);                    // свойство 3
        return checkRec(root, -1e18, 1e18);
    }

    void inorder(vector<int>& out) const { inorderRec(root, out); }

    int height() const { return heightRec(root); }

private:
    Node* root;
    Node* NIL;

    Node* find(int key) const {
        Node* x = root;
        while (x != NIL && x->key != key) x = key < x->key ? x->left : x->right;
        return x;
    }

    Node* minimum(Node* x) const {
        while (x->left != NIL) x = x->left;
        return x;
    }

    // Повороты как в 08_bst.cpp, но нужно обновить и указатели на родителей,
    // и ссылку из родителя вниз (раньше это делал "return новый корень").
    void leftRotate(Node* x) {
        Node* y = x->right;
        x->right = y->left;
        if (y->left != NIL) y->left->parent = x;
        y->parent = x->parent;
        if (x->parent == NIL) root = y;
        else if (x == x->parent->left) x->parent->left = y;
        else x->parent->right = y;
        y->left = x;
        x->parent = y;
    }

    void rightRotate(Node* y) {
        Node* x = y->left;
        y->left = x->right;
        if (x->right != NIL) x->right->parent = y;
        x->parent = y->parent;
        if (y->parent == NIL) root = x;
        else if (y == y->parent->right) y->parent->right = x;
        else y->parent->left = x;
        x->right = y;
        y->parent = x;
    }

    // ------------------------------------------------------------------------
    // Починка после вставки. z красный, проблема: родитель z тоже красный.
    // (Тогда дед точно есть и он чёрный — корень чёрный, двух красных подряд не было.)
    // Разбираем случай "родитель — ЛЕВЫЙ ребёнок деда", второй зеркальный.
    // y — дядя (брат родителя).
    //
    //  Случай 1: дядя КРАСНЫЙ. Перекрашиваем: родитель и дядя -> чёрные,
    //    дед -> красный. Чёрные высоты не изменились. Но дед мог получить
    //    красного родителя -> продолжаем с z = дед (поднимаемся на 2 уровня).
    //  Случай 2: дядя ЧЁРНЫЙ, z — ПРАВЫЙ ребёнок ("зигзаг").
    //    Левый поворот вокруг родителя -> превращается в случай 3.
    //  Случай 3: дядя ЧЁРНЫЙ, z — ЛЕВЫЙ ребёнок ("прямая линия").
    //    Родитель -> чёрный, дед -> красный, правый поворот вокруг деда. Готово.
    //
    //  Случаи 2-3 выполняются максимум раз -> <= 2 поворотов на вставку.
    // ------------------------------------------------------------------------
    void insertFixup(Node* z) {
        while (z->parent->color == RED) {
            Node* p = z->parent;
            Node* g = p->parent;
            if (p == g->left) {
                Node* y = g->right;                 // дядя
                if (y->color == RED) {              // случай 1
                    p->color = BLACK;
                    y->color = BLACK;
                    g->color = RED;
                    z = g;
                } else {
                    if (z == p->right) {            // случай 2
                        z = p;
                        leftRotate(z);
                        p = z->parent;
                    }
                    p->color = BLACK;               // случай 3
                    g->color = RED;
                    rightRotate(g);
                }
            } else {                                // зеркально
                Node* y = g->left;
                if (y->color == RED) {
                    p->color = BLACK;
                    y->color = BLACK;
                    g->color = RED;
                    z = g;
                } else {
                    if (z == p->left) {
                        z = p;
                        rightRotate(z);
                        p = z->parent;
                    }
                    p->color = BLACK;
                    g->color = RED;
                    leftRotate(g);
                }
            }
        }
        root->color = BLACK;  // свойство 2 (корень мог стать красным в случае 1)
    }

    // u заменяется поддеревом v (в родителе u ссылка перенаправляется на v)
    void transplant(Node* u, Node* v) {
        if (u->parent == NIL) root = v;
        else if (u == u->parent->left) u->parent->left = v;
        else u->parent->right = v;
        v->parent = u->parent;  // v может быть NIL — это нормально, так задумано
    }

    // ------------------------------------------------------------------------
    // Починка после удаления. x "дважды чёрный" (на путях через x не хватает
    // одного чёрного). Если x красный — просто красим его в чёрный (после цикла).
    // Разбираем x — левый ребёнок, w — брат x.
    //
    //  Случай 1: w КРАСНЫЙ. Перекрашиваем w и родителя, левый поворот вокруг
    //    родителя. Теперь брат x чёрный -> один из случаев 2-4.
    //  Случай 2: w чёрный, оба ребёнка w чёрные. Красим w в красный
    //    (убираем чёрный из правой стороны), "лишний чёрный" поднимаем к родителю:
    //    x = родитель. Если родитель был красным — цикл закончится и он станет чёрным.
    //  Случай 3: w чёрный, левый ребёнок w красный, правый чёрный.
    //    Перекраска + правый поворот вокруг w -> случай 4.
    //  Случай 4: w чёрный, правый ребёнок w КРАСНЫЙ.
    //    w берёт цвет родителя, родитель и правый ребёнок w -> чёрные,
    //    левый поворот вокруг родителя. Лишний чёрный устранён, выходим.
    //  => не больше 3 поворотов.
    // ------------------------------------------------------------------------
    void eraseFixup(Node* x) {
        while (x != root && x->color == BLACK) {
            if (x == x->parent->left) {
                Node* w = x->parent->right;
                if (w->color == RED) {                                   // случай 1
                    w->color = BLACK;
                    x->parent->color = RED;
                    leftRotate(x->parent);
                    w = x->parent->right;
                }
                if (w->left->color == BLACK && w->right->color == BLACK) { // случай 2
                    w->color = RED;
                    x = x->parent;
                } else {
                    if (w->right->color == BLACK) {                      // случай 3
                        w->left->color = BLACK;
                        w->color = RED;
                        rightRotate(w);
                        w = x->parent->right;
                    }
                    w->color = x->parent->color;                         // случай 4
                    x->parent->color = BLACK;
                    w->right->color = BLACK;
                    leftRotate(x->parent);
                    x = root;  // выход из цикла
                }
            } else {                                                     // зеркально
                Node* w = x->parent->left;
                if (w->color == RED) {
                    w->color = BLACK;
                    x->parent->color = RED;
                    rightRotate(x->parent);
                    w = x->parent->left;
                }
                if (w->right->color == BLACK && w->left->color == BLACK) {
                    w->color = RED;
                    x = x->parent;
                } else {
                    if (w->left->color == BLACK) {
                        w->right->color = BLACK;
                        w->color = RED;
                        leftRotate(w);
                        w = x->parent->left;
                    }
                    w->color = x->parent->color;
                    x->parent->color = BLACK;
                    w->left->color = BLACK;
                    rightRotate(x->parent);
                    x = root;
                }
            }
        }
        x->color = BLACK;
    }

    int checkRec(Node* t, long long lo, long long hi) const {
        if (t == NIL) return 0;
        assert(lo < t->key && t->key < hi);                          // свойство BST
        if (t->color == RED) {
            assert(t->left->color == BLACK && t->right->color == BLACK);  // свойство 4
        }
        if (t->left != NIL) assert(t->left->parent == t);
        if (t->right != NIL) assert(t->right->parent == t);
        int bl = checkRec(t->left, lo, t->key);
        int br = checkRec(t->right, t->key, hi);
        assert(bl == br);                                            // свойство 5
        return bl + (t->color == BLACK ? 1 : 0);
    }

    void inorderRec(Node* t, vector<int>& out) const {
        if (t == NIL) return;
        inorderRec(t->left, out);
        out.push_back(t->key);
        inorderRec(t->right, out);
    }

    int heightRec(Node* t) const {
        if (t == NIL) return 0;
        return 1 + max(heightRec(t->left), heightRec(t->right));
    }

    void clear(Node* t) {
        if (t == NIL) return;
        clear(t->left);
        clear(t->right);
        delete t;
    }
};

int main() {
    mt19937 rng(10);
    {
        RBTree tree;  // объект в фигурных скобках — деструктор вызовется на "}"
        set<int> ref;
        for (int iter = 0; iter < 200000; ++iter) {
            int key = rng() % 5000;
            int op = rng() % 3;
            if (op == 0) {
                tree.insert(key);
                ref.insert(key);
            } else if (op == 1) {
                tree.erase(key);
                ref.erase(key);
            } else {
                assert(tree.contains(key) == (ref.count(key) > 0));
            }
            if (iter % 5000 == 0) tree.check();
        }
        tree.check();
        vector<int> keys;
        tree.inorder(keys);
        assert(keys == vector<int>(ref.begin(), ref.end()));
        cout << "RB-tree vs std::set: OK\n";
    }
    {
        RBTree tree;
        const int N = 1'000'000;
        for (int i = 0; i < N; ++i) tree.insert(i);
        int bh = tree.check();
        cout << "После вставки 0.." << N - 1 << ": высота " << tree.height()
             << ", чёрная высота " << bh << " (оценка 2*log2(n+1) ~ 40)\n";
    }
}
