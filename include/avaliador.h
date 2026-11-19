#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_AVALIADOR_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_AVALIADOR_H

#include <memory>
#include <string>
#include <vector>

#include "aleatorio.h"
#include "arvore.h"
#include "criterioAvaliador.h"
#include "fitness.h"
#include "jogador.h"
#include "jogo.h"
#include "partida.h"
#include "vocabulario.h"

// Resultado da avaliação de uma árvore.
struct Avaliacao {
    Fitness fitness;               // (pior winrate, média dos winrates)
    std::vector<Placar> placares;  // um por adversário, na ordem do json, do ponto de vista da árvore
};

// Avaliador: mede quão boa é uma árvore jogando contra os adversários do avaliador_*.json.
//
// Para cada adversário i, joga partidas_por_arvore partidas (alternando quem começa) e calcula
//     wᵢ = (vitórias + 0.5 * empates) / partidas
// e o fitness "minimo_com_desempate_pela_media":
//     minimo = menor wᵢ   (o adversário contra quem a árvore vai pior)
//     media  = média dos wᵢ
//
// Não conhece nenhum jogo: recebe um Jogo& já montado (Pig, BlackjackDados...).
// Os adversários são montados uma vez, no construtor, e reaproveitados.
// Tudo sorteia com o Aleatorio único, na mesma ordem sempre: mesma semente, mesmo fitness.
class Avaliador {
public:
    // Lança std::runtime_error se o critério ou algum adversário for inválido.
    Avaliador(const CriterioAvaliador& criterio, const Vocabulario& vocab, Jogo& jogo, Aleatorio& aleatorio);

    Avaliacao avaliar(const No& arvore);

    // O mesmo, com outro número de partidas por adversário (ex.: a confirmação do
    // motor, com 20000). Lança std::invalid_argument se partidas < 1.
    Avaliacao avaliar(const No& arvore, int partidas);

    // Avalia a população inteira, na ordem do vetor.
    std::vector<Avaliacao> avaliar(const std::vector<Arvore>& populacao);

    std::size_t numAdversarios() const { return adversarios_.size(); }
    const Adversario& adversario(std::size_t i) const { return criterio_.adversarios.at(i); }
    const CriterioAvaliador& criterio() const { return criterio_; }

private:
    CriterioAvaliador criterio_;
    Jogo& jogo_;
    std::vector<std::unique_ptr<Jogador>> adversarios_;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_AVALIADOR_H
