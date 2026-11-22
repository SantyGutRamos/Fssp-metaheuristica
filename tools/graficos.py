"""Resume los resultados (CSV) y genera los gráficos del informe y las diapositivas.

Uso:
    python3 tools/graficos.py
Lee   resultados/resultados_20trabajos.csv, resultados/resultados_50x10.csv,
      resultados/convergencia_ta041.csv
Escribe resultados/resumen.csv, resultados/resumen.md y figuras/*.png
"""
import csv
import os
from collections import defaultdict

import matplotlib

matplotlib.use("Agg")
import matplotlib.pyplot as plt  # noqa: E402

RAIZ = os.path.dirname(os.path.dirname(os.path.abspath(__file__)))
RES = os.path.join(RAIZ, "resultados")
FIG = os.path.join(RAIZ, "figuras")
os.makedirs(FIG, exist_ok=True)

ORDEN = ["neh", "neh_ls", "sa", "tabu", "ils_b", "ils_rw", "ils_sa", "ig", "ig_ls"]
NOMBRE = {
    "neh": "NEH", "neh_ls": "NEH + BL", "sa": "Recocido simulado", "tabu": "Búsqueda tabú",
    "ils_b": "ILS (Better)", "ils_rw": "ILS (Random Walk)", "ils_sa": "ILS (SA)",
    "ig": "IG − LS", "ig_ls": "IG + LS",
}
GRIS, ACENTO = "#9aa0a6", "#1a73e8"


def leer(ruta):
    with open(ruta) as f:
        return list(csv.DictReader(f))


def resumir(filas):
    """RPD promedio por (grupo n×m, algoritmo) y por algoritmo en total."""
    por_grupo = defaultdict(list)
    for r in filas:
        grupo = f"{r['n']}x{r['m']}"
        por_grupo[(grupo, r["algoritmo"])].append(float(r["rpd"]))
    grupos = sorted({g for g, _ in por_grupo}, key=lambda g: tuple(map(int, g.split("x"))))
    tabla = {}
    for g in grupos:
        for a in ORDEN:
            v = por_grupo.get((g, a))
            if v:
                tabla[(g, a)] = sum(v) / len(v)
    return grupos, tabla


def coma(v, d=2):
    return f"{v:.{d}f}".replace(".", ",")


def main(fondo="white", sufijo="", escala=1.0):
    plt.rcParams.update({"font.size": 10 * escala, "axes.titlesize": 11 * escala,
                         "axes.labelsize": 10 * escala, "xtick.labelsize": 9.5 * escala,
                         "ytick.labelsize": 9.5 * escala, "legend.fontsize": 8.5 * escala})
    filas = []
    for nombre in ("resultados_20trabajos.csv", "resultados_50x10.csv"):
        ruta = os.path.join(RES, nombre)
        if os.path.exists(ruta):
            filas += leer(ruta)
    grupos, tabla = resumir(filas)

    # Óptimos alcanzados (cuántas ejecuciones llegaron exactamente al óptimo)
    hits = defaultdict(lambda: [0, 0])
    for r in filas:
        h = hits[(f"{r['n']}x{r['m']}", r["algoritmo"])]
        h[1] += 1
        h[0] += int(r["cmax"]) == int(r["bks"])

    with open(os.path.join(RES, "resumen.csv"), "w", newline="") as f:
        w = csv.writer(f)
        w.writerow(["grupo", "algoritmo", "rpd_promedio", "optimos", "ejecuciones"])
        for g in grupos:
            for a in ORDEN:
                if (g, a) in tabla:
                    w.writerow([g, a, f"{tabla[(g, a)]:.3f}", hits[(g, a)][0], hits[(g, a)][1]])

    with open(os.path.join(RES, "resumen.md"), "w") as f:
        f.write("| Algoritmo | " + " | ".join(f"RPD {g} (%)" for g in grupos) + " |\n")
        f.write("| --- | " + " | ".join("---" for _ in grupos) + " |\n")
        for a in ORDEN:
            f.write(f"| {NOMBRE[a]} | " + " | ".join(
                f"{tabla[(g, a)]:.2f}" if (g, a) in tabla else "" for g in grupos) + " |\n")

    print(open(os.path.join(RES, "resumen.md")).read())

    # Figura 1: RPD promedio en 50x10 (barras horizontales; acento en el mejor)
    g = "50x10"
    algs = [a for a in ORDEN if (g, a) in tabla]
    if algs:
        vals = [tabla[(g, a)] for a in algs]
        mejor = min(vals)
        fig, ax = plt.subplots(figsize=(8, 4.2), dpi=200, facecolor=fondo)
        ax.set_facecolor(fondo)
        colores = [ACENTO if v == mejor else GRIS for v in vals]
        y = range(len(algs))[::-1]
        ax.barh(list(y), vals, color=colores)
        for yi, v in zip(y, vals):
            ax.text(v + 0.05, yi, f"{coma(v)} %", va="center", fontsize=9 * escala)
        ax.set_yticks(list(y))
        ax.set_yticklabels([NOMBRE[a] for a in algs])
        ax.set_xlabel("Desviación promedio respecto al óptimo (RPD, %)")
        ax.set_title(f"Taillard 50×10: IG + LS queda a {coma(mejor)} % del óptimo; NEH, a {coma(tabla[(g, 'neh')])} %",
                     loc="left")
        ax.spines[["top", "right"]].set_visible(False)
        fig.tight_layout()
        fig.savefig(os.path.join(FIG, f"rpd_50x10{sufijo}.png"), facecolor=fondo)
        plt.close(fig)

    # Figura 2: convergencia en ta041
    ruta = os.path.join(RES, "convergencia_ta041.csv")
    if os.path.exists(ruta):
        conv = defaultdict(list)
        for r in leer(ruta):
            conv[r["algoritmo"]].append((float(r["ms"]) / 1000, int(r["cmax"])))
        fig, ax = plt.subplots(figsize=(8, 4.2), dpi=200, facecolor=fondo)
        ax.set_facecolor(fondo)
        for a in ["sa", "tabu", "ils_b", "ils_rw", "ils_sa", "ig", "ig_ls"]:
            if a not in conv:
                continue
            pts = sorted(conv[a])
            xs, ys = [], []
            for t, c in pts:
                if xs:
                    xs.append(t)
                    ys.append(ys[-1])
                xs.append(t)
                ys.append(c)
            destacado = a == "ig_ls"
            ax.plot(xs, ys, lw=2.2 if destacado else 1.2,
                    color=ACENTO if destacado else None, alpha=1 if destacado else 0.75,
                    label=NOMBRE[a])
        ax.axhline(2991, color="black", lw=0.8, ls="--")
        ax.text(0.02, 2993, "óptimo 2991", fontsize=8 * escala, transform=ax.get_yaxis_transform())
        ax.set_xlabel("Tiempo (s)")
        ax.set_ylabel("Mejor Cmax encontrado")
        ax.set_title("ta041 (50×10): IG + LS termina en 3023, el mejor; Random Walk se estanca en 3037", loc="left")
        ax.legend(ncol=2, frameon=False)
        ax.spines[["top", "right"]].set_visible(False)
        fig.tight_layout()
        fig.savefig(os.path.join(FIG, f"convergencia_ta041{sufijo}.png"), facecolor=fondo)
        plt.close(fig)


def gantt_ejemplo():
    """Diagramas de Gantt del ejemplo 5x3: NEH (45) frente a una iteración de IG (43)."""
    import subprocess

    exe = os.path.join(RAIZ, "fssp")
    inst = os.path.join(RAIZ, "ejemplos", "ejemplo_5x3.txt")
    if not os.path.exists(exe):
        return
    casos = [("NEH: J2 J4 J3 J5 J1  →  Cmax = 45", ["2", "4", "3", "5", "1"]),
             ("IG (1 iteración): J4 J3 J2 J5 J1  →  Cmax = 43 (óptimo)", ["4", "3", "2", "5", "1"])]
    colores = ["#4e79a7", "#f28e2b", "#59a14f", "#e15759", "#76b7b2"]
    fig, axs = plt.subplots(2, 1, figsize=(8, 4.6), dpi=200, sharex=True)
    for ax, (titulo, perm) in zip(axs, casos):
        salida = subprocess.run([exe, "gantt", inst, *perm], capture_output=True, text=True).stdout
        filas = list(csv.DictReader(salida.strip().splitlines()))
        for r in filas:
            j, mq, ini, fin = int(r["trabajo"]), int(r["maquina"]), int(r["inicio"]), int(r["fin"])
            ax.barh(4 - mq, fin - ini, left=ini, color=colores[j - 1], edgecolor="white")
            ax.text((ini + fin) / 2, 4 - mq, f"J{j}", ha="center", va="center", color="white", fontsize=8)
        cmax = max(int(r["fin"]) for r in filas)
        ax.axvline(cmax, color="black", lw=0.8, ls="--")
        ax.set_yticks([3, 2, 1])
        ax.set_yticklabels(["M1", "M2", "M3"])
        ax.set_title(titulo, fontsize=10, loc="left")
        ax.spines[["top", "right"]].set_visible(False)
    axs[-1].set_xlabel("Tiempo")
    fig.tight_layout()
    fig.savefig(os.path.join(FIG, "gantt_ejemplo.png"))
    plt.close(fig)


def gantt_individual(nombre, perm, fondo="#F6F4EF"):
    """Un Gantt grande por imagen (para las diapositivas)."""
    import subprocess

    exe = os.path.join(RAIZ, "fssp")
    inst = os.path.join(RAIZ, "ejemplos", "ejemplo_5x3.txt")
    colores = ["#4e79a7", "#f28e2b", "#59a14f", "#e15759", "#76b7b2"]
    salida = subprocess.run([exe, "gantt", inst, *perm], capture_output=True, text=True).stdout
    filas = list(csv.DictReader(salida.strip().splitlines()))
    fig, ax = plt.subplots(figsize=(14, 4.2), dpi=150)
    fig.patch.set_facecolor(fondo)
    ax.set_facecolor(fondo)
    for r in filas:
        j, mq, ini, fin = int(r["trabajo"]), int(r["maquina"]), int(r["inicio"]), int(r["fin"])
        ax.barh(4 - mq, fin - ini, left=ini, height=0.72, color=colores[j - 1], edgecolor=fondo, lw=2)
        if fin - ini >= 2:
            ax.text((ini + fin) / 2, 4 - mq, f"J{j}", ha="center", va="center", color="white",
                    fontsize=17, fontweight="bold")
    cmax = max(int(r["fin"]) for r in filas)
    ax.axvline(cmax, color="#16213A", lw=1.5, ls="--")
    ax.text(cmax + 0.4, 3.35, f"Cmax = {cmax}", fontsize=18, color="#16213A", fontweight="bold")
    ax.set_xlim(0, 50)
    ax.set_yticks([3, 2, 1])
    ax.set_yticklabels(["M1", "M2", "M3"], fontsize=17)
    ax.tick_params(axis="x", labelsize=15)
    ax.set_xlabel("Tiempo", fontsize=15)
    ax.spines[["top", "right"]].set_visible(False)
    fig.tight_layout()
    fig.savefig(os.path.join(FIG, nombre), facecolor=fondo)
    plt.close(fig)


if __name__ == "__main__":
    main()
    main(fondo="#F6F4EF", sufijo="_diapo", escala=1.0)
    gantt_ejemplo()
    gantt_individual("gantt_neh.png", ["2", "4", "3", "5", "1"])
    gantt_individual("gantt_ig.png", ["4", "3", "2", "5", "1"])
