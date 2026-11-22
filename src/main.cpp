// main.cpp
// Programa de línea de comandos para el Grupo 6 – ADA (UNSA 2026-B)
// "Metaheurísticas básicas y metaheurísticas iterativas" aplicadas al FSSP.
//
// Modos:
//   fssp run <instancia> <algoritmo> [--t 30] [--seed 1]
//   fssp experimento [--t 30] [--reps 5] [--out resultados.csv] <instancias...>
//   fssp convergencia <instancia> [--t 30] [--seed 1] [--out convergencia.csv]
//   fssp traza <instancia> [--seed 1] [--d 2]
//   fssp gantt <instancia> <J..> (orden de trabajos, numerados desde 1)
//   fssp vecindades <n>
//
// Algoritmos: neh, neh_ls, sa, tabu, ils_b, ils_rw, ils_sa, ig, ig_ls
// Tiempo límite (criterio de parada, como en Ruiz y Stützle 2007):
//   n · (m/2) · t  milisegundos
#include "fssp.hpp"
#include "heuristicas.hpp"
#include "metaheuristicas_basicas.hpp"
#include "metaheuristicas_iterativas.hpp"

#include <cstring>
#include <iomanip>
#include <map>

static const std::vector<std::string> ALGORITMOS = {
    "neh", "neh_ls", "sa", "tabu", "ils_b", "ils_rw", "ils_sa", "ig", "ig_ls"};

static const std::map<std::string, std::string> NOMBRE_LARGO = {
    {"neh", "NEH"},
    {"neh_ls", "NEH + Búsqueda local"},
    {"sa", "Recocido simulado"},
    {"tabu", "Búsqueda tabú"},
    {"ils_b", "ILS (Better)"},
    {"ils_rw", "ILS (Random Walk)"},
    {"ils_sa", "ILS (SA)"},
    {"ig", "IG - LS"},
    {"ig_ls", "IG + LS"}};

Resultado correr(const std::string& alg, const Instancia& ins, double limiteMs,
                 std::mt19937& rng, Traza* tr = nullptr) {
    if (alg == "neh") {
        Cronometro r;
        Resultado res;
        res.pi = neh(ins);
        res.cmax = makespan(ins, res.pi);
        res.ms = r.ms();
        if (tr) tr->anotar(res.ms, res.cmax);
        return res;
    }
    if (alg == "neh_ls") {
        Cronometro r;
        Resultado res;
        res.pi = neh(ins);
        res.cmax = busquedaLocal(ins, res.pi, rng);
        res.ms = r.ms();
        if (tr) tr->anotar(res.ms, res.cmax);
        return res;
    }
    if (alg == "sa") return recocidoSimulado(ins, limiteMs, rng, {}, tr);
    if (alg == "tabu") return busquedaTabu(ins, limiteMs, rng, {}, tr);
    if (alg == "ils_b") return ils(ins, limiteMs, rng, {Aceptacion::Better}, tr);
    if (alg == "ils_rw") return ils(ins, limiteMs, rng, {Aceptacion::RandomWalk}, tr);
    if (alg == "ils_sa") return ils(ins, limiteMs, rng, {Aceptacion::Recocido}, tr);
    if (alg == "ig") { ParamIG p; p.usarLS = false; return iteratedGreedy(ins, limiteMs, rng, p, tr); }
    if (alg == "ig_ls") return iteratedGreedy(ins, limiteMs, rng, {}, tr);
    throw std::runtime_error("Algoritmo desconocido: " + alg);
}

static double limiteTiempo(const Instancia& ins, double t) { return ins.n * (ins.m / 2.0) * t; }

// Lectura simple de opciones "--clave valor".
struct Opciones {
    std::map<std::string, std::string> kv;
    std::vector<std::string> pos;
    Opciones(int argc, char** argv, int desde) {
        for (int k = desde; k < argc; ++k) {
            if (std::strncmp(argv[k], "--", 2) == 0 && k + 1 < argc) {
                std::string clave = argv[k] + 2;
                kv[clave] = argv[k + 1];
                ++k;
            } else {
                pos.push_back(argv[k]);
            }
        }
    }
    double num(const std::string& k, double def) const {
        auto it = kv.find(k);
        return it == kv.end() ? def : std::stod(it->second);
    }
    std::string str(const std::string& k, const std::string& def) const {
        auto it = kv.find(k);
        return it == kv.end() ? def : it->second;
    }
};

static void imprimirTabla(const Instancia& ins, const Permutacion& pi) {
    auto C = tablaFin(ins, pi);
    std::cout << "        ";
    for (int i = 0; i < ins.m; ++i) std::cout << std::setw(6) << ("M" + std::to_string(i + 1));
    std::cout << "\n";
    for (size_t k = 0; k < pi.size(); ++k) {
        std::cout << std::setw(6) << ("J" + std::to_string(pi[k] + 1)) << "  ";
        for (int i = 0; i < ins.m; ++i) std::cout << std::setw(6) << C[k][i];
        std::cout << "\n";
    }
}

int modoRun(const Opciones& o) {
    if (o.pos.size() < 2) { std::cerr << "uso: fssp run <instancia> <algoritmo>\n"; return 1; }
    Instancia ins = leerInstancia(o.pos[0]);
    std::mt19937 rng(static_cast<unsigned>(o.num("seed", 1)));
    double lim = limiteTiempo(ins, o.num("t", 30));
    Resultado r = correr(o.pos[1], ins, lim, rng);
    std::cout << "Instancia  : " << ins.nombre << " (" << ins.n << " trabajos x " << ins.m << " máquinas)\n"
              << "Algoritmo  : " << NOMBRE_LARGO.at(o.pos[1]) << "\n"
              << "Cmax       : " << r.cmax << "\n";
    if (ins.bks > 0)
        std::cout << "Óptimo     : " << ins.bks << "   RPD = " << std::fixed << std::setprecision(2)
                  << rpd(r.cmax, ins.bks) << " %\n";
    std::cout << "Tiempo     : " << std::fixed << std::setprecision(1) << r.ms << " ms\n"
              << "Iteraciones: " << r.iteraciones << "\n"
              << "Secuencia  : " << permATexto(r.pi) << "\n";
    if (makespan(ins, r.pi) != r.cmax) { std::cerr << "ERROR: Cmax inconsistente\n"; return 2; }
    return 0;
}

int modoExperimento(const Opciones& o) {
    double t = o.num("t", 30);
    int reps = static_cast<int>(o.num("reps", 5));
    std::string out = o.str("out", "resultados.csv");
    std::string lista = o.str("algs", "");
    std::vector<std::string> algs = ALGORITMOS;
    if (!lista.empty()) {
        algs.clear();
        std::stringstream ss(lista);
        std::string a;
        while (std::getline(ss, a, ',')) algs.push_back(a);
    }
    std::ofstream f(out);
    f << "instancia,n,m,bks,algoritmo,rep,semilla,cmax,rpd,ms,iteraciones\n";
    for (const auto& ruta : o.pos) {
        Instancia ins = leerInstancia(ruta);
        double lim = limiteTiempo(ins, t);
        for (const auto& alg : algs) {
            int R = (alg == "neh") ? 1 : reps;
            for (int r = 0; r < R; ++r) {
                unsigned semilla = 1000u * (r + 1) + 7u;
                std::mt19937 rng(semilla);
                Resultado res = correr(alg, ins, lim, rng);
                if (makespan(ins, res.pi) != res.cmax) throw std::runtime_error("Cmax inconsistente");
                if (ins.bks > 0 && res.cmax < ins.bks) throw std::runtime_error("Cmax < óptimo: revisar instancia");
                f << ins.nombre << "," << ins.n << "," << ins.m << "," << ins.bks << "," << alg << ","
                  << r + 1 << "," << semilla << "," << res.cmax << "," << std::fixed << std::setprecision(4)
                  << rpd(res.cmax, ins.bks) << "," << std::setprecision(1) << res.ms << ","
                  << res.iteraciones << "\n";
                f.flush();
                std::cerr << ins.nombre << " " << std::setw(7) << alg << " rep " << r + 1 << ": Cmax "
                          << res.cmax << " (RPD " << std::setprecision(2) << rpd(res.cmax, ins.bks) << "%)\n";
            }
        }
    }
    return 0;
}

int modoConvergencia(const Opciones& o) {
    if (o.pos.empty()) { std::cerr << "uso: fssp convergencia <instancia>\n"; return 1; }
    Instancia ins = leerInstancia(o.pos[0]);
    double lim = limiteTiempo(ins, o.num("t", 30));
    unsigned seed = static_cast<unsigned>(o.num("seed", 1));
    std::ofstream f(o.str("out", "convergencia.csv"));
    f << "algoritmo,ms,cmax\n";
    for (const auto& alg : ALGORITMOS) {
        if (alg == "neh" || alg == "neh_ls") continue;
        std::mt19937 rng(seed);
        Traza tr;
        Resultado r = correr(alg, ins, lim, rng, &tr);
        for (auto [ms, c] : tr.puntos) f << alg << "," << std::fixed << std::setprecision(2) << ms << "," << c << "\n";
        f << alg << "," << r.ms << "," << r.cmax << "\n";   // punto final
        std::cerr << alg << ": " << r.cmax << "\n";
    }
    return 0;
}

// Traza didáctica: NEH paso a paso + una iteración completa de IG (destrucción y
// construcción con el costo de cada posición) + búsqueda local.
int modoTraza(const Opciones& o) {
    if (o.pos.empty()) { std::cerr << "uso: fssp traza <instancia>\n"; return 1; }
    Instancia ins = leerInstancia(o.pos[0]);
    std::mt19937 rng(static_cast<unsigned>(o.num("seed", 1)));
    int d = static_cast<int>(o.num("d", 2));

    std::cout << "=== Datos (p_ji) ===\n";
    for (int j = 0; j < ins.n; ++j) {
        long long s = 0;
        std::cout << "J" << j + 1 << ":";
        for (int i = 0; i < ins.m; ++i) { std::cout << " " << std::setw(3) << ins.p[j][i]; s += ins.p[j][i]; }
        std::cout << "   total = " << s << "\n";
    }

    std::cout << "\n=== NEH paso a paso ===\n";
    Permutacion orden = ordenNEH(ins);
    std::cout << "Orden por tiempo total decreciente: " << permATexto(orden) << "\n";
    Permutacion pi;
    for (int job : orden) {
        auto costos = evaluarInserciones(ins, pi, job);
        std::cout << "Insertar J" << job + 1 << " en (" << permATexto(pi) << "): ";
        for (size_t k = 0; k < costos.size(); ++k) std::cout << "pos" << k + 1 << "=" << costos[k] << " ";
        auto [pos, c] = mejorInsercion(ins, pi, job);
        pi.insert(pi.begin() + pos, job);
        std::cout << "-> " << permATexto(pi) << " (Cmax " << c << ")\n";
    }
    int cNEH = makespan(ins, pi);
    std::cout << "NEH final: " << permATexto(pi) << "  Cmax = " << cNEH << "\n";
    imprimirTabla(ins, pi);

    std::cout << "\n=== Una iteración de Iterated Greedy (d = " << d << ") ===\n";
    Permutacion parcial = pi, removidos;
    for (int k = 0; k < d; ++k) {
        std::uniform_int_distribution<int> U(0, static_cast<int>(parcial.size()) - 1);
        int idx = U(rng);
        removidos.push_back(parcial[idx]);
        parcial.erase(parcial.begin() + idx);
    }
    std::cout << "DESTRUCCIÓN: se sacan " << permATexto(removidos) << " -> parcial = ("
              << permATexto(parcial) << "), Cmax parcial = " << makespan(ins, parcial) << "\n";
    for (int job : removidos) {
        auto costos = evaluarInserciones(ins, parcial, job);
        std::cout << "CONSTRUCCIÓN: insertar J" << job + 1 << ": ";
        for (size_t k = 0; k < costos.size(); ++k) std::cout << "pos" << k + 1 << "=" << costos[k] << " ";
        auto [pos, c] = mejorInsercion(ins, parcial, job);
        parcial.insert(parcial.begin() + pos, job);
        std::cout << "-> " << permATexto(parcial) << " (Cmax " << c << ")\n";
    }
    int cIG = makespan(ins, parcial);
    Permutacion conLS = parcial;
    int cLS = busquedaLocal(ins, conLS, rng, cIG);
    std::cout << "Tras construcción: Cmax = " << cIG << "\n";
    std::cout << "Tras búsqueda local: " << permATexto(conLS) << "  Cmax = " << cLS << "\n";
    double T = temperaturaConstante(ins, 0.4);
    std::cout << "Aceptación: actual = " << cNEH << ", nueva = " << cLS << ", T = " << std::fixed
              << std::setprecision(3) << T;
    if (cLS < cNEH) std::cout << " -> MEJORA, se acepta.\n";
    else std::cout << " -> no mejora; se aceptaría con prob. e^(-" << (cLS - cNEH) << "/T) = "
                   << std::exp(-(cLS - cNEH) / T) << "\n";
    imprimirTabla(ins, conLS);
    return 0;
}

int modoGantt(const Opciones& o) {
    if (o.pos.size() < 2) { std::cerr << "uso: fssp gantt <instancia> <J1 J2 ...>\n"; return 1; }
    Instancia ins = leerInstancia(o.pos[0]);
    Permutacion pi;
    for (size_t k = 1; k < o.pos.size(); ++k) {
        std::string s = o.pos[k];
        if (!s.empty() && (s[0] == 'J' || s[0] == 'j')) s = s.substr(1);
        pi.push_back(std::stoi(s) - 1);
    }
    auto C = tablaFin(ins, pi);
    std::cout << "trabajo,maquina,inicio,fin\n";
    for (size_t k = 0; k < pi.size(); ++k)
        for (int i = 0; i < ins.m; ++i)
            std::cout << pi[k] + 1 << "," << i + 1 << "," << C[k][i] - ins.p[pi[k]][i] << "," << C[k][i] << "\n";
    return 0;
}

int main(int argc, char** argv) {
    std::ios::sync_with_stdio(false);
    if (argc < 2) {
        std::cerr << "Modos: run | experimento | convergencia | traza | gantt | vecindades\n"
                     "Algoritmos: neh neh_ls sa tabu ils_b ils_rw ils_sa ig ig_ls\n";
        return 1;
    }
    std::string modo = argv[1];
    Opciones o(argc, argv, 2);
    try {
        if (modo == "run") return modoRun(o);
        if (modo == "experimento") return modoExperimento(o);
        if (modo == "convergencia") return modoConvergencia(o);
        if (modo == "traza") return modoTraza(o);
        if (modo == "gantt") return modoGantt(o);
        if (modo == "vecindades") {
            long long n = o.pos.empty() ? 20 : std::stoll(o.pos[0]);
            std::cout << "n = " << n << "\n  intercambio: " << tamVecindadIntercambio(n)
                      << " vecinos\n  reinsercion: " << tamVecindadReinsercion(n) << " vecinos\n";
            return 0;
        }
    } catch (const std::exception& e) {
        std::cerr << "Error: " << e.what() << "\n";
        return 1;
    }
    std::cerr << "Modo desconocido: " << modo << "\n";
    return 1;
}
