# programacao-genetica-em-jogos

Programação genética com árvores tipadas que evolui estratégias CONTINUAR/PARAR para
dois jogos de dados: Pig e Blackjack de dados. C++17, json com nlohmann/json.

## Compilar e testar

```bash
make testes        # todos os testes, com -Wall -Wextra e sanitizers
make experimento   # o executável dos experimentos (-O2)
```

## Rodar experimentos

Uma rodada (um jogo, uma semente):

```bash
./experimento --jogo pig --semente 1
./experimento --jogo pig --limite 3,5,7,10    # uma rodada por limite de profundidade
./experimento --ajuda                          # todas as opções
```

As opções trocam os valores dos json só naquela execução; os arquivos de `dataset/` nunca
são alterados.

A bateria do artigo (2 jogos × limites 3, 5, 7, 10 × sementes 1 a 10, em paralelo):

```bash
RAPIDO=1 scripts/bateria.sh   # conferência em segundos
scripts/bateria.sh            # bateria completa (~50-60 min em 12 núcleos)
```

Qualquer padrão pode ser trocado na linha de comando, por exemplo
`LIMITES=3,5,7,10,12 SEMENTES="1 2 3" PARTIDAS=2000 scripts/bateria.sh`.
Tudo vai para `resultados/bateria_<data>_<hora>/`.

## Tabelas e gráficos

A bateria roda `scripts/graficos.py` no fim. Para rodar à mão (precisa de
`pip install pandas matplotlib`; os testes estatísticos usam também `scipy`):

```bash
python3 scripts/graficos.py resultados/bateria_<data>_<hora>
```

Ele grava em `<pasta da bateria>/graficos/`:

| Arquivo | O que mostra |
|---|---|
| `fitness_por_geracao` | pior winrate do melhor de cada geração e da média da população, por limite |
| `bloat_por_geracao` | nós e profundidade médios da população, por limite |
| `diversidade_por_geracao` | árvores distintas na população |
| `final_por_limite` | campeão reavaliado por limite: pior winrate e nós (total × efetivos) |
| `desempenho_vs_tamanho` | pior winrate × nós de cada campeão (escala log) |
| `winrate_por_adversario` | winrate do campeão contra cada adversário |
| `sorte_vs_reavaliado` | número medido na evolução × reavaliado com partidas novas |
| `tabela_final.md` | a tabela principal (média ± desvio padrão entre sementes) |
| `menores_arvores.txt` | a menor árvore que fica a até 0,5 ponto da melhor média, por jogo |
| `testes_estatisticos.md`, `.csv` | diferença entre limites (Kruskal-Wallis; Mann-Whitney par a par com correção de Bonferroni) no pior winrate, nos nós e nos nós efetivos do campeão, e correlação de Spearman entre tamanho e desempenho; nível de significância em `--alfa` (padrão 0,05) |
| `agregado_geracoes.csv`, `agregado_final.csv` | as médias (`__med`) e desvios (`__dp`) por configuração |

Os gráficos saem em `.png` e `.pdf` (vetorial, para o texto), com Pig e Blackjack lado a lado.
Cada painel tem a sua escala vertical: no blackjack as diferenças entre limites são de décimos
de ponto, e o eixo se ajusta a isso.

## Arquivos de saída de cada rodada

Em `<saida>/<jogo>/limite_<L>_partidas_<P>/`:

- `semente_<n>.csv`: uma linha por geração;
- `semente_<n>.resumo.csv`: uma linha com o resultado da rodada (a bateria junta todas em `resumo.csv`);
- `semente_<n>_campeao.txt`: a árvore campeã, legível.

Winrates são frações de 0 a 1: (vitórias + 0,5 × empates) / partidas. "Pior" é o menor winrate
entre os adversários; "média" é a média deles. O fitness compara primeiro o pior e, no empate, a média.

### `semente_<n>.csv` (histórico por geração)

| Coluna | Significado |
|---|---|
| `geracao` | 0 é a população inicial |
| `individuos` | tamanho da população |
| `melhor_minimo`, `melhor_media` | fitness do melhor da geração (medido na evolução) |
| `melhor_w_<adversário>` | winrate do melhor contra cada adversário |
| `melhor_nos`, `melhor_profundidade` | tamanho do melhor |
| `media_minimo`, `media_media` | média do fitness na população |
| `nos_medio`, `nos_max` | nós da população (média e máximo) |
| `profundidade_media`, `profundidade_max` | profundidade da população |
| `distintas` | árvores diferentes na população |
| `cruzamentos`, `mutacoes` | operadores aplicados para formar a geração |
| `rejeitadas` | candidatas que bateram o alvo mas não o confirmaram com partidas novas |
| `segundos` | tempo da geração |

### `resumo.csv` (uma linha por rodada)

| Coluna | Significado |
|---|---|
| `jogo`, `rotulo`, `semente` | identificação da rodada |
| `populacao`, `limite_profundidade`, `partidas`, `partidas_confirmacao` | configuração usada |
| `geracoes`, `motivo` | quantas gerações e por que parou (`alvo atingido`, `sem melhora`, `geracoes_max`) |
| `rejeitadas` | candidatas barradas pela confirmação |
| `campeao_minimo`, `campeao_media` | fitness do campeão reavaliado com `partidas_confirmacao` partidas novas |
| `campeao_winrates` | winrate reavaliado contra cada adversário (`aleatorio=0.88;limiar=0.52`) |
| `campeao_minimo_na_evolucao` | o pior winrate que o campeão tinha durante a evolução |
| `campeao_nos`, `campeao_nos_efetivos`, `campeao_profundidade` | tamanho do campeão; efetivos = nós visitados em decisões reais |
| `nos_medios_inicio`, `nos_medios_fim` | nós médios da população na geração 0 e na última |
| `nos_efetivos_medios_fim` | nós efetivos médios da população final |
| `distintas_fim` | árvores distintas na população final |
| `segundos` | tempo total da rodada |
| `campeao_arvore` | a árvore campeã em texto (última coluna, entre aspas) |
