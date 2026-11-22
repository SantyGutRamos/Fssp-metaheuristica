# Metaheurísticas básicas y metaheurísticas iterativas para el FSSP

Grupo 6 · Análisis y Diseño de Algoritmos (ADA B) · UNSA, Escuela Profesional de Ingeniería de Sistemas · 2026

Integrantes: Gutierrez Ramos, Santiago Cristopher · Hancco Flores, Angel Gabriel · Chamana Gonsalez, Matías Hernán · Negrón Ccalluchi, Richard Antony

Implementación en C++17 (sin librerías externas) de metaheurísticas para el **problema de flow shop de permutación** (minimizar el makespan Cmax):

| Familia | Algoritmo | Clave |
| --- | --- | --- |
| Heurística constructiva | NEH (Nawaz, Enscore y Ham, 1983) | `neh` |
| Heurística de mejora | NEH + búsqueda local por reinserción | `neh_ls` |
| Metaheurística básica | Recocido simulado | `sa` |
| Metaheurística básica | Búsqueda tabú | `tabu` |
| Metaheurística iterativa | ILS con criterio Better | `ils_b` |
| Metaheurística iterativa | ILS con criterio Random Walk | `ils_rw` |
| Metaheurística iterativa | ILS con criterio de recocido simulado | `ils_sa` |
| Metaheurística iterativa | Iterated Greedy sin búsqueda local | `ig` |
| Metaheurística iterativa | Iterated Greedy con búsqueda local (Ruiz y Stützle, 2007) | `ig_ls` |

## Compilar

```bash
g++ -std=c++17 -O2 -Iinclude src/main.cpp -o fssp
g++ -std=c++17 -O2 -Iinclude tests/pruebas.cpp -o pruebas && ./pruebas
```

## Usar

```bash
# Un algoritmo en una instancia (tiempo límite = n·(m/2)·t ms)
./fssp run instances/ta041.txt ig_ls --t 15 --seed 1

# Ejemplo didáctico: NEH paso a paso + una iteración de IG
./fssp traza ejemplos/ejemplo_5x3.txt --seed 5 --d 2

# Tiempos de inicio/fin para dibujar un Gantt
./fssp gantt ejemplos/ejemplo_5x3.txt 4 3 2 5 1

# Tamaño de las vecindades
./fssp vecindades 20

# Experimento completo (todos los algoritmos, varias semillas) -> CSV
./fssp experimento --t 5 --reps 5 --out resultados/resultados_20trabajos.csv instances/ta0[0-3]*.txt
./fssp experimento --t 15 --reps 3 --out resultados/resultados_50x10.csv instances/ta04*.txt instances/ta050.txt
./fssp convergencia instances/ta041.txt --t 30 --out resultados/convergencia_ta041.csv

# Tablas y gráficos
python3 tools/graficos.py
```

## Estructura

```
include/fssp.hpp                       instancia, makespan, aceleración de Taillard
include/heuristicas.hpp                NEH, búsqueda local, vecindades
include/metaheuristicas_basicas.hpp    recocido simulado, búsqueda tabú
include/metaheuristicas_iterativas.hpp ILS (Better / Random Walk / SA), IG ± LS
src/main.cpp                           programa de línea de comandos
tests/pruebas.cpp                      pruebas de corrección
tools/generar_taillard.py              genera ta001–ta030 y ta041–ta050 desde las semillas de Taillard
tools/graficos.py                      resumen y figuras
instances/                             instancias (n m óptimo, luego una fila por trabajo)
ejemplos/                              ejemplo 5×3 y su traza
resultados/                            CSV de los experimentos
figuras/                               gráficos generados
```

## Formato de instancia

```
n m óptimo_conocido
p_11 p_12 ... p_1m     <- trabajo 1 (un tiempo por máquina)
...
p_n1 p_n2 ... p_nm
```

## Referencias

- Nawaz, M., Enscore, E. E. y Ham, I. (1983). A heuristic algorithm for the m-machine, n-job flow-shop sequencing problem. *Omega*, 11(1), 91–95.
- Taillard, E. (1990). Some efficient heuristic methods for the flow shop sequencing problem. *European Journal of Operational Research*, 47(1), 65–74.
- Taillard, E. (1993). Benchmarks for basic scheduling problems. *European Journal of Operational Research*, 64(2), 278–285.
- Kirkpatrick, S., Gelatt, C. D. y Vecchi, M. P. (1983). Optimization by simulated annealing. *Science*, 220(4598), 671–680.
- Glover, F. (1986). Future paths for integer programming and links to artificial intelligence. *Computers & Operations Research*, 13(5), 533–549.
- Lourenço, H. R., Martin, O. C. y Stützle, T. (2003). Iterated local search. En *Handbook of Metaheuristics* (pp. 320–353). Kluwer Academic Publishers.
- Ruiz, R. y Stützle, T. (2007). A simple and effective iterated greedy algorithm for the permutation flowshop scheduling problem. *European Journal of Operational Research*, 177(3), 2033–2049.
