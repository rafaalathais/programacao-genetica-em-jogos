#include "partida.h"

#include <stdexcept>

// Proteção contra um jogo com defeito que nunca termine. Os simuladores já
// garantem o fim (Pig tem limite de turnos, no blackjack quem continua acaba
// estourando); se mesmo assim passar disso, é bug.
static constexpr long LIMITE_DECISOES = 10'000'000;

Resultado jogarPartida(Jogo& jogo, Jogador& j1, Jogador& j2, bool j1Comeca) {
    Jogador* jogadores[2] = {&j1, &j2};
    jogo.reiniciar(j1Comeca ? 0 : 1);
    for (long n = 0; !jogo.terminou(); ++n) {
        if (n >= LIMITE_DECISOES) throw std::logic_error("jogarPartida: a partida não terminou (bug no simulador?)");
        const Acao acao = jogadores[jogo.daVez()]->decidir(jogo.estado());
        jogo.aplicar(acao);
    }
    return jogo.resultado();
}

Placar jogarSerie(Jogo& jogo, Jogador& j1, Jogador& j2, int partidas, bool alternar) {
    Placar placar;
    for (int i = 0; i < partidas; ++i) {
        switch (jogarPartida(jogo, j1, j2, !alternar || i % 2 == 0)) {
            case Resultado::VITORIA_J1: ++placar.vitorias; break;
            case Resultado::VITORIA_J2: ++placar.derrotas; break;
            case Resultado::EMPATE: ++placar.empates; break;
        }
    }
    return placar;
}
