#pragma once
#include "heuristicas.hpp"
#include <cmath>
#include <cstdint>

struct ParamSA {
    double tp0 = 5.0;
    double tpf = 0.1;
};

inline Resultado recocidoSimulado(const Instancia& ins, double limiteMs, std::mt19937& rng,
                                  ParamSA prm = {}, Traza* tr = nullptr) {
    Cronometro reloj;
    std::uniform_real_distribution<double> U01(0.0, 1.0);
    const double T0 = temperaturaConstante(ins, prm.tp0);
    const double Tf = temperaturaConstante(ins, prm.tpf);

    Permutacion actual = neh(ins);
    int cActual = makespan(ins, actual);
    Resultado mejor{actual, cActual, 0.0, 0};
    if (tr) tr->anotar(reloj.ms(), cActual);

    long long it = 0;
    double T = T0;
    while (true) {
        if ((it & 63) == 0) {
            double t = reloj.ms();
            if (t >= limiteMs) break;
            T = T0 * std::pow(Tf / T0, t / limiteMs);
        }
        ++it;
        Permutacion vecino = actual;
        movimientoReinsercion(vecino, rng);
        int cVecino = makespan(ins, vecino);
        int delta = cVecino - cActual;
        if (delta <= 0 || U01(rng) < std::exp(-delta / T)) {
            actual.swap(vecino);
            cActual = cVecino;
            if (cActual < mejor.cmax) {
                mejor.pi = actual;
                mejor.cmax = cActual;
                if (tr) tr->anotar(reloj.ms(), cActual);
            }
        }
    }
    mejor.ms = reloj.ms();
    mejor.iteraciones = it;
    return mejor;
}

struct ParamTabu {
    int tenencia = 7;
};

inline Resultado busquedaTabu(const Instancia& ins, double limiteMs, std::mt19937& rng,
                              ParamTabu prm = {}, Traza* tr = nullptr) {
    Cronometro reloj;
    Permutacion actual = neh(ins);
    int cActual = makespan(ins, actual);
    Resultado mejor{actual, cActual, 0.0, 0};
    if (tr) tr->anotar(reloj.ms(), cActual);

    std::vector<long long> tabuHasta(ins.n, 0);
    std::uniform_int_distribution<int> desempate(0, 1 << 30);
    long long it = 0;
    while (reloj.ms() < limiteMs) {
        ++it;
        int bestC = INT32_MAX, bestJob = -1, bestPos = -1, bestDesde = -1, bestTie = 0;
        for (int desde = 0; desde < ins.n; ++desde) {
            int job = actual[desde];
            Permutacion sinJob = actual;
            sinJob.erase(sinJob.begin() + desde);
            auto costos = evaluarInserciones(ins, sinJob, job);
            for (int pos = 0; pos < static_cast<int>(costos.size()); ++pos) {
                if (pos == desde) continue;
                int c = costos[pos];
                bool esTabu = tabuHasta[job] > it;
                bool aspiracion = c < mejor.cmax;
                if (esTabu && !aspiracion) continue;
                int tie = desempate(rng);
                if (c < bestC || (c == bestC && tie < bestTie)) {
                    bestC = c; bestJob = job; bestPos = pos; bestDesde = desde; bestTie = tie;
                }
            }
        }
        if (bestJob < 0) {
            std::fill(tabuHasta.begin(), tabuHasta.end(), 0);
            continue;
        }
        actual.erase(actual.begin() + bestDesde);
        actual.insert(actual.begin() + bestPos, bestJob);
        cActual = bestC;
        tabuHasta[bestJob] = it + prm.tenencia;
        if (cActual < mejor.cmax) {
            mejor.pi = actual;
            mejor.cmax = cActual;
            if (tr) tr->anotar(reloj.ms(), cActual);
        }
    }
    mejor.ms = reloj.ms();
    mejor.iteraciones = it;
    return mejor;
}