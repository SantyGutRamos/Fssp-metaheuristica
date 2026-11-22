// pruebas.cpp — verificaciones básicas de corrección.
//   1) La aceleración de Taillard da el mismo Cmax que recalcular desde cero.
//   2) NEH reproduce los valores publicados para ta001-ta010.
//   3) Ningún algoritmo devuelve una permutación inválida ni un Cmax menor al óptimo.
// Compilar: g++ -std=c++17 -O2 -Iinclude tests/pruebas.cpp -o pruebas && ./pruebas
#include "fssp.hpp"
#include "heuristicas.hpp"
#include "metaheuristicas_basicas.hpp"
#include "metaheuristicas_iterativas.hpp"
#include <cassert>

static bool esPermutacion(const Permutacion& pi, int n) {
    std::vector<int> v(pi);
    std::sort(v.begin(), v.end());
    for (int k = 0; k < n; ++k) if (v[k] != k) return false;
    return static_cast<int>(v.size()) == n;
}

int main() {
    std::mt19937 rng(42);
    int fallos = 0;

    // 1) Taillard vs fuerza bruta, en instancias aleatorias pequeñas.
    for (int caso = 0; caso < 500; ++caso) {
        Instancia ins;
        ins.n = 2 + rng() % 9;
        ins.m = 1 + rng() % 6;
        ins.p.assign(ins.n, std::vector<int>(ins.m));
        for (auto& f : ins.p) for (auto& x : f) x = 1 + rng() % 99;
        Permutacion seq(ins.n - 1);
        std::iota(seq.begin(), seq.end(), 0);
        std::shuffle(seq.begin(), seq.end(), rng);
        int job = ins.n - 1;
        auto rapido = evaluarInserciones(ins, seq, job);
        for (size_t pos = 0; pos <= seq.size(); ++pos) {
            Permutacion s = seq;
            s.insert(s.begin() + pos, job);
            if (makespan(ins, s) != rapido[pos]) ++fallos;
        }
    }
    std::cout << "[1] Aceleración de Taillard: " << (fallos ? "FALLA" : "OK") << "\n";

    // 2) NEH en ta001-ta010 (valores de la literatura).
    const int nehEsperado[10] = {1286, 1365, 1159, 1325, 1305, 1228, 1278, 1223, 1291, 1151};
    int fallosNEH = 0;
    for (int k = 1; k <= 10; ++k) {
        char ruta[64];
        std::snprintf(ruta, sizeof ruta, "instances/ta%03d.txt", k);
        Instancia ins = leerInstancia(ruta);
        if (makespan(ins, neh(ins)) != nehEsperado[k - 1]) ++fallosNEH;
    }
    std::cout << "[2] NEH en ta001-ta010: " << (fallosNEH ? "FALLA" : "OK") << "\n";

    // 3) Validez de las soluciones de cada metaheurística (100 ms cada una).
    Instancia ins = leerInstancia("instances/ta011.txt");
    int fallosAlg = 0;
    std::vector<Resultado> rs = {
        recocidoSimulado(ins, 100, rng), busquedaTabu(ins, 100, rng),
        ils(ins, 100, rng, {Aceptacion::Better}), ils(ins, 100, rng, {Aceptacion::RandomWalk}),
        ils(ins, 100, rng, {Aceptacion::Recocido}), iteratedGreedy(ins, 100, rng, {4, 0.4, false}),
        iteratedGreedy(ins, 100, rng, {4, 0.4, true})};
    for (const auto& r : rs)
        if (!esPermutacion(r.pi, ins.n) || makespan(ins, r.pi) != r.cmax || r.cmax < ins.bks) ++fallosAlg;
    std::cout << "[3] Soluciones válidas: " << (fallosAlg ? "FALLA" : "OK") << "\n";

    return (fallos || fallosNEH || fallosAlg) ? 1 : 0;
}
