#include "selecao.h"

#include <algorithm>
#include <numeric>
#include <stdexcept>

std::size_t torneio(const std::vector<Fitness>& fitness, int tamanho, Aleatorio& aleatorio) {
    if (fitness.empty()) throw std::invalid_argument("torneio: população vazia");
    if (tamanho < 1) throw std::invalid_argument("torneio: tamanho deve ser >= 1");
    std::size_t vencedor = aleatorio.indice(fitness.size());
    for (int i = 1; i < tamanho; ++i) {
        const std::size_t desafiante = aleatorio.indice(fitness.size());
        if (fitness[vencedor] < fitness[desafiante]) vencedor = desafiante;  // só troca se for estritamente melhor
    }
    return vencedor;
}

std::vector<std::size_t> melhores(const std::vector<Fitness>& fitness, int quantidade) {
    if (quantidade < 0 || static_cast<std::size_t>(quantidade) > fitness.size())
        throw std::invalid_argument("melhores: quantidade fora de [0, tamanho da população]");
    std::vector<std::size_t> indices(fitness.size());
    std::iota(indices.begin(), indices.end(), 0);  // 0, 1, 2, ...
    // stable_sort mantém a ordem original entre empatados -> menor índice primeiro.
    std::stable_sort(indices.begin(), indices.end(),
                     [&](std::size_t a, std::size_t b) { return fitness[b] < fitness[a]; });  // melhor primeiro
    indices.resize(static_cast<std::size_t>(quantidade));
    return indices;
}
