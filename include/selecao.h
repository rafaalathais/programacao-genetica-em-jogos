#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_SELECAO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_SELECAO_H

#include <cstddef>
#include <vector>

#include "aleatorio.h"
#include "fitness.h"

// A seleção trabalha só com os fitness (posição i = indivíduo i da população):
// devolve ÍNDICES, sem copiar árvores.

// Torneio: sorteia `tamanho` indivíduos (com reposição: o mesmo pode sair mais de
// uma vez) e devolve o índice do melhor. Em empate, fica o que saiu primeiro.
std::size_t torneio(const std::vector<Fitness>& fitness, int tamanho, Aleatorio& aleatorio);

// Elitismo: índices dos `quantidade` melhores, do melhor para o pior.
// Em empate, o de menor índice vem antes (resultado sempre o mesmo).
std::vector<std::size_t> melhores(const std::vector<Fitness>& fitness, int quantidade);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_SELECAO_H
