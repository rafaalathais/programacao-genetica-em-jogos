#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_PARTIDA_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_PARTIDA_H

#include "jogador.h"
#include "jogo.h"

// Joga uma partida inteira entre j1 (jogador 0) e j2 (jogador 1).
// Não conhece nenhum jogo: só usa a interface Jogo.
Resultado jogarPartida(Jogo& jogo, Jogador& j1, Jogador& j2, bool j1Comeca);

// Contagem do ponto de vista de j1.
struct Placar {
    int vitorias = 0;
    int derrotas = 0;
    int empates = 0;

    int partidas() const { return vitorias + derrotas + empates; }
    // w = (vitórias + 0.5 * empates) / partidas
    double winrate() const { return partidas() == 0 ? 0.0 : (vitorias + 0.5 * empates) / partidas(); }
};

// Joga `partidas` partidas. Com `alternar` (o padrão), j1 começa nas de índice par e j2
// nas ímpares: com um número par de partidas, cada um começa exatamente metade das vezes.
// Sem `alternar`, j1 começa todas.
Placar jogarSerie(Jogo& jogo, Jogador& j1, Jogador& j2, int partidas, bool alternar = true);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_PARTIDA_H
