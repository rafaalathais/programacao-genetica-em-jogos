# Análise de Compromisso entre Tamanho de Árvore (Bloat) e Desempenho em Programação Genética Aplicada a Jogos

Programação genética com árvores tipadas que evolui estratégias CONTINUAR/PARAR para dois jogos de dados: Pig e Blackjack de dados. C++17, json com nlohmann/json. Este projeto serve como base experimental para investigar o fenômeno do *bloat* (crescimento estrutural excessivo das árvores sem ganho proporcional de desempenho) e identificar o **menor tamanho de árvore evoluída capaz de manter alta qualidade competitiva** em diferentes perfis de jogos.


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

## Parâmetros de entrada

Os valores do experimento vêm de cinco arquivos em `dataset/`. As poucas regras que ficam no código
estão anotadas no fim de cada parte.

| Arquivo | Conteúdo |
|---|---|
| `parametros.json` | o algoritmo genético (os dois jogos usam o mesmo) |
| `specs_regras_pig.json`, `specs_regras_blackjack.json` | o vocabulário das árvores e as regras de cada jogo |
| `avaliador_pig.json`, `avaliador_blackjack.json` | os adversários e o número de partidas de cada jogo |

A coluna "Opção" diz qual opção do `./experimento` troca o valor só naquela execução. A coluna
"Bateria" mostra o que o `scripts/bateria.sh` usa quando ele é diferente do json.

### `parametros.json`

| Chave | Padrão | Opção | Bateria | O que faz |
|---|---|---|---|---|
| `populacao` | 100 | `--populacao` | | tamanho da população, igual em toda geração |
| `geracoes_max` | 50 | `--geracoes` | | número máximo de gerações |
| `semente` | 42 | `--semente` | 1 a 10 | semente do único gerador de números aleatórios; a mesma semente repete a rodada |
| `profundidade_min`, `profundidade_max` | 2, 5 | | | faixa de profundidade da população inicial (ramped half-and-half) |
| `profundidade_limite_filhos` | 6 | `--limite` | 3, 5, 7, 10 | nenhum filho passa dessa profundidade; com `--limite`, `profundidade_max` é reduzida para caber nele |
| `selecao.torneio_tamanho` | 3 | | | indivíduos sorteados em cada torneio; vence o de maior fitness |
| `selecao.elitismo` | 2 | | | os melhores passam para a geração seguinte sem alteração |
| `cruzamento.probabilidade` | 0,85 | | | chance de um filho vir de cruzamento; senão, é cópia de um pai |
| `cruzamento.prob_ponto_interno` | 0,9 | | | chance de o ponto de corte ser um nó interno, e não uma folha |
| `mutacao.probabilidade` | 0,05 | | | chance de mutar cada filho, depois do cruzamento |
| `mutacao.prob_so_constante` | 0,5 | | | chance de a mutação só trocar uma constante; senão, troca uma subárvore |
| `mutacao.profundidade_subarvore` | 2 | | | profundidade máxima da subárvore nova na mutação |
| `mutacao.variacao_constante` | 3 | | | a constante nova fica a até ±3 da antiga, dentro da faixa da variável |
| `parada.winrate_alvo` | 0,60 | `--alvo` | | para quando o pior winrate do melhor chega aqui (e é confirmado com partidas novas) |
| `parada.geracoes_sem_melhora` | 10 | `--sem-melhora` | 1000 | para depois desse número de gerações sem o melhor fitness subir; 1000 desliga o critério |
| `parada.partidas_confirmacao` | 20000 | `--confirmacao` | 100000 | partidas novas por adversário para confirmar o alvo e reavaliar o campeão |

As probabilidades são sorteadas para cada filho. Não são cotas exatas por geração.

No código ficam as chances do gerador de árvores (no método CRESCER, um nó vira SE com 50%; uma
comparação usa outra variável, quando há uma compatível, com 50%) e o limite de 20 tentativas para
evitar árvores repetidas na população inicial e filhos acima do limite.

### `specs_regras_<jogo>.json`

O vocabulário (`variaveis`, `acoes`, `comparacoes`) diz com o que as árvores podem ser montadas.
Cada variável tem uma faixa `min`/`max`, e as constantes das comparações são sorteadas dentro dela.
Os nomes das variáveis precisam ser os que o simulador conhece, porque ele as procura pelo nome:

| Jogo | Variáveis (faixa) |
|---|---|
| Pig | `pontos_turno`, `meu_placar`, `placar_adv` (0 a 100) |
| Blackjack | `minha_soma`, `soma_adv` (0 a 21), `sou_segundo` (0 ou 1) |

Pelo json dá para mudar as faixas e as comparações. Uma variável nova exige mudar o simulador.

O bloco `regras` vai direto para o simulador do jogo:

| Jogo | Chave | Valor | O que faz |
|---|---|---|---|
| Pig | `pontos_para_vencer` | 100 | placar que encerra a partida |
| Pig | `dado.faces`, `dado.quantidade` | 6, 1 | um dado de 6 faces (só a variante de 1 dado existe) |
| Pig | `face_q_zera_turno` | 1 | tirar essa face perde os pontos do turno e passa a vez |
| Pig | `vitoria_imediata` | true | vence assim que o placar mais os pontos do turno chegam ao alvo, sem precisar parar |
| Blackjack | `limite_de_estouro` | 21 | passar disso perde a partida |
| Blackjack | `dado.faces` | 6 | faces de cada dado |
| Blackjack | `dado.quantidade`, `dado.quantidade_reduzida`, `dado.reduz_a_partir_de` | 2, 1, 16 | rola 2 dados enquanto a soma é menor que 16, e 1 dado a partir daí |
| Blackjack | `rolagem_obrigatoria` | true | a primeira rolagem da mão é automática |
| Blackjack | `empate` | `"meio_ponto"` | somas iguais valem meio ponto para cada um (única opção) |

Duas regras do Pig ficam no código, e não no json: a primeira rolagem de cada turno é obrigatória
(sem ela, duas árvores que param com 0 pontos jogariam para sempre) e uma partida com mais de 1000
turnos termina empatada.

### `avaliador_<jogo>.json`

| Chave | Padrão | Opção | Bateria | O que faz |
|---|---|---|---|---|
| `partidas_por_arvore` | 500 | `--partidas` | 20000 | partidas contra cada adversário na evolução (par) |
| `alternar_quem_comeca` | true | | | cada jogador começa metade das partidas |
| `adversarios` | ver abaixo | | | os jogadores fixos contra quem as árvores são medidas |
| `fitness` | `"minimo_com_desempate_pela_media"` | | | pior winrate entre os adversários; a média desempata (único critério) |

Tipos de adversário:

| Tipo | Campos | Jogo | O que faz |
|---|---|---|---|
| `aleatorio` | nenhum | os dois | CONTINUAR ou PARAR com 50% de chance |
| `limiar` | `variavel`, `valor` | Pig: `pontos_turno` 20; Blackjack: `minha_soma` 17 | continua até a variável chegar ao valor |
| `superar` | `limiar_se_primeiro` | Blackjack: 17 | jogando primeiro, para em 17; jogando em segundo, continua até igualar ou passar a soma do adversário |

Para usar outros arquivos sem mexer nos de `dataset/`: `--parametros`, `--spec` e `--avaliador`.

## Tabelas e gráficos

A bateria roda `scripts/graficos.py` no fim. Para rodar à mão (precisa de
`pip install pandas matplotlib`):

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
