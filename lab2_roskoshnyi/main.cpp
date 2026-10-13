// Компілятор: GCC / Clang / MSVC | Стандарт: C++23  Roskoshnyi Serhii k24
// Збирання (Release): g++ -O3 -std=c++23 main.cpp -o lab_release -pthread

#include <iostream>
#include <vector>
#include <numeric>
#include <execution>
#include <random>
#include <chrono>
#include <fstream>
#include <thread>
#include <iomanip>

// Генерація та збереження вхідних даних у файл
void generate_test_file() {
    size_t size = 1'000'000;
    std::ofstream out("data.txt");
    out << size << "\n";

    std::mt19937 gen(42);
    std::uniform_real_distribution<double> dist(-100.0, 100.0);
    for (size_t i = 0; i < size; ++i) out << dist(gen) << " ";
}

// Завантаження даних із файлу
std::vector<double> load_data(size_t& size) {
    std::ifstream in("data.txt");
    in >> size;
    std::vector<double> data(size);
    for (size_t i = 0; i < size; ++i) in >> data[i];
    return data;
}

// Власний паралельний алгоритм: розбиває дані на K шматків, обробляє у потоках і зводить суму
template <typename T>
T custom_parallel_transform_reduce(const std::vector<T>& data, int K) {
    size_t n = data.size();
    if (K <= 0) K = 1;
    if (static_cast<size_t>(K) > n) K = static_cast<int>(n);

    size_t chunk_size = n / K;
    std::vector<T> partial_results(K, 0);

    {
        std::vector<std::jthread> threads;
        threads.reserve(K);

        for (int i = 0; i < K; ++i) {
            size_t start = i * chunk_size;
            size_t end = (i == K - 1) ? n : start + chunk_size;

            // Кожен потік паралельно виконує transform_reduce для свого діапазону (піднесення до квадрата і додавання)
            threads.emplace_back([&data, start, end, &res = partial_results[i]]() {
                res = std::transform_reduce(
                    data.begin() + start, data.begin() + end, 0.0, std::plus<>(), 
                    [](double x) { return x * x; }
                );
            });
        }
    } // std::jthread автоматично очікує завершення всіх потоків (RAII)

    // Фінальне послідовне зведення проміжних результатів усіх потоків
    return std::accumulate(partial_results.begin(), partial_results.end(), 0.0);
}

int main() {
    generate_test_file();
    size_t size = 0;
    auto data = load_data(size);
    std::cout << "Loaded data: " << size << " elements\n\n";

    // 1. Послідовний алгоритм (без розпаралелювання)
    auto start = std::chrono::high_resolution_clock::now();
    double res_seq = std::transform_reduce(
        data.begin(), data.end(), 0.0, std::plus<>(), [](double x) { return x * x; }
    );
    auto end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_seq = end - start;
    std::cout << "1. Sequential time: " << std::fixed << std::setprecision(4) << time_seq.count() << " ms\n";

    // 2. Бібліотечний паралельний алгоритм (стандартне виконання std::execution::par)
    start = std::chrono::high_resolution_clock::now();
    double res_par = std::transform_reduce(
        std::execution::par,
        data.begin(), data.end(), 0.0, std::plus<>(), [](double x) { return x * x; }
    );
    end = std::chrono::high_resolution_clock::now();
    std::chrono::duration<double, std::milli> time_par = end - start;
    std::cout << "2. Parallel time (std::execution::par): " << time_par.count() << " ms\n\n";

    // 3. Дослідження власного алгоритму при різній кількості потоків (K)
    std::cout << "--- Custom Algorithm Results Table ---\n";
    std::cout << std::setw(4) << "K" << " | " << std::setw(12) << "Time (ms)" << " | " << std::setw(10) << "Speedup" << "\n";
    std::cout << "----------------------------------------\n";

    std::vector<int> k_values = {1, 2, 4, 8, 12, 16, 32, 64};
    int best_k = 1;
    double min_time = 1e9;

    for (int k : k_values) {
        start = std::chrono::high_resolution_clock::now();
        double res_custom = custom_parallel_transform_reduce(data, k);
        end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> time_custom = end - start;

        double speedup = time_seq.count() / time_custom.count();
        std::cout << std::setw(4) << k << " | " 
                  << std::setw(12) << std::fixed << std::setprecision(4) << time_custom.count() << " | " 
                  << std::setw(9) << std::setprecision(2) << speedup << "x\n";

        if (time_custom.count() < min_time) {
            min_time = time_custom.count();
            best_k = k;
        }
    }

    std::cout << "\n[Conclusion]: Best value K = " << best_k << "\n";
    std::cout << "Hardware CPU threads: " << std::thread::hardware_concurrency() << "\n";

    return 0;
}