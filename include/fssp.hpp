#pragma once

#include <algorithm>
#include <chrono>
#include <fstream>
#include <iostream>
#include <numeric>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

using Permutacion = std::vector<int>;

struct Instancia {
    std::string nombre;
    int n = 0;
    int m = 0;
    int bks = -1;
    std::vector<std::vector<int>> p;

    long long sumaTiempos() const {
        long long s = 0;
        for (const auto& fila : p) for (int x : fila) s += x;
        return s;
    }
};

inline Instancia leerInstancia(const std::string& ruta) {
    std::ifstream in(ruta);
    if (!in) throw std::runtime_error("No se pudo abrir la instancia: " + ruta);
    Instancia ins;
    in >> ins.n >> ins.m >> ins.bks;
    ins.p.assign(ins.n, std::vector<int>(ins.m));
    for (int j = 0; j < ins.n; ++j)
        for (int i = 0; i < ins.m; ++i)
            if (!(in >> ins.p[j][i])) throw std::runtime_error("Archivo incompleto: " + ruta);
    auto barra = ruta.find_last_of("/\\");
    std::string base = (barra == std::string::npos) ? ruta : ruta.substr(barra + 1);
    auto punto = base.find_last_of('.');
    ins.nombre = (punto == std::string::npos) ? base : base.substr(0, punto);
    return ins;
}

inline int makespan(const Instancia& ins, const Permutacion& pi) {
    std::vector<int> c(ins.m, 0);
    for (int job : pi) {
        c[0] += ins.p[job][0];
        for (int i = 1; i < ins.m; ++i)
            c[i] = std::max(c[i], c[i - 1]) + ins.p[job][i];
    }
    return pi.empty() ? 0 : c[ins.m - 1];
}

inline std::vector<std::vector<int>> tablaFin(const Instancia& ins, const Permutacion& pi) {
    std::vector<std::vector<int>> C(pi.size(), std::vector<int>(ins.m, 0));
    for (size_t k = 0; k < pi.size(); ++k)
        for (int i = 0; i < ins.m; ++i) {
            int antesMaquina = (k > 0) ? C[k - 1][i] : 0;
            int antesTrabajo = (i > 0) ? C[k][i - 1] : 0;
            C[k][i] = std::max(antesMaquina, antesTrabajo) + ins.p[pi[k]][i];
        }
    return C;
}

inline std::vector<int> evaluarInserciones(const Instancia& ins, const Permutacion& seq, int job) {
    const int L = static_cast<int>(seq.size());
    const int m = ins.m;
    std::vector<int> e((L + 1) * (m + 1), 0);
    std::vector<int> q((L + 2) * (m + 2), 0);
    auto E = [&](int k, int i) -> int& { return e[k * (m + 1) + i]; };
    auto Q = [&](int k, int i) -> int& { return q[k * (m + 2) + i]; };

    for (int k = 1; k <= L; ++k)
        for (int i = 1; i <= m; ++i)
            E(k, i) = std::max(E(k - 1, i), E(k, i - 1)) + ins.p[seq[k - 1]][i - 1];

    for (int k = L; k >= 1; --k)
        for (int i = m; i >= 1; --i)
            Q(k, i) = std::max(Q(k + 1, i), Q(k, i + 1)) + ins.p[seq[k - 1]][i - 1];

    std::vector<int> res(L + 1);
    std::vector<int> f(m + 1, 0);
    for (int pos = 0; pos <= L; ++pos) {
        int best = 0;
        f[0] = 0;
        for (int i = 1; i <= m; ++i) {
            f[i] = std::max(f[i - 1], E(pos, i)) + ins.p[job][i - 1];
            best = std::max(best, f[i] + Q(pos + 1, i));
        }
        res[pos] = best;
    }
    return res;
}

inline std::pair<int, int> mejorInsercion(const Instancia& ins, const Permutacion& seq, int job) {
    auto v = evaluarInserciones(ins, seq, job);
    int pos = static_cast<int>(std::min_element(v.begin(), v.end()) - v.begin());
    return {pos, v[pos]};
}

using Reloj = std::chrono::steady_clock;

struct Cronometro {
    Reloj::time_point inicio = Reloj::now();
    double ms() const {
        return std::chrono::duration<double, std::milli>(Reloj::now() - inicio).count();
    }
};

struct Traza {
    std::vector<std::pair<double, int>> puntos;
    void anotar(double t, int c) {
        if (puntos.empty() || c < puntos.back().second) puntos.push_back({t, c});
    }
};

struct Resultado {
    Permutacion pi;
    int cmax = 0;
    double ms = 0.0;
    long long iteraciones = 0;
};

inline double rpd(int cmax, int bks) {
    return bks > 0 ? 100.0 * (cmax - bks) / bks : 0.0;
}

inline double temperaturaConstante(const Instancia& ins, double Tp) {
    return Tp * static_cast<double>(ins.sumaTiempos()) / (ins.n * ins.m * 10.0);
}

inline std::string permATexto(const Permutacion& pi) {
    std::ostringstream os;
    for (size_t k = 0; k < pi.size(); ++k) os << (k ? " " : "") << "J" << pi[k] + 1;
    return os.str();
}