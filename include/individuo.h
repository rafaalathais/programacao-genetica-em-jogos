#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_INDIVIDUO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_INDIVIDUO_H

#include "arvore.h"
#include "avaliador.h"

// Um indivíduo da população: a árvore e o que se sabe dela.
// (Ideia do `Individuo` da Rafaela, com unique_ptr e o Fitness em par.)
//
// Como a árvore é um unique_ptr, o Individuo só pode ser MOVIDO, não copiado
// sem querer. Para duplicar (ex.: guardar o melhor de todos), use copiarIndividuo,
// que faz cópia profunda da árvore.
struct Individuo {
    Arvore arvore;
    Avaliacao avaliacao;    // fitness (pior winrate, média) + placar contra cada adversário
    int profundidade = 0;   // em arestas
    int nos = 0;
};

inline Individuo copiarIndividuo(const Individuo& i) {
    return Individuo{copiar(*i.arvore), i.avaliacao, i.profundidade, i.nos};
}

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_INDIVIDUO_H
