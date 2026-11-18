#!/usr/bin/env python3
"""Lê os CSV de uma bateria (scripts/bateria.sh) e monta as tabelas e os gráficos do artigo.

Uso, da raiz do projeto:
    python3 scripts/graficos.py resultados/bateria_20261118_120000
    python3 scripts/graficos.py resultados/bateria_X --formatos png,pdf --tolerancia 0.005

Precisa de pandas e matplotlib (pip install pandas matplotlib).

Entrada: a pasta da bateria. O script procura, em qualquer subpasta, os arquivos que o
executável `experimento` grava por rodada:
    <jogo>/<rotulo>/semente_<n>.resumo.csv   uma linha com o resultado da rodada
    <jogo>/<rotulo>/semente_<n>.csv          o histórico, uma linha por geração
Funciona também com uma rodada avulsa (./experimento ... --saida PASTA).

Saída (padrão: <pasta>/graficos/):
    agregado_geracoes.csv  média (__med) e desvio padrão (__dp) entre sementes, por geração
    agregado_final.csv     o mesmo para o resultado final de cada rodada (campeão reavaliado)
    tabela_final.md        a tabela principal, pronta para copiar para o texto
    menores_arvores.txt    a menor árvore que mantém o desempenho, por jogo
    fitness_por_geracao, bloat_por_geracao, diversidade_por_geracao,
    final_por_limite, desempenho_vs_tamanho, winrate_por_adversario, sorte_vs_reavaliado
                           os gráficos (.png e/ou .pdf); Pig e Blackjack sempre lado a lado

Uma "configuração" é (jogo, limite de profundidade, partidas por adversário). As médias
são sempre entre as sementes de uma mesma configuração.
"""

import argparse
import math
import sys
from pathlib import Path

try:
    import numpy as np
    import pandas as pd
    import matplotlib

    matplotlib.use("Agg")  # só grava arquivos, não abre janela
    import matplotlib.pyplot as plt
    import matplotlib.ticker
    from matplotlib.lines import Line2D
except ImportError as erro:
    sys.exit(f"falta uma biblioteca: {erro.name}. Instale com: pip install pandas matplotlib")


# ---------------------------------------------------------------- aparência
# Cores conferidas com o validador de paletas (daltonismo e contraste):
#  - limites de profundidade são ORDENADOS: um tom de azul, do claro (limite pequeno)
#    ao escuro (limite grande). A forma do marcador repete a informação, para quem
#    imprime em preto e branco ou não distingue bem os tons.
#  - adversários e "total × efetivos" são CATEGORIAS: azul, laranja, verde-água.
RAMPA_AZUL = ["#86b6ef", "#6da7ec", "#5598e7", "#3987e5", "#2a78d6",
              "#256abf", "#1c5cab", "#184f95", "#104281", "#0d366b"]
CATEGORICAS = ["#2a78d6", "#eb6834", "#1baf7a"]
MARCADORES = ["o", "s", "^", "D", "v", "P", "X", "*"]
TINTA = "#0b0b0b"
TINTA_2 = "#52514e"
APAGADA = "#898781"
GRADE = "#e1e0d9"
EIXO = "#c3c2b7"

NOMES_JOGOS = {"pig": "Pig", "blackjack": "Blackjack de dados"}
NOMES_ADVERSARIOS = {"aleatorio": "aleatório"}  # no json não há acento; no gráfico, sim
ORDEM_JOGOS = ["pig", "blackjack"]

# colunas do histórico que entram na agregação por geração
METRICAS_GERACAO = ["melhor_minimo", "melhor_media", "media_minimo", "media_media", "melhor_nos",
                    "melhor_profundidade", "nos_medio", "nos_max", "profundidade_media",
                    "profundidade_max", "distintas", "rejeitadas", "segundos"]
# colunas do resumo que entram na agregação final
METRICAS_FINAIS = ["geracoes", "rejeitadas", "campeao_minimo", "campeao_media", "campeao_minimo_na_evolucao",
                   "sorte", "campeao_nos", "campeao_nos_efetivos", "fracao_efetiva", "campeao_profundidade",
                   "nos_medios_inicio", "nos_medios_fim", "nos_efetivos_medios_fim", "distintas_fim", "segundos"]
CHAVE = ["jogo", "limite", "partidas"]


def estilo():
    plt.rcParams.update({
        "figure.facecolor": "white", "axes.facecolor": "white", "savefig.facecolor": "white",
        "font.size": 10, "axes.titlesize": 11, "axes.labelsize": 10,
        "text.color": TINTA, "axes.labelcolor": TINTA_2, "axes.edgecolor": EIXO, "axes.linewidth": 0.8,
        "xtick.color": APAGADA, "ytick.color": APAGADA, "xtick.labelcolor": TINTA_2, "ytick.labelcolor": TINTA_2,
        "axes.grid": True, "grid.color": GRADE, "grid.linewidth": 0.6, "axes.axisbelow": True,
        "axes.spines.top": False, "axes.spines.right": False,
        "legend.frameon": False, "lines.linewidth": 1.6, "savefig.dpi": 200,
    })


def cor_limite(i, total):
    """i-ésimo limite (em ordem crescente) de `total`: espalha os tons pela rampa."""
    if total == 1:
        return RAMPA_AZUL[6]
    return RAMPA_AZUL[round(i * (len(RAMPA_AZUL) - 1) / (total - 1))]


def nome_jogo(jogo):
    return NOMES_JOGOS.get(jogo, jogo)


def nome_adversario(adv):
    return NOMES_ADVERSARIOS.get(adv, adv)


def virgula(texto):
    return texto.replace(".", ",")  # decimais em português


def porcentagem(eixo, qual="y"):
    """Rótulos em %, com as casas decimais que o espaçamento das marcas pede:
    marcas de 1 em 1 ponto -> "52%"; de 0,5 em 0,5 -> "51,5%" (sem "51%, 51%, 51%")."""
    ax = eixo.yaxis if qual == "y" else eixo.xaxis

    def formatar(v, _):
        marcas = ax.get_majorticklocs()
        passo = abs(marcas[1] - marcas[0]) * 100 if len(marcas) > 1 else 1.0
        casas = 0  # as casas que o passo precisa: 1 -> 0 casas, 0,5 -> 1, 0,25 -> 2
        while casas < 3 and abs(round(passo, casas) - passo) > 1e-9:
            casas += 1
        return virgula(f"{100 * v:.{casas}f}%")

    ax.set_major_formatter(matplotlib.ticker.FuncFormatter(formatar))


def linha_empate(eixo):
    eixo.axhline(0.5, color=APAGADA, linewidth=0.9, linestyle=(0, (4, 3)), zorder=1)


# ---------------------------------------------------------------- leitura
def ler_resumos(pasta: Path) -> pd.DataFrame:
    """Junta todos os semente_<n>.resumo.csv (uma linha por rodada)."""
    arquivos = sorted(pasta.rglob("*.resumo.csv"))
    if not arquivos:
        sys.exit(f"nenhum *.resumo.csv em {pasta}: é a pasta de uma bateria?")
    partes = []
    for arq in arquivos:
        df = pd.read_csv(arq)  # a árvore vem entre aspas; o leitor de csv cuida disso
        df["historico"] = str(arq).removesuffix(".resumo.csv") + ".csv"
        partes.append(df)
    r = pd.concat(partes, ignore_index=True)
    r = r.rename(columns={"limite_profundidade": "limite"})

    # "aleatorio=0.88;limiar=0.52" -> colunas w_aleatorio, w_limiar
    for i, texto in r["campeao_winrates"].items():
        for par in str(texto).split(";"):
            adv, valor = par.split("=")
            r.loc[i, "w_" + adv] = float(valor)

    r["sorte"] = r["campeao_minimo_na_evolucao"] - r["campeao_minimo"]  # quanto o número da evolução inflou
    r["fracao_efetiva"] = r["campeao_nos_efetivos"] / r["campeao_nos"]   # parte da árvore que é usada
    return r


def ler_historicos(resumos: pd.DataFrame) -> pd.DataFrame:
    """Lê o histórico de cada rodada e marca de qual configuração e semente ele é."""
    partes = []
    for _, linha in resumos.iterrows():
        caminho = Path(linha["historico"])
        if not caminho.exists():
            print(f"aviso: falta o histórico {caminho}", file=sys.stderr)
            continue
        h = pd.read_csv(caminho)
        for col in CHAVE + ["semente"]:
            h[col] = linha[col]
        partes.append(h)
    return pd.concat(partes, ignore_index=True)


# ---------------------------------------------------------------- agregação
def agregar(df: pd.DataFrame, chave, metricas) -> pd.DataFrame:
    """Média (__med), desvio padrão (__dp) e número de sementes (n) de cada métrica.
    Com uma semente só, o desvio fica vazio (não dá para estimar)."""
    metricas = [m for m in metricas if m in df.columns]
    g = df.groupby(chave)[metricas]
    med = g.mean().add_suffix("__med")
    dp = g.std(ddof=1).add_suffix("__dp")
    n = g.size().rename("n")
    tabela = pd.concat([n, med, dp], axis=1).reset_index()
    ordem = ["n"] + [c for m in metricas for c in (m + "__med", m + "__dp")]
    return tabela[chave + ordem]


def jogos_presentes(df):
    presentes = list(df["jogo"].unique())
    return [j for j in ORDEM_JOGOS if j in presentes] + sorted(j for j in presentes if j not in ORDEM_JOGOS)


def configuracoes(df, jogo):
    """Configurações de um jogo, em ordem crescente de limite: [(limite, partidas, rótulo)]."""
    cfg = df[df["jogo"] == jogo][["limite", "partidas"]].drop_duplicates().sort_values(["limite", "partidas"])
    varias_partidas = df["partidas"].nunique() > 1
    return [(int(l), int(p), f"limite {l}" + (f" · {p} partidas" if varias_partidas else ""))
            for l, p in cfg.itertuples(index=False)]


def todos_limites(df):
    return sorted(int(l) for l in df["limite"].unique())


# ---------------------------------------------------------------- tabelas
def fmt_pct(m, dp):
    if m is None or pd.isna(m):
        return "—"
    return virgula(f"{100 * m:.1f}" + ("" if dp is None or pd.isna(dp) else f" ± {100 * dp:.1f}"))


def fmt_num(m, dp, casas=1):
    if m is None or pd.isna(m):
        return "—"
    return virgula(f"{m:.{casas}f}" + ("" if dp is None or pd.isna(dp) else f" ± {dp:.{casas}f}"))


def tabela_markdown(final: pd.DataFrame, adversarios) -> str:
    cab = (["jogo", "limite", "n", "campeão: pior winrate (%)", "na evolução (%)"]
           + [f"vs {nome_adversario(a)} (%)" for a in adversarios]
           + ["nós do campeão", "nós efetivos", "efetivos / nós", "nós da população (fim)", "segundos"])
    linhas = ["| " + " | ".join(cab) + " |", "|" + "---|" * len(cab)]
    for jogo in jogos_presentes(final):
        for _, r in final[final["jogo"] == jogo].sort_values(["limite", "partidas"]).iterrows():
            celulas = [nome_jogo(jogo), str(int(r["limite"])), str(int(r["n"])),
                       fmt_pct(r["campeao_minimo__med"], r["campeao_minimo__dp"]),
                       fmt_pct(r["campeao_minimo_na_evolucao__med"], r["campeao_minimo_na_evolucao__dp"])]
            for a in adversarios:
                celulas.append(fmt_pct(r.get(f"w_{a}__med"), r.get(f"w_{a}__dp")))
            celulas += [fmt_num(r["campeao_nos__med"], r["campeao_nos__dp"]),
                        fmt_num(r["campeao_nos_efetivos__med"], r["campeao_nos_efetivos__dp"]),
                        fmt_pct(r["fracao_efetiva__med"], r["fracao_efetiva__dp"]),
                        fmt_num(r["nos_medios_fim__med"], r["nos_medios_fim__dp"]),
                        fmt_num(r["segundos__med"], r["segundos__dp"])]
            linhas.append("| " + " | ".join(celulas) + " |")
    nota = ("\nValores: média ± desvio padrão entre as sementes (n). \"Campeão\" é o winrate reavaliado com "
            "partidas novas; \"na evolução\" é o número que a mesma árvore tinha durante a evolução "
            "(inflado pela sorte de ter sido a melhor). Nós efetivos: nós visitados em decisões reais.\n")
    return "\n".join(linhas) + "\n" + nota


def menores_arvores(resumos: pd.DataFrame, final: pd.DataFrame, tolerancia: float) -> str:
    """Por jogo: a referência é a melhor média entre as configurações; entre os campeões que
    ficam a no máximo `tolerancia` dela, vale o de menos nós (empate: o de maior winrate)."""
    saida = []
    for jogo in jogos_presentes(resumos):
        f = final[final["jogo"] == jogo]
        ref_linha = f.loc[f["campeao_minimo__med"].idxmax()]
        ref = ref_linha["campeao_minimo__med"]
        r = resumos[(resumos["jogo"] == jogo) & (resumos["campeao_minimo"] >= ref - tolerancia)]
        saida.append(f"== {nome_jogo(jogo)} ==")
        saida.append(f"referência: limite {int(ref_linha['limite'])}, pior winrate médio {100 * ref:.2f}%")
        saida.append(f"tolerância: {100 * tolerancia:.2f} ponto(s) -> mínimo aceito {100 * (ref - tolerancia):.2f}%")
        if r.empty:
            saida.append("nenhum campeão individual chegou à faixa (a média vem de sementes mais fracas e mais fortes)\n")
            continue
        m = r.sort_values(["campeao_nos", "campeao_minimo"], ascending=[True, False]).iloc[0]
        saida.append(f"menor árvore: {int(m['campeao_nos'])} nós ({int(m['campeao_nos_efetivos'])} efetivos), "
                     f"profundidade {int(m['campeao_profundidade'])}, limite {int(m['limite'])}, semente {int(m['semente'])}")
        saida.append(f"pior winrate {100 * m['campeao_minimo']:.2f}% ({m['campeao_winrates']})")
        saida.append(f"{m['campeao_arvore']}\n")
    return "\n".join(saida)


# ---------------------------------------------------------------- gráficos
def grade_jogos(jogos, linhas=1, altura=3.2, compartilhar_y="row"):
    fig, eixos = plt.subplots(linhas, len(jogos), figsize=(4.6 * len(jogos), altura * linhas),
                              squeeze=False, sharey=compartilhar_y)
    for c, jogo in enumerate(jogos):
        eixos[0][c].set_title(nome_jogo(jogo), loc="left")
    return fig, eixos


def legenda_limites(fig, limites, linha=True):
    alcas = [Line2D([], [], color=cor_limite(i, len(limites)), marker=MARCADORES[i % len(MARCADORES)],
                    linestyle="-" if linha else "none", markersize=6, label=f"limite {l}")
             for i, l in enumerate(limites)]
    fig.legend(handles=alcas, loc="upper center", ncol=len(alcas), bbox_to_anchor=(0.5, 1.0),
               title="profundidade máxima dos filhos", title_fontsize=9, fontsize=9)


def curvas_por_geracao(eixo, ag, jogo, coluna, limites):
    for _, (lim, part, rot) in enumerate(configuracoes(ag, jogo)):
        i = limites.index(lim)
        d = ag[(ag["jogo"] == jogo) & (ag["limite"] == lim) & (ag["partidas"] == part)].sort_values("geracao")
        cor = cor_limite(i, len(limites))
        x, y, dp = d["geracao"], d[coluna + "__med"], d[coluna + "__dp"]
        if dp.notna().any():  # faixa de ± 1 desvio padrão entre sementes
            eixo.fill_between(x, y - dp, y + dp, color=cor, alpha=0.15, linewidth=0)
        eixo.plot(x, y, color=cor, label=rot)
        # marcador só no fim: identifica a curva sem poluir
        eixo.plot(x.iloc[-1:], y.iloc[-1:], color=cor, marker=MARCADORES[i % len(MARCADORES)],
                  markersize=6, markeredgecolor="white", markeredgewidth=1, linestyle="none")
    eixo.xaxis.set_major_locator(matplotlib.ticker.MaxNLocator(integer=True))  # geração 2, nunca 2,5


def salvar(fig, saida: Path, nome, formatos):
    # reserva ~0,7 polegada no alto para a legenda, seja a figura de 1 ou de 2 linhas
    fig.tight_layout(rect=(0, 0, 1, 1 - 0.7 / fig.get_figheight()))
    for fmt in formatos:
        fig.savefig(saida / f"{nome}.{fmt}", bbox_inches="tight")
    plt.close(fig)


def fig_fitness(ag, saida, formatos):
    jogos, limites = jogos_presentes(ag), todos_limites(ag)
    fig, eixos = grade_jogos(jogos, linhas=2)
    for c, jogo in enumerate(jogos):
        curvas_por_geracao(eixos[0][c], ag, jogo, "melhor_minimo", limites)
        curvas_por_geracao(eixos[1][c], ag, jogo, "media_minimo", limites)
        for l in range(2):
            linha_empate(eixos[l][c])
            porcentagem(eixos[l][c])
        eixos[1][c].set_xlabel("geração")
    eixos[0][0].set_ylabel("melhor da geração\npior winrate (medido)")
    eixos[1][0].set_ylabel("média da população\npior winrate")
    legenda_limites(fig, limites)
    salvar(fig, saida, "fitness_por_geracao", formatos)


def fig_bloat(ag, saida, formatos):
    jogos, limites = jogos_presentes(ag), todos_limites(ag)
    fig, eixos = grade_jogos(jogos, linhas=2)
    for c, jogo in enumerate(jogos):
        curvas_por_geracao(eixos[0][c], ag, jogo, "nos_medio", limites)
        curvas_por_geracao(eixos[1][c], ag, jogo, "profundidade_media", limites)
        eixos[1][c].set_xlabel("geração")
    eixos[0][0].set_ylabel("nós (média da população)")
    eixos[1][0].set_ylabel("profundidade\n(média da população)")
    legenda_limites(fig, limites)
    salvar(fig, saida, "bloat_por_geracao", formatos)


def fig_diversidade(ag, saida, formatos):
    jogos, limites = jogos_presentes(ag), todos_limites(ag)
    fig, eixos = grade_jogos(jogos)
    for c, jogo in enumerate(jogos):
        curvas_por_geracao(eixos[0][c], ag, jogo, "distintas", limites)
        eixos[0][c].set_xlabel("geração")
    eixos[0][0].set_ylabel("árvores distintas\nna população")
    legenda_limites(fig, limites)
    salvar(fig, saida, "diversidade_por_geracao", formatos)


def pontos_e_media(eixo, x, valores, cor, marcador, deslocamento=0.0):
    """Cada semente como um ponto pequeno e a média ± desvio como um marcador maior."""
    x = x + deslocamento
    jitter = np.linspace(-0.06, 0.06, len(valores)) if len(valores) > 1 else np.zeros(1)
    eixo.scatter(x + jitter, valores, s=14, color=cor, alpha=0.45, linewidths=0, zorder=2)
    m = np.mean(valores)
    dp = np.std(valores, ddof=1) if len(valores) > 1 else 0.0
    eixo.errorbar([x], [m], yerr=[dp], color=cor, marker=marcador, markersize=7, capsize=3,
                  markeredgecolor="white", markeredgewidth=1, linewidth=1.4, zorder=3)


def eixo_limites(eixo, limites):
    eixo.set_xticks(range(len(limites)), [str(l) for l in limites])
    eixo.set_xlim(-0.5, len(limites) - 0.5)
    eixo.grid(axis="x", visible=False)


def fig_final_por_limite(res, saida, formatos):
    jogos, limites = jogos_presentes(res), todos_limites(res)
    fig, eixos = grade_jogos(jogos, linhas=2, compartilhar_y=False)
    for c, jogo in enumerate(jogos):
        r = res[res["jogo"] == jogo]
        for i, lim in enumerate(limites):
            d = r[r["limite"] == lim]
            if d.empty:
                continue
            pontos_e_media(eixos[0][c], i, d["campeao_minimo"].to_numpy(), cor_limite(i, len(limites)),
                           MARCADORES[i % len(MARCADORES)])
            pontos_e_media(eixos[1][c], i, d["campeao_nos"].to_numpy(), CATEGORICAS[0], "o", -0.15)
            pontos_e_media(eixos[1][c], i, d["campeao_nos_efetivos"].to_numpy(), CATEGORICAS[1], "s", 0.15)
        porcentagem(eixos[0][c])
        for l in range(2):
            eixo_limites(eixos[l][c], limites)
        eixos[1][c].set_xlabel("limite de profundidade")
        eixos[1][c].set_ylim(bottom=0)
    eixos[0][0].set_ylabel("campeão: pior winrate\n(reavaliado)")
    eixos[1][0].set_ylabel("nós do campeão")
    alcas = [Line2D([], [], color=CATEGORICAS[0], marker="o", linestyle="none", label="nós no total"),
             Line2D([], [], color=CATEGORICAS[1], marker="s", linestyle="none", label="nós efetivos (usados nas decisões)")]
    fig.legend(handles=alcas, loc="upper center", ncol=2, bbox_to_anchor=(0.5, 1.0), fontsize=9,
               title="pontos pequenos: uma semente · marcador grande: média ± desvio padrão", title_fontsize=9)
    salvar(fig, saida, "final_por_limite", formatos)


def fig_desempenho_vs_tamanho(res, final, saida, formatos, tolerancia):
    jogos, limites = jogos_presentes(res), todos_limites(res)
    fig, eixos = grade_jogos(jogos, compartilhar_y=False)
    for c, jogo in enumerate(jogos):
        eixo = eixos[0][c]
        r = res[res["jogo"] == jogo]
        ref = final[final["jogo"] == jogo]["campeao_minimo__med"].max()
        eixo.axhline(ref - tolerancia, color=APAGADA, linewidth=0.9, linestyle=(0, (4, 3)), zorder=1)
        eixo.text(1.0, ref - tolerancia, virgula(f"melhor média − {100 * tolerancia:.1f} ponto"), transform=eixo.get_yaxis_transform(),
                  va="bottom", ha="right", fontsize=8, color=TINTA_2)
        for i, lim in enumerate(limites):
            d = r[r["limite"] == lim]
            eixo.scatter(d["campeao_nos"], d["campeao_minimo"], s=36, color=cor_limite(i, len(limites)),
                         marker=MARCADORES[i % len(MARCADORES)], edgecolors="white", linewidths=0.8, zorder=3)
        eixo.set_xscale("log")
        eixo.xaxis.set_major_formatter(matplotlib.ticker.FuncFormatter(lambda v, _: f"{v:g}"))
        porcentagem(eixo)
        eixo.set_xlabel("nós do campeão (escala log)")
    eixos[0][0].set_ylabel("campeão: pior winrate\n(reavaliado)")
    legenda_limites(fig, limites, linha=False)
    salvar(fig, saida, "desempenho_vs_tamanho", formatos)


def fig_adversarios(res, saida, formatos):
    jogos, limites = jogos_presentes(res), todos_limites(res)
    fig, eixos = grade_jogos(jogos, compartilhar_y=False)
    todos_adv = []
    for c, jogo in enumerate(jogos):
        r = res[res["jogo"] == jogo]
        advs = [col[2:] for col in r.columns if col.startswith("w_") and r[col].notna().any()]
        for a in advs:
            if a not in todos_adv:
                todos_adv.append(a)
        largura = 0.5 / max(len(advs), 1)
        for k, a in enumerate(advs):
            j = todos_adv.index(a)
            desl = (k - (len(advs) - 1) / 2) * largura
            for i, lim in enumerate(limites):
                d = r[r["limite"] == lim]["w_" + a].dropna()
                if not d.empty:
                    pontos_e_media(eixos[0][c], i, d.to_numpy(), CATEGORICAS[j % 3], MARCADORES[j], desl)
        linha_empate(eixos[0][c])
        porcentagem(eixos[0][c])
        eixo_limites(eixos[0][c], limites)
        eixos[0][c].set_xlabel("limite de profundidade")
    eixos[0][0].set_ylabel("campeão: winrate\n(reavaliado)")
    alcas = [Line2D([], [], color=CATEGORICAS[j % 3], marker=MARCADORES[j], linestyle="none", label=f"contra {nome_adversario(a)}")
             for j, a in enumerate(todos_adv)]
    fig.legend(handles=alcas, loc="upper center", ncol=len(alcas), bbox_to_anchor=(0.5, 1.0), fontsize=9)
    salvar(fig, saida, "winrate_por_adversario", formatos)


def fig_sorte(res, saida, formatos):
    jogos, limites = jogos_presentes(res), todos_limites(res)
    fig, eixos = grade_jogos(jogos, compartilhar_y=False)
    for c, jogo in enumerate(jogos):
        eixo = eixos[0][c]
        r = res[res["jogo"] == jogo]
        valores = pd.concat([r["campeao_minimo_na_evolucao"], r["campeao_minimo"]])
        lo, hi = valores.min() - 0.01, valores.max() + 0.01
        eixo.plot([lo, hi], [lo, hi], color=APAGADA, linewidth=0.9, linestyle=(0, (4, 3)), zorder=1)
        amp = hi - lo
        # o canto de cima à esquerda fica sempre vazio (acima da diagonal quase não há pontos)
        eixo.text(lo + 0.04 * amp, hi - 0.04 * amp, "linha: medido = reavaliado\nabaixo dela: sorte na evolução",
                  ha="left", va="top", fontsize=8, color=TINTA_2)
        for i, lim in enumerate(limites):
            d = r[r["limite"] == lim]
            eixo.scatter(d["campeao_minimo_na_evolucao"], d["campeao_minimo"], s=36, color=cor_limite(i, len(limites)),
                         marker=MARCADORES[i % len(MARCADORES)], edgecolors="white", linewidths=0.8, zorder=3)
        eixo.set_xlim(lo, hi)
        eixo.set_ylim(lo, hi)
        porcentagem(eixo)
        porcentagem(eixo, "x")
        eixo.set_xlabel("pior winrate na evolução")
    eixos[0][0].set_ylabel("pior winrate reavaliado\n(partidas novas)")
    legenda_limites(fig, limites, linha=False)
    salvar(fig, saida, "sorte_vs_reavaliado", formatos)


# ---------------------------------------------------------------- main
def main():
    ap = argparse.ArgumentParser(description="Tabelas e gráficos de uma bateria de experimentos.")
    ap.add_argument("pasta", type=Path, help="pasta da bateria (a SAIDA do scripts/bateria.sh)")
    ap.add_argument("--saida", type=Path, help="onde gravar (padrão: <pasta>/graficos)")
    ap.add_argument("--formatos", default="png,pdf", help="formatos das imagens, separados por vírgula (padrão: png,pdf)")
    ap.add_argument("--tolerancia", type=float, default=0.005,
                    help="quanto abaixo da melhor média ainda conta como 'mantém o desempenho' (padrão: 0.005 = 0,5 ponto)")
    args = ap.parse_args()

    if not args.pasta.is_dir():
        sys.exit(f"não existe a pasta {args.pasta}")
    saida = args.saida or args.pasta / "graficos"
    saida.mkdir(parents=True, exist_ok=True)
    formatos = [f.strip() for f in args.formatos.split(",") if f.strip()]

    resumos = ler_resumos(args.pasta)
    historicos = ler_historicos(resumos)
    adversarios = [c[2:] for c in resumos.columns if c.startswith("w_")]

    ag = agregar(historicos, CHAVE + ["geracao"], METRICAS_GERACAO)
    final = agregar(resumos, CHAVE, METRICAS_FINAIS + ["w_" + a for a in adversarios])
    ag.to_csv(saida / "agregado_geracoes.csv", index=False, float_format="%.6f")
    final.to_csv(saida / "agregado_final.csv", index=False, float_format="%.6f")
    (saida / "tabela_final.md").write_text(tabela_markdown(final, adversarios), encoding="utf-8")
    (saida / "menores_arvores.txt").write_text(menores_arvores(resumos, final, args.tolerancia), encoding="utf-8")

    estilo()
    fig_fitness(ag, saida, formatos)
    fig_bloat(ag, saida, formatos)
    fig_diversidade(ag, saida, formatos)
    fig_final_por_limite(resumos, saida, formatos)
    fig_desempenho_vs_tamanho(resumos, final, saida, formatos, args.tolerancia)
    fig_adversarios(resumos, saida, formatos)
    fig_sorte(resumos, saida, formatos)

    sementes = resumos.groupby(CHAVE).size()
    print(f"{len(resumos)} rodadas, {len(sementes)} configurações "
          f"({sementes.min()}–{sementes.max()} sementes cada); gravado em {saida}")


if __name__ == "__main__":
    main()
