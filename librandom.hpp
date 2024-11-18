#include <random>

uint64_t random(uint64_t start, uint64_t end) {
    std::random_device rd; 
    std::mt19937_64 mersenne(rd());

    std::uniform_int_distribution<std::mt19937_64::result_type> dist(start, end);

    return dist(mersenne);
}