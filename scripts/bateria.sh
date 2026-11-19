#!/usr/bin/env bash
# Bateria de experimentos do artigo: jogos × limites de profundidade × sementes,
# várias rodadas ao mesmo tempo (uma por núcleo), e no fim junta os resumos.
#
# Uso, da raiz do projeto:
#   scripts/bateria.sh                       # bateria completa (padrões abaixo, ~50-60 min em 12 núcleos)
#   RAPIDO=1 scripts/bateria.sh              # bateria mínima, só para conferir que tudo funciona (segundos)
#   PARTIDAS=2000 CONFIRMACAO=20000 LIMITES=3,5,7,10,12 scripts/bateria.sh
#
# LIMITES aceita a lista com vírgulas (3,5,7,10, igual ao --limite do experimento) ou com espaços.
# Aqui cada limite vira rodadas separadas, para rodarem em paralelo em núcleos diferentes.
#
# Cada rodada grava em $SAIDA/<jogo>/limite_<L>_partidas_<P>/semente_<n>.*  (ver include/experimento.h)
# No fim: $SAIDA/resumo.csv (uma linha por rodada), $SAIDA/log.txt e $SAIDA/configuracao.txt,
# e as tabelas e os gráficos em $SAIDA/graficos/ (scripts/graficos.py; GRAFICOS=0 pula essa parte).
set -euo pipefail
cd "$(dirname "$0")/.."

JOGOS=${JOGOS:-"pig blackjack"}
LIMITES=${LIMITES:-"3,5,7,10"}        # limites de profundidade do esboço do artigo
SEMENTES=${SEMENTES:-"1 2 3 4 5 6 7 8 9 10"}
PARTIDAS=${PARTIDAS:-20000}        # partidas por adversário na evolução
CONFIRMACAO=${CONFIRMACAO:-100000} # partidas por adversário na reavaliação (maior que PARTIDAS)
GERACOES=${GERACOES:-50}
SEM_MELHORA=${SEM_MELHORA:-1000}   # alto = toda rodada vai até GERACOES (curvas de bloat comparáveis)
POPULACAO=${POPULACAO:-}           # vazio = a do parametros.json
GRAFICOS=${GRAFICOS:-1}            # 1 = roda scripts/graficos.py no fim
PARALELO=${PARALELO:-$(nproc)}
SAIDA=${SAIDA:-resultados/bateria_$(date +%Y%m%d_%H%M%S)}

if [[ "${RAPIDO:-0}" == 1 ]]; then
    LIMITES="3,5"; SEMENTES="1 2"; PARTIDAS=100; CONFIRMACAO=1000; GERACOES=3; POPULACAO=20
    SAIDA=${SAIDA_RAPIDO:-resultados/rapido}
fi

LIMITES=${LIMITES//,/ }             # "3,5,7,10" -> "3 5 7 10"

make -s experimento
mkdir -p "$SAIDA"
{
    echo "data: $(date)"
    echo "commit: $(git rev-parse --short HEAD 2>/dev/null || echo '?')$(git diff --quiet 2>/dev/null || echo ' (com alterações não commitadas)')"
    echo "jogos: $JOGOS"
    echo "limites: $LIMITES"
    echo "sementes: $SEMENTES"
    echo "partidas: $PARTIDAS  confirmacao: $CONFIRMACAO  geracoes: $GERACOES  sem_melhora: $SEM_MELHORA  populacao: ${POPULACAO:-json}"
    echo "em paralelo: $PARALELO"
} > "$SAIDA/configuracao.txt"

rodada() {
    local args=(--jogo "$1" --limite "$2" --semente "$3" --partidas "$PARTIDAS" --confirmacao "$CONFIRMACAO"
                --geracoes "$GERACOES" --sem-melhora "$SEM_MELHORA" --saida "$SAIDA")
    [[ -n "$POPULACAO" ]] && args+=(--populacao "$POPULACAO")
    ./experimento "${args[@]}" || { echo "FALHOU: $1 limite $2 semente $3"; return 1; }
}
export -f rodada
export PARTIDAS CONFIRMACAO GERACOES SEM_MELHORA POPULACAO SAIDA

# uma linha por rodada; o pig (mais lento) vai primeiro para equilibrar os núcleos
lista=()
for j in $JOGOS; do for l in $LIMITES; do for s in $SEMENTES; do lista+=("$j $l $s"); done; done; done
echo "rodadas: ${#lista[@]}, em paralelo: $PARALELO, saída: $SAIDA"
inicio=$(date +%s)
falhou=0
printf '%s\n' "${lista[@]}" | xargs -P "$PARALELO" -n 3 bash -c 'rodada "$@"' _ | tee "$SAIDA/log.txt" || falhou=1

# junta os resumos (cabeçalho uma vez só)
mapfile -t resumos < <(find "$SAIDA" -name '*.resumo.csv' | sort)
if (( ${#resumos[@]} > 0 )); then
    head -n 1 "${resumos[0]}" > "$SAIDA/resumo.csv"
    for f in "${resumos[@]}"; do tail -n +2 "$f" >> "$SAIDA/resumo.csv"; done
fi
echo "tempo total: $(( $(date +%s) - inicio )) s; rodadas concluídas: ${#resumos[@]} de ${#lista[@]}"

# tabela rápida por jogo e limite (as 22 primeiras colunas não têm vírgula; a árvore vem por último)
if [[ -f "$SAIDA/resumo.csv" ]]; then
    echo
    awk -F, 'NR > 1 { k = $1 " " $5; n[k]++; w[k] += $11; e[k] += $14; nos[k] += $15; ef[k] += $16; pop[k] += $19; t[k] += $22 }
        END {
            printf "%-10s %6s %3s %13s %13s %10s %10s %11s %8s\n", "jogo", "limite", "n", "campeão pior", "na evolução", "nós camp.", "efetivos", "nós pop.fim", "seg/rod"
            for (k in n) { split(k, a, " ")
                printf "%-10s %6s %3d %12.1f%% %12.1f%% %10.1f %10.1f %11.1f %8.1f\n", a[1], a[2], n[k], 100*w[k]/n[k], 100*e[k]/n[k], nos[k]/n[k], ef[k]/n[k], pop[k]/n[k], t[k]/n[k] }
        }' "$SAIDA/resumo.csv" | (read -r cab; echo "$cab"; sort -k1,1 -k2,2n)
fi
# tabelas e gráficos (não derruba a bateria se faltar python/pandas/matplotlib: os csv já estão salvos)
if [[ "$GRAFICOS" == 1 && -f "$SAIDA/resumo.csv" ]]; then
    echo
    if python3 -c "import pandas, matplotlib" 2>/dev/null; then
        python3 scripts/graficos.py "$SAIDA" || echo "aviso: scripts/graficos.py falhou; rode de novo à mão"
    else
        echo "gráficos pulados: falta pandas/matplotlib (pip install pandas matplotlib)."
        echo "depois, rode: python3 scripts/graficos.py $SAIDA"
    fi
fi
exit $falhou
