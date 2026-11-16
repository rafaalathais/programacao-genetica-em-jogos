//
// Created by leon on 16/11/2026.
//

#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_VOCABULARIO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_VOCABULARIO_H
#include "nlohmann/json.hpp"
#include <string>
#include <vector>

struct Variavel {
    std::string nome;
    int min;
    int max;
};

struct Vocabulario {
    std::vector<Variavel> variaveis;
    std::vector<std::string> acoes;
    std::vector<std::string> comparacoes;
};

 NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(Variavel,
    nome,
    min,
    max
);

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(
    Vocabulario,
    variaveis,
    acoes,
    comparacoes
);
#endif //PROGRAMACAO_GENETICA_EM_JOGOS_VOCABULARIO_H
