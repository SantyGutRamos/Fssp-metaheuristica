#pragma once
#include "heuristicas.hpp"
#include <cmath>

enum class Aceptacion { Better, RandomWalk, Recocido };

inline const char* nombreAceptacion(Aceptacion a) {
    switch (a) {
        case Aceptacion::Better: return "Better";
        case Aceptacion::RandomWalk: return "RandomWalk";
        default: return "SA";
    }
}

inline bool aceptar(Aceptacion crit, int cNueva, int cActual, double T, std::mt19937& rng) {
    std::uniform_real_distribution<double> U01(0.0, 1.0);
    switch (crit) {
        case Aceptacion::Better:     return cNueva <= cActual;
        case Aceptacion::RandomWalk: return true;
        case Aceptacion::Recocido:
            if (cNueva <= cActual) return true;
            return U01(rng) <= std::exp(-(cNueva - cActual) / T);
    }
    return false;
}

struct ParamILS {
    Aceptacion criterio = Aceptacion::Better;
    int fuerza = 3;
    double Tp = 0.4;
};

inline Resultado ils(const Instancia& ins, double limiteMs, std::mt19937& rng,
                     ParamILS prm = {}, Traza* tr = nullptr) {
    Cronometro reloj;
    const double T = temperaturaConstante(ins, prm.Tp);

    Permutacion s = neh(ins);
    int cS = busquedaLocal(ins, s, rng);
    Resultado mejor{s, cS, 0.0, 0};
    if (tr) tr->anotar(reloj.ms(), cS);

    long long it = 0;
    while (reloj.ms() < limiteMs) {
        ++it;
        Permutacion s2 = s;
        for (int k = 0; k < prm.fuerza; ++k) movimientoIntercambio(s2, rng);
        int c2 = busquedaLocal(ins, s2, rng);
        if (aceptar(prm.criterio, c2, cS, T, rng)) { s.swap(s2); cS = c2; }
        if (cS < mejor.cmax) {
            mejor.pi = s;
            mejor.cmax = cS;
            if (tr) tr->anotar(reloj.ms(), cS);
        }
    }
    mejor.ms = reloj.ms();
    mejor.iteraciones = it;
    return mejor;
}

struct ParamIG {
    int d = 4;
    double Tp = 0.4;
    bool usarLS = true;
};

inline int destruirYConstruir(const Instancia& ins, Permutacion& pi, int d, std::mt19937& rng,
                              Permutacion* removidosOut = nullptr) {
    Permutacion removidos;
    for (int k = 0; k < d && !pi.empty(); ++k) {
        std::uniform_int_distribution<int> U(0, static_cast<int>(pi.size()) - 1);
        int idx = U(rng);
        removidos.push_back(pi[idx]);
        pi.erase(pi.begin() + idx);
    }
    if (removidosOut) *removidosOut = removidos;
    int c = makespan(ins, pi);
    for (int job : removidos) {
        auto [pos, cc] = mejorInsercion(ins, pi, job);
        pi.insert(pi.begin() + pos, job);
        c = cc;
    }
    return c;
}

inline Resultado iteratedGreedy(const Instancia& ins, double limiteMs, std::mt19937& rng,
                                ParamIG prm = {}, Traza* tr = nullptr) {
    Cronometro reloj;
    const double T = temperaturaConstante(ins, prm.Tp);
    std::uniform_real_distribution<double> U01(0.0, 1.0);

    Permutacion pi = neh(ins);
    int cPi = prm.usarLS ? busquedaLocal(ins, pi, rng) : makespan(ins, pi);
    Resultado mejor{pi, cPi, 0.0, 0};
    if (tr) tr->anotar(reloj.ms(), cPi);

    long long it = 0;
    while (reloj.ms() < limiteMs) {
        ++it;
        Permutacion pi2 = pi;
        int c2 = destruirYConstruir(ins, pi2, prm.d, rng);
        if (prm.usarLS) c2 = busquedaLocal(ins, pi2, rng, c2);
        if (c2 < cPi) {
            pi.swap(pi2); cPi = c2;
            if (cPi < mejor.cmax) {
                mejor.pi = pi; mejor.cmax = cPi;
                if (tr) tr->anotar(reloj.ms(), cPi);
            }
        } else if (U01(rng) <= std::exp(-(c2 - cPi) / T)) {
            pi.swap(pi2); cPi = c2;
        }
    }
    mejor.ms = reloj.ms();
    mejor.iteraciones = it;
    return mejor;
}