#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_CONFIGURACAO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_CONFIGURACAO_H

#include "nlohmann/json.hpp"
#include "criterioAvaliador.h"
#include "parametros.h"

// Converte o json do parametros.json (já lido pelo LeitorJSON) na struct Parametros
// e confere os valores. Lança std::runtime_error explicando o problema.
//
// Além da conversão das macros do nlohmann, faz uma "conferência de ida e volta":
// converte a struct de volta para json e compara com o original. Isso pega dois
// erros que a macro deixaria passar calados:
//   - número quebrado num campo int ("populacao": 100.5 viraria 100);
//   - chave a mais ou com erro de digitação ("popolacao": 100 seria ignorada).
Parametros lerParametros(const nlohmann::json& json);

// Confere as regras entre os campos (probabilidades em [0,1], elitismo menor que a
// população, profundidades em ordem etc.). Lista TODOS os problemas de uma vez.
void validarParametros(const Parametros& p);

// Confere o avaliador_*.json já convertido: partidas_por_arvore >= 1 (e par quando se
// alterna quem começa, para cada lado começar metade das vezes), fitness conhecido e
// pelo menos um adversário de tipo conhecido. Lista todos os problemas de uma vez.
// (Os campos de cada adversário são conferidos por criarAdversario, que conhece o vocabulário.)
void validarCriterio(const CriterioAvaliador& c);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_CONFIGURACAO_H
