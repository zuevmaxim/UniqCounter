#include <set>
#include <random>
#include <cassert>
#include <cmath>

const uint8_t BITS_NUMBER = 32;
const uint32_t B = 15; // 32 KB == 2^15 B
const uint32_t M = static_cast<const uint32_t>(1 << B); // 32 KB

/**
 * HyperLogLog algorithm realization.
 * https://en.wikipedia.org/wiki/HyperLogLog
 */
class UniqCounter {
    // no more than 32kb of memory should be used here
private:
    uint8_t* array;

    uint32_t int_hash(uint32_t x) {
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = ((x >> 16) ^ x) * 0x45d9f3b;
        x = (x >> 16) ^ x;
        return x;
    }

    uint8_t first_one_bit(uint32_t x) {
        for (uint8_t i = 0; i < BITS_NUMBER; ++i) {
            if (x & (1 << i)) {
                return static_cast<uint8_t>(i + 1);
            }
        }
        return BITS_NUMBER;
    }

public:
    UniqCounter() {
        array = new uint8_t[M];
        std::fill(array, array + M, 0);
    }

    void add(int x) {
        uint32_t h = int_hash(static_cast<uint32_t>(x));
        uint32_t i = h & ((1 << B) - 1);
        uint32_t w = (h >> B) | (1 << (BITS_NUMBER - B));
        array[i] = std::max(array[i], first_one_bit(w));
    }

    int get_uniq_num() const {
        double z = 0;
        int v = 0;
        for (int i = 0; i < M; ++i) {
            if (array[i] == 0) {
                ++v;
            }
            z += 1.0 / (1 << array[i]);
        }
        double alpha = 0.7213 / (1 + 1.079 / M);
        double E = alpha * M * M / z;
        if (v > 0) {
            return static_cast<int>(M * std::log(static_cast<double>(M) / v));
        }
        return static_cast<int>(E);
    }

    ~UniqCounter() {
        delete[](array);
    }
};

double relative_error(int expected, int got) {
    return abs(got - expected) / (double) expected;
}

int main() {
    std::random_device rd;
    std::mt19937 gen(rd());

    const int N = (int) 1e6;
    for (int k : {1, 10, 1000, 10000, N / 10, N, N * 10}) {
        std::uniform_int_distribution<> dis(1, k);
        std::set<int> all;
        UniqCounter counter;
        for (int i = 0; i < N; i++) {
            int value = dis(gen);
            all.insert(value);
            counter.add(value);
        }
        int expected = (int) all.size();
        int counter_result = counter.get_uniq_num();
        double error = relative_error(expected, counter_result);
        printf("%d numbers in range [1 .. %d], %d uniq, %d result, %.5f relative error\n", N, k, expected, counter_result, error);
        assert(error <= 0.1);
    }

    return 0;
}