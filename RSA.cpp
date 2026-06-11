#include "RSA.h"
#include <fstream>
#include <cstdint>
#include <vector>
#include <iostream>
#include <random>
#include <chrono>
#include <filesystem>

namespace fs = std::filesystem;

namespace {
    uint64_t powmod(uint64_t a, uint64_t b, uint64_t m) {
        uint64_t res = 1;
        a %= m;
        while (b) {
            if (b & 1) res = (__int128)res * a % m;
            a = (__int128)a * a % m;
            b >>= 1;
        }
        return res;
    }

    uint64_t gcd(uint64_t a, uint64_t b) {
        while (b) { uint64_t t = b; b = a % b; a = t; }
        return a;
    }

    uint64_t mod_inverse(uint64_t a, uint64_t m) {
        int64_t m0 = m, t, q;
        int64_t x0 = 0, x1 = 1;
        if (m == 1) return 0;
        while (a > 1) {
            q = a / m;
            t = m;
            m = a % m;
            a = t;
            t = x0;
            x0 = x1 - q * x0;
            x1 = t;
        }
        if (x1 < 0) x1 += m0;
        return x1;
    }

    bool is_prime(uint64_t n) {
        if (n < 2) return false;
        if (n % 2 == 0) return n == 2;
        for (uint64_t i = 3; i * i <= n; i += 2)
            if (n % i == 0) return false;
            return true;
    }

    uint64_t generate_prime(int bits) {
        std::mt19937_64 rng(std::chrono::steady_clock::now().time_since_epoch().count());
        uint64_t min_val = 1ULL << (bits - 1);
        uint64_t max_val = (1ULL << bits) - 1;
        for (int attempts = 0; attempts < 10000; attempts++) {
            uint64_t candidate = min_val + (rng() % (max_val - min_val + 1));
            if (candidate % 2 == 0) candidate++;
            if (is_prime(candidate)) return candidate;
        }
        return 61;
    }

    void rsa_keygen(uint64_t& n, uint64_t& e, uint64_t& d) {
        uint64_t p = generate_prime(16);
        uint64_t q = generate_prime(16);
        while (p == q) q = generate_prime(16);
        n = p * q;
        uint64_t phi = (p - 1) * (q - 1);
        e = 65537;
        while (gcd(e, phi) != 1) e += 2;
        d = mod_inverse(e, phi);
    }

    bool parse_key(const std::string& key_str, uint64_t& n, uint64_t& e, uint64_t& d) {
        size_t first = key_str.find(',');
        size_t second = key_str.find(',', first + 1);
        if (first == std::string::npos) return false;
        n = std::stoull(key_str.substr(0, first));
        e = std::stoull(key_str.substr(first + 1, second - first - 1));
        if (second != std::string::npos) {
            d = std::stoull(key_str.substr(second + 1));
        } else {
            d = 0;
        }
        return true;
    }
}

void process_rsa(char action, const std::string& key, const std::string& input, const std::string& output) {
    if (action == 2) {
        uint64_t n, e, d;
        rsa_keygen(n, e, d);
        std::cout << n << "," << e << "," << d << std::endl;
        return;
    }

    uint64_t n, e, d;
    if (!parse_key(key, n, e, d)) {
        std::cerr << "Ошибка: неверный формат ключа" << std::endl;
        return;
    }

    uint64_t exp = (action == 1) ? e : d;
    if (exp == 0) {
        std::cerr << "Ошибка: для дешифрования нужен d" << std::endl;
        return;
    }

    std::ifstream infile(input, std::ios::binary);
    if (!infile) {
        std::cerr << "Ошибка: не удалось открыть входной файл" << std::endl;
        return;
    }

    std::vector<char> data((std::istreambuf_iterator<char>(infile)), std::istreambuf_iterator<char>());
    infile.close();

    std::vector<char> out;
    const size_t BLOCK = 2;
    const size_t ENC_BLOCK = 4;
    if (action == 1) {
        // Шифрование
        for (size_t i = 0; i < data.size(); i += BLOCK) {
            uint64_t block = 0;
            size_t bytes = 0;
            for (size_t j = 0; j < BLOCK && i + j < data.size(); j++) {
                block |= (uint64_t)(unsigned char)data[i + j] << (j * 8);
                bytes++;
            }

            // Шифруем блок по формуле: (block ^ exp) % n
            uint64_t enc = powmod(block, exp, n);
            for (size_t j = 0; j < ENC_BLOCK; j++) {
                out.push_back((enc >> (j * 8)) & 0xFF);
            }
            out.push_back(bytes);
        }
    } else {
        // Дешифрование
        for (size_t i = 0; i + ENC_BLOCK < data.size(); i += ENC_BLOCK + 1) {
            uint64_t block = 0;
            for (size_t j = 0; j < ENC_BLOCK; j++) {
                block |= (uint64_t)(unsigned char)data[i + j] << (j * 8);
            }
            // Расшифровываем блок по формуле: (block ^ exp) % n
            uint64_t dec = powmod(block, exp, n);
            // Читаем, сколько байт было в этом блоке изначально
            size_t bytes = (unsigned char)data[i + ENC_BLOCK];
            if (bytes > BLOCK) bytes = BLOCK;
            // Восстанавливаем только исходные байты
            for (size_t j = 0; j < bytes; j++) {
                out.push_back((dec >> (j * 8)) & 0xFF);
            }
        }
    }

    std::ofstream outfile(output, std::ios::binary);
    if (!outfile) {
        std::cerr << "Ошибка: не удалось открыть файл для записи" << std::endl;
        return;
    }
    outfile.write(out.data(), out.size());
    outfile.close();

    std::cout << (action == 1 ? "Шифрование" : "Дешифрование") << " завершено" << std::endl;
}
