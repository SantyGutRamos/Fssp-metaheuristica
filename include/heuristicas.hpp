// heuristicas.hpp
// Heurísticas básicas que usan todas las metaheurísticas:
//   1) NEH (Nawaz, Enscore y Ham, 1983): heurística CONSTRUCTIVA.
//   2) Búsqueda local por reinserción: heurística de MEJORA.
//   3) Vecindades: intercambio (swap) y reinserción (insert).
#pragma once
#include "fssp.hpp"

// ---------------------------------------------------------------------------
// NEH
//  1. Ordenar los trabajos de mayor a menor tiempo total (suma en todas las máquinas).
//  2. Empezar con el primero.
//  3. Insertar cada trabajo siguiente en la posición que dé menor Cmax parcial.
// Con la aceleración de Taillard cuesta O(n²·m).
// ---------------------------------------------------------------------------
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

// ---------------------------------------------------------------------------
// Tamaño de las vecindades (para una permutación de n trabajos):
//   - Intercambio (swap de dos trabajos cualesquiera):  n·(n-1)/2 vecinos.
//   - Reinserción (sacar un trabajo y ponerlo en otra posición): (n-1)² vecinos.
// ---------------------------------------------------------------------------
inline long long tamVecindadIntercambio(long long n) { return n * (n - 1) / 2; }
inline long long tamVecindadReinsercion(long long n) { return (n - 1) * (n - 1); }

// Movimientos aleatorios (se usan en perturbaciones y en el recocido simulado).
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

// ---------------------------------------------------------------------------
// Búsqueda local por reinserción (la que usan Ruiz y Stützle, 2007)
//   Repetir mientras haya mejora:
//     para cada trabajo (en orden aleatorio):
//        sacarlo y reinsertarlo en su MEJOR posición (Taillard);
//        si el Cmax mejora, aceptar el cambio.
// Termina en un ÓPTIMO LOCAL respecto a la vecindad de reinserción.
// ---------------------------------------------------------------------------
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
                pi.insert(pi.begin() + posOriginal, job);   // deshacer
            }
        }
    }
    return cmaxActual;
}

// Búsqueda local por intercambio con "mejor mejora" (best improvement).
// Se incluye para comparar vecindades; revisa los n(n-1)/2 vecinos en cada paso.
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
        if (ba < 0) return actual;          // óptimo local
        std::swap(pi[ba], pi[bb]);
        actual = mejor;
    }
}
