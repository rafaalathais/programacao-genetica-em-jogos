# Análise de Compromisso entre Tamanho de Árvore (Bloat) e Desempenho em Programação Genética Aplicada a Jogos

Implementação em **C++** de um motor genérico de Programação Genética (PG) para a evolução de estratégias autônomas em jogos estocásticos de tomada de decisão sequencial (**Pig** e **Blackjack de Dados**). 

Este projeto serve como base experimental para investigar o fenômeno do *bloat* (crescimento estrutural excessivo das árvores sem ganho proporcional de desempenho) e identificar o **menor tamanho de árvore evoluída capaz de manter alta qualidade competitiva** em diferentes perfis de jogos.


## Objetivos do Projeto

1. **Motor Genérico Orientado a Dados:** Evoluir estratégias de decisão binária (`CONTINUAR` ou `PARAR`) sem alterar o núcleo do algoritmo genético, alterando apenas arquivos de configuração `.json` com as regras e o vocabulário de cada jogo.
2. **Análise de Bloat vs. Qualidade:** Avaliar o impacto de diferentes limites de profundidade e número de nós sobre a taxa de vitória (*winrate*) e o tempo de execução.
3. **Comparação entre Perfis de Jogos:** Contrastar a complexidade estrutural mínima exigida pelo **Pig (1 dado)** em relação ao **Blackjack de Dados**.

## Arquitetura do Sistema

### Configuração via datasets json

O comportamento do algoritmo e as regras dos ambientes são controlados externamente por três tipos de arquivos JSON:

* Parâmetros Evolutivos (`parametros.json`): Define tamanho da população, máximo de gerações, semente aleatória (seed), limites de profundidade (profundidade_min, profundidade_max, profundidade_limite_filhos), taxas de cruzamento/mutação e critérios de parada antecipada.

* Especificações do Jogo (`specs_*.json`): Define as variáveis de estado (com intervalos `min` e `max`), ações possíveis, comparadores permitidos e regras específicas da simulação (ex.: pontuação alvo, faces do dado, regra de estouro).

* Protocolo de Avaliação (`avaliacao_*.json`): Define o número de partidas por árvore, alternância de turno inicial e os agentes de referência (baselines como `aleatorio`, `limiar` e `superar`).

## Métricas Coletadas
Para responder às perguntas de pesquisa do artigo, o módulo de estatísticas registra ao longo das gerações:
* **Taxa de Vitória (winrate/fitness)**: Desempenho médio contra os oponentes de referência.
* **Número de Nós**: Complexidade estrutural total da árvore.
* **Profundidade**: Nível máximo de ramificação da estratégia.
* **Tempo de Execução**: Custo computacional de evolução e avaliação.

## Como compilar e executar
Pré-requisitos:
* Compilador de C++

(teste)
``g++  -Iinclude teste_gerador.cpp src/No.cpp src/GeradorArvores.cpp -o teste_gerador ``

