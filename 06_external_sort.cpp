/*
 * ============================================================================
 *  06. ВНЕШНЯЯ СОРТИРОВКА (external sort) + СЛИЯНИЕ K МАССИВОВ
 *  Кнут т.3 гл. 5.4
 *  Запуск: g++ -std=c++17 -O2 -Wall -Wextra 06_external_sort.cpp -o ext && ./ext
 * ============================================================================
 *
 *  Задача: отсортировать файл, который НЕ ПОМЕЩАЕТСЯ в оперативную память.
 *  Пусть N чисел в файле, а в память влезает только M (M << N).
 *
 *  Алгоритм (2 фазы):
 *    Фаза 1 (split): читаем файл кусками по M чисел, каждый кусок сортируем
 *      в памяти (std::sort) и пишем во временный файл. Получаем k = ceil(N/M)
 *      отсортированных файлов ("серий", runs).
 *    Фаза 2 (k-way merge): открываем все k файлов сразу. В min-куче держим
 *      по ОДНОМУ текущему числу из каждого файла (k чисел в памяти).
 *      Достаём минимум -> пишем в ответ -> читаем следующее число из того же
 *      файла и кладём в кучу. Повторяем, пока куча не пуста.
 *
 *  Сложность:
 *    CPU: фаза 1 — (N/M) * O(M log M) = O(N log M); фаза 2 — O(N log k).
 *    Диск (главное!): каждое число прочитано 2 раза и записано 2 раза.
 *  Если k слишком большое (ОС ограничивает число открытых файлов, ~1000),
 *  сливают в несколько проходов: по 100 файлов за раз, потом результаты и т.д.
 *
 *  Здесь всё моделируется на маленьких размерах, чтобы можно было запустить.
 *  В реальности M — это, например, 100 МБ, а N — 100 ГБ.
 */

#include <algorithm>
#include <cassert>
#include <cstdint>
#include <cstdio>     // std::remove — удалить файл
#include <fstream>    // ifstream / ofstream — чтение/запись файлов
#include <iostream>
#include <queue>      // priority_queue
#include <random>
#include <stdexcept>  // runtime_error
#include <string>
#include <vector>

using namespace std;

const size_t CHUNK_SIZE = 1000;  // M — сколько чисел "помещается в память"

// ============================================================================
// Фаза 1: разбить на отсортированные куски
// ============================================================================
// Работа с файлами:
//   ifstream in("file.txt");  — открыть на чтение; in >> x читает число как cin
//   ofstream out("file.txt"); — открыть на запись (перезаписывает файл)
//   while (in >> x) — читает, пока числа есть; когда конец файла, условие false
//   файл закрывается сам, когда переменная in/out выходит из области видимости { }
vector<string> splitToSortedChunks(const string& inputFile, bool firstIsCount) {
    ifstream in(inputFile);
    if (!in.is_open()) throw runtime_error("Не открылся " + inputFile);

    // ВАЖНО: generator.py препода пишет первой строкой n. Если его не пропустить,
    // n будет отсортировано вместе с данными (баг в external_sort.cpp препода).
    if (firstIsCount) {
        size_t n;
        in >> n;
    }

    vector<string> chunkNames;
    vector<int64_t> buffer;
    buffer.reserve(CHUNK_SIZE);

    // лямбда (00_cpp_basics п.9): сортирует буфер и сбрасывает в новый файл.
    // [&] — видит buffer и chunkNames по ссылке.
    auto flush = [&]() {
        sort(buffer.begin(), buffer.end());
        string name = "chunk_" + to_string(chunkNames.size()) + ".tmp";
        ofstream out(name);
        if (!out.is_open()) throw runtime_error("Не открылся " + name);
        for (int64_t x : buffer) out << x << '\n';
        chunkNames.push_back(name);
        buffer.clear();
    };

    int64_t x;
    while (in >> x) {
        buffer.push_back(x);
        if (buffer.size() == CHUNK_SIZE) flush();
    }
    if (!buffer.empty()) flush();  // хвост меньше CHUNK_SIZE

    return chunkNames;
}

// ============================================================================
// Фаза 2: слияние k отсортированных файлов через min-кучу
// ============================================================================
void mergeChunks(const vector<string>& chunkNames, const string& outputFile) {
    // streams[i] — "читалка" i-го файла. ifstream нельзя копировать, поэтому
    // emplace_back — создаёт объект прямо внутри вектора.
    vector<ifstream> streams;
    for (const string& name : chunkNames) streams.emplace_back(name);

    // В куче пары (значение, номер файла). Пары сравниваются сначала по first —
    // т.е. по значению, это нам и нужно. greater -> на вершине МИНИМУМ.
    using Item = pair<int64_t, size_t>;
    priority_queue<Item, vector<Item>, greater<Item>> heap;

    for (size_t i = 0; i < streams.size(); ++i) {
        int64_t x;
        if (streams[i] >> x) heap.push({x, i});  // проверяем: вдруг файл пустой
    }

    ofstream out(outputFile);
    while (!heap.empty()) {
        auto [value, idx] = heap.top();  // structured binding (00_cpp_basics п.8)
        heap.pop();
        out << value << '\n';

        int64_t next;
        if (streams[idx] >> next) heap.push({next, idx});  // следующее из ТОГО ЖЕ файла
    }
}

void externalSort(const string& inputFile, const string& outputFile, bool firstIsCount) {
    vector<string> chunks = splitToSortedChunks(inputFile, firstIsCount);
    mergeChunks(chunks, outputFile);
    for (const string& name : chunks) remove(name.c_str());  // удаляем временные файлы
}

// ============================================================================
// Та же идея в памяти: слить k отсортированных массивов за O(N log k).
// Классическая задача ("Merge k sorted lists" на LeetCode).
// ============================================================================
vector<int> mergeKSorted(const vector<vector<int>>& arrays) {
    // храним (значение, номер массива, индекс в массиве)
    struct Item {
        int value;
        int arr;
        int pos;
        // priority_queue по умолчанию — max-куча, ей нужен operator<.
        // Хотим min-кучу -> "меньше" тот, у кого значение БОЛЬШЕ.
        bool operator<(const Item& other) const { return value > other.value; }
    };
    priority_queue<Item> heap;
    for (int i = 0; i < (int)arrays.size(); ++i) {
        if (!arrays[i].empty()) heap.push({arrays[i][0], i, 0});
    }
    vector<int> result;
    while (!heap.empty()) {
        Item it = heap.top();
        heap.pop();
        result.push_back(it.value);
        if (it.pos + 1 < (int)arrays[it.arr].size()) {
            heap.push({arrays[it.arr][it.pos + 1], it.arr, it.pos + 1});
        }
    }
    return result;
}

int main() {
    mt19937_64 rng(3);

    // Генерируем входной файл в формате generator.py: первая строка n, потом числа
    const size_t N = 25'500;  // -> 26 кусков по 1000
    vector<int64_t> original(N);
    {
        ofstream out("input.tmp");
        out << N << '\n';
        for (auto& x : original) {
            x = (int64_t)(rng() % 2'000'001) - 1'000'000;
            out << x << ' ';
        }
    }  // фигурные скобки -> out закрывается здесь, данные гарантированно записаны на диск

    externalSort("input.tmp", "output.tmp", true);

    // Проверка
    sort(original.begin(), original.end());
    ifstream check("output.tmp");
    vector<int64_t> got;
    int64_t x;
    while (check >> x) got.push_back(x);
    assert(got == original);
    cout << "externalSort: OK (" << N << " чисел, куски по " << CHUNK_SIZE << ")\n";
    remove("input.tmp");
    remove("output.tmp");

    vector<vector<int>> arrays = {{1, 4, 9}, {}, {2, 3, 10, 11}, {0}, {4, 4}};
    vector<int> merged = mergeKSorted(arrays);
    assert((merged == vector<int>{0, 1, 2, 3, 4, 4, 4, 9, 10, 11}));
    cout << "mergeKSorted: OK\n";
}
