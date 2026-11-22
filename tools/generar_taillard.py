"""Genera las instancias de Taillard (1993) para el flow shop de permutación.

Taillard publicó el generador y las semillas, así que las instancias se pueden
reconstruir exactamente. Aquí se generan ta001-ta030 (20 trabajos con 5, 10 y 20
máquinas) y ta041-ta050 (50 trabajos y 10 máquinas) junto con su óptimo conocido (BKS).

Formato de salida (lo lee leerInstancia en fssp.hpp):
    n m bks
    p_11 p_12 ... p_1m      <- trabajo 1
    ...
    p_n1 p_n2 ... p_nm      <- trabajo n

Uso: python3 tools/generar_taillard.py instances/
"""
import os
import sys

# (nombre, n, m, semilla, óptimo conocido)
INSTANCIAS = [
    ("ta001", 20, 5, 873654221, 1278), ("ta002", 20, 5, 379008056, 1359),
    ("ta003", 20, 5, 1866992158, 1081), ("ta004", 20, 5, 216771124, 1293),
    ("ta005", 20, 5, 495070989, 1235), ("ta006", 20, 5, 402959317, 1195),
    ("ta007", 20, 5, 1369363414, 1234), ("ta008", 20, 5, 2021925980, 1206),
    ("ta009", 20, 5, 573109518, 1230), ("ta010", 20, 5, 88325120, 1108),
    ("ta011", 20, 10, 587595453, 1582), ("ta012", 20, 10, 1401007982, 1659),
    ("ta013", 20, 10, 873136276, 1496), ("ta014", 20, 10, 268827376, 1377),
    ("ta015", 20, 10, 1634173168, 1419), ("ta016", 20, 10, 691823909, 1397),
    ("ta017", 20, 10, 73807235, 1484), ("ta018", 20, 10, 1273398721, 1538),
    ("ta019", 20, 10, 2065119309, 1593), ("ta020", 20, 10, 1672900551, 1591),
    ("ta021", 20, 20, 479340445, 2297), ("ta022", 20, 20, 268827376, 2099),
    ("ta023", 20, 20, 1958948863, 2326), ("ta024", 20, 20, 918272953, 2223),
    ("ta025", 20, 20, 555010963, 2291), ("ta026", 20, 20, 2010851491, 2226),
    ("ta027", 20, 20, 1519833303, 2273), ("ta028", 20, 20, 1748670931, 2200),
    ("ta029", 20, 20, 1923497586, 2237), ("ta030", 20, 20, 1829909967, 2178),
    ("ta041", 50, 10, 1958948863, 2991), ("ta042", 50, 10, 575633267, 2867),
    ("ta043", 50, 10, 655816003, 2839), ("ta044", 50, 10, 1977864101, 3063),
    ("ta045", 50, 10, 93805469, 2976), ("ta046", 50, 10, 1803345551, 3006),
    ("ta047", 50, 10, 49612559, 3093), ("ta048", 50, 10, 1899802599, 3037),
    ("ta049", 50, 10, 2013025619, 2897), ("ta050", 50, 10, 578962478, 3065),
]


def unif(semilla, bajo, alto):
    """Generador congruencial lineal de Taillard (Park-Miller, a = 16807)."""
    m, a, b, c = 2147483647, 16807, 127773, 2836
    k = semilla // b
    semilla = a * (semilla % b) - k * c
    if semilla < 0:
        semilla += m
    return semilla, bajo + int(semilla / m * (alto - bajo + 1))


def generar(n, m, semilla):
    p = [[0] * m for _ in range(n)]
    for i in range(m):          # Taillard genera máquina por máquina
        for j in range(n):
            semilla, p[j][i] = unif(semilla, 1, 99)
    return p


def main():
    destino = sys.argv[1] if len(sys.argv) > 1 else "instances"
    os.makedirs(destino, exist_ok=True)
    for nombre, n, m, semilla, bks in INSTANCIAS:
        p = generar(n, m, semilla)
        with open(os.path.join(destino, nombre + ".txt"), "w") as f:
            f.write(f"{n} {m} {bks}\n")
            for fila in p:
                f.write(" ".join(f"{x:2d}" for x in fila) + "\n")
    print(f"{len(INSTANCIAS)} instancias escritas en {destino}/")


if __name__ == "__main__":
    main()
