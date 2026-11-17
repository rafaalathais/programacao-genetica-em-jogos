#include "avaliador.h"

#include <algorithm>

#include "configuracao.h"  // validarCriterio

Avaliador::Avaliador(const CriterioAvaliador& criterio, const Vocabulario& vocab, Jogo& jogo, Aleatorio& aleatorio)
    : criterio_(criterio), jogo_(jogo) {
    validarCriterio(criterio_);  // o LeitorJSON já confere, mas o Avaliador pode ser montado à mão (testes)
    for (const Adversario& a : criterio_.adversarios) adversarios_.push_back(criarAdversario(a, vocab, aleatorio));
}

Avaliacao Avaliador::avaliar(const No& arvore) {
    JogadorArvore jogador(arvore);
    Avaliacao r;
    double soma = 0.0;
    double minimo = 1.0;
    for (const auto& adversario : adversarios_) {
        const Placar p = jogarSerie(jogo_, jogador, *adversario, criterio_.partidas_por_arvore,
                                    criterio_.alternar_quem_comeca);
        r.placares.push_back(p);
        soma += p.winrate();
        minimo = std::min(minimo, p.winrate());
    }
    r.fitness.minimo = minimo;
    r.fitness.media = soma / static_cast<double>(adversarios_.size());
    return r;
}

std::vector<Avaliacao> Avaliador::avaliar(const std::vector<Arvore>& populacao) {
    std::vector<Avaliacao> resultado;
    resultado.reserve(populacao.size());
    for (const Arvore& arvore : populacao) resultado.push_back(avaliar(*arvore));
    return resultado;
}
