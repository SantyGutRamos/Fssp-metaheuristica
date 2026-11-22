// fssp.hpp
// Problema de Flow Shop de Permutación (PFSP / FSSP).
//  - n trabajos, m máquinas.
//  - Todos los trabajos pasan por las máquinas en el mismo orden M1, M2, ..., Mm.
//  - p[j][i] = tiempo del trabajo j en la máquina i.
//  - Una solución es una permutación (orden) de los trabajos.
//  - Objetivo: minimizar el makespan Cmax (momento en que termina el último
//    trabajo en la última máquina).
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
    int n = 0;                         // número de trabajos
    int m = 0;                         // número de máquinas
    int bks = -1;                      // mejor solución conocida (óptimo), -1 si no se sabe
    std::vector<std::vector<int>> p;   // p[j][i]: trabajo j, máquina i

    long long sumaTiempos() const {
        long long s = 0;
        for (const auto& fila : p) for (int x : fila) s += x;
        return s;
    }
};

// Formato del archivo (ver instances/README.md):
//   línea 1: n m bks
//   luego n líneas, una por trabajo, con sus m tiempos de procesamiento.
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

// ---------------------------------------------------------------------------
// Makespan: recurrencia clásica
//   C(j,i) = max( C(j-1,i), C(j,i-1) ) + p(j,i)
// Solo necesitamos la fila anterior, así que la memoria es O(m) y el tiempo O(n·m).
// Sirve también para secuencias parciales (menos de n trabajos).
// ---------------------------------------------------------------------------
inline int makespan(const Instancia& ins, const Permutacion& pi) {
    std::vector<int> c(ins.m, 0);
    for (int job : pi) {
        c[0] += ins.p[job][0];
        for (int i = 1; i < ins.m; ++i)
            c[i] = std::max(c[i], c[i - 1]) + ins.p[job][i];
    }
    return pi.empty() ? 0 : c[ins.m - 1];
}

// Tabla completa de tiempos de fin C[k][i] (k = posición en la secuencia).
// Se usa para dibujar diagramas de Gantt y para las trazas paso a paso.
inline std::vector<std::vector<int>> tablaFin(const Instancia& ins, const Permutacion& pi) {
    std::vector<std::vector<int>> C(pi.size(), std::vector<int>(ins.m, 0));
    for (size_t k = 0; k < pi.size(); ++k)
        for (int i = 0; i < ins.m; ++i) {
            int antesMaquina = (k > 0) ? C[k - 1][i] : 0;   // la máquina se libera
            int antesTrabajo = (i > 0) ? C[k][i - 1] : 0;   // el trabajo sale de la máquina anterior
            C[k][i] = std::max(antesMaquina, antesTrabajo) + ins.p[pi[k]][i];
        }
    return C;
}

// ---------------------------------------------------------------------------
// Aceleración de Taillard (1990)
// Evalúa en O(n·m) el makespan de insertar `job` en TODAS las posiciones de `seq`
// (en vez de O(n²·m) recalculando cada una desde cero).
//   e[k][i]: tiempo de fin "desde el inicio" del k-ésimo trabajo en la máquina i.
//   q[k][i]: tiempo "desde el final" (cola) del k-ésimo trabajo en la máquina i.
//   f[i]   : tiempo de fin del trabajo insertado en la posición k, máquina i.
//   Cmax(k) = max_i ( f[i] + q[k][i] )
// Devuelve un vector de tamaño |seq|+1 con el Cmax de cada posición.
// ---------------------------------------------------------------------------
inline std::vector<int> evaluarInserciones(const Instancia& ins, const Permutacion& seq, int job) {
    const int L = static_cast<int>(seq.size());
    const int m = ins.m;
    // Matrices con relleno de ceros para evitar casos borde.
    std::vector<int> e((L + 1) * (m + 1), 0);   // fila 0 = "antes del primer trabajo"
    std::vector<int> q((L + 2) * (m + 2), 0);   // fila L+1 = "después del último trabajo"
    auto E = [&](int k, int i) -> int& { return e[k * (m + 1) + i]; };      // k en 0..L, i en 0..m
    auto Q = [&](int k, int i) -> int& { return q[k * (m + 2) + i]; };      // k en 1..L+1, i en 1..m+1

    for (int k = 1; k <= L; ++k)
        for (int i = 1; i <= m; ++i)
            E(k, i) = std::max(E(k - 1, i), E(k, i - 1)) + ins.p[seq[k - 1]][i - 1];

    for (int k = L; k >= 1; --k)
        for (int i = m; i >= 1; --i)
            Q(k, i) = std::max(Q(k + 1, i), Q(k, i + 1)) + ins.p[seq[k - 1]][i - 1];

    std::vector<int> res(L + 1);
    std::vector<int> f(m + 1, 0);
    for (int pos = 0; pos <= L; ++pos) {        // insertar antes de seq[pos]
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

// Mejor posición para insertar `job` (en empate, la primera). Devuelve {posición, Cmax}.
inline std::pair<int, int> mejorInsercion(const Instancia& ins, const Permutacion& seq, int job) {
    auto v = evaluarInserciones(ins, seq, job);
    int pos = static_cast<int>(std::min_element(v.begin(), v.end()) - v.begin());
    return {pos, v[pos]};
}

// ---------------------------------------------------------------------------
// Utilidades comunes a todos los algoritmos
// ---------------------------------------------------------------------------
using Reloj = std::chrono::steady_clock;

struct Cronometro {
    Reloj::time_point inicio = Reloj::now();
    double ms() const {
        return std::chrono::duration<double, std::milli>(Reloj::now() - inicio).count();
    }
};

// Registro opcional de la convergencia: (tiempo en ms, mejor Cmax hasta ese momento).
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

// Desviación relativa porcentual respecto al óptimo conocido:
//   RPD = 100 · (Cmax - BKS) / BKS
inline double rpd(int cmax, int bks) {
    return bks > 0 ? 100.0 * (cmax - bks) / bks : 0.0;
}

// Temperatura constante usada en ILS-SA e IG (Osman y Potts, 1989; Ruiz y Stützle, 2007):
//   T = Tp · (suma de todos los p_ji) / (n · m · 10)
inline double temperaturaConstante(const Instancia& ins, double Tp) {
    return Tp * static_cast<double>(ins.sumaTiempos()) / (ins.n * ins.m * 10.0);
}

inline std::string permATexto(const Permutacion& pi) {
    std::ostringstream os;
    for (size_t k = 0; k < pi.size(); ++k) os << (k ? " " : "") << "J" << pi[k] + 1;
    return os.str();
}
