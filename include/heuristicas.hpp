#pragma once
#include "fssp.hpp"

inline Permutacion ordenNEH(const Instancia& ins) {
    Permutacion orden(ins.n);
    std::iota(orden.begin(), orden.end(), 0);
    std::vector<long long> total(ins.n, 0);
    for (int j = 0; j < ins.n; ++j)
        for (int i = 0; i < ins.m; ++i) total[j] += ins.p[j][i];
    std::stable_sort(orden.begin(), orden.end(),
                     [&](int a, int b) { return total[a] > total[b]; });
    return orden;
}

inline Permutacion neh(const Instancia& ins) {
    Permutacion orden = ordenNEH(ins);
    Permutacion pi;
    pi.reserve(ins.n);
    for (int job : orden) {
        auto [pos, c] = mejorInsercion(ins, pi, job);
        pi.insert(pi.begin() + pos, job);
    }
    return pi;
}

inline long long tamVecindadIntercambio(long long n) { return n * (n - 1) / 2; }
inline long long tamVecindadReinsercion(long long n) { return (n - 1) * (n - 1); }

inline void movimientoIntercambio(Permutacion& pi, std::mt19937& rng) {
    std::uniform_int_distribution<int> U(0, static_cast<int>(pi.size()) - 1);
    int a = U(rng), b = U(rng);
    while (b == a) b = U(rng);
    std::swap(pi[a], pi[b]);
}

inline void movimientoReinsercion(Permutacion& pi, std::mt19937& rng) {
    std::uniform_int_distribution<int> U(0, static_cast<int>(pi.size()) - 1);
    int desde = U(rng), hasta = U(rng);
    while (hasta == desde) hasta = U(rng);
    int job = pi[desde];
    pi.erase(pi.begin() + desde);
    pi.insert(pi.begin() + hasta, job);
}

inline int busquedaLocal(const Instancia& ins, Permutacion& pi, std::mt19937& rng,
                         int cmaxActual = -1) {
    if (cmaxActual < 0) cmaxActual = makespan(ins, pi);
    bool mejora = true;
    Permutacion orden(pi);
    while (mejora) {
        mejora = false;
        std::shuffle(orden.begin(), orden.end(), rng);
        for (int job : orden) {
            auto it = std::find(pi.begin(), pi.end(), job);
            int posOriginal = static_cast<int>(it - pi.begin());
            pi.erase(it);
            auto [pos, c] = mejorInsercion(ins, pi, job);
            if (c < cmaxActual) {
                pi.insert(pi.begin() + pos, job);
                cmaxActual = c;
                mejora = true;
            } else {
                pi.insert(pi.begin() + posOriginal, job);
            }
        }
    }
    return cmaxActual;
}

inline int busquedaLocalIntercambio(const Instancia& ins, Permutacion& pi) {
    int actual = makespan(ins, pi);
    while (true) {
        int mejor = actual, ba = -1, bb = -1;
        for (int a = 0; a < ins.n; ++a)
            for (int b = a + 1; b < ins.n; ++b) {
                std::swap(pi[a], pi[b]);
                int c = makespan(ins, pi);
                if (c < mejor) { mejor = c; ba = a; bb = b; }
                std::swap(pi[a], pi[b]);
            }
        if (ba < 0) return actual;
        std::swap(pi[ba], pi[bb]);
        actual = mejor;
    }
}