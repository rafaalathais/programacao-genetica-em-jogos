#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_PARAMETROS_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_PARAMETROS_H
#include "nlohmann/json.hpp"


struct Selecao {
    int torneio_tamanho;
    int elitismo;
};

struct Cruzamento {
    double probabilidade;
    double prob_ponto_interno;
};

struct Mutacao {
    double probabilidade;
    double prob_so_constante;
    int profundidade_subarvore;
    double variacao_constante;
};

struct Parada {
    double winrate_alvo;
    int geracoes_sem_melhora;
};

struct Parametros {
    int populacao;
    int geracoes_max;
    int semente;

    int profundidade_min;
    int profundidade_max;
    int profundidade_limite_filhos;

    Selecao selecao;
    Cruzamento cruzamento;
    Mutacao mutacao;
    Parada parada;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Selecao,
    torneio_tamanho, elitismo);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Cruzamento,
    probabilidade, prob_ponto_interno);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Mutacao,
    probabilidade, prob_so_constante,
    profundidade_subarvore, variacao_constante);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Parada,
    winrate_alvo, geracoes_sem_melhora);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Parametros,
    populacao, geracoes_max, semente,
    profundidade_min, profundidade_max,
    profundidade_limite_filhos,
    selecao, cruzamento, mutacao, parada);

#endif //PROGRAMACAO_GENETICA_EM_JOGOS_PARAMETROS_H
