#include "motor.h"

#include <chrono>
#include <set>
#include <stdexcept>

#include "operadores.h"
#include "selecao.h"

const char* textoDe(MotivoParada motivo) {
    switch (motivo) {
        case MotivoParada::ALVO_ATINGIDO: return "alvo atingido";
        case MotivoParada::SEM_MELHORA: return "sem melhora";
        case MotivoParada::GERACOES_MAX: return "geracoes_max";
    }
    return "?";
}

Motor::Motor(const Parametros& parametros, Gerador& gerador, Avaliador& avaliador)
    : p_(parametros), gerador_(gerador), avaliador_(avaliador) {}

Individuo Motor::novoIndividuo(Arvore arvore) {
    Individuo i;
    i.profundidade = profundidade(*arvore);
    i.nos = tamanho(*arvore);
    i.arvore = std::move(arvore);
    return i;
}

void Motor::avaliarTodos(std::vector<Individuo>& populacao, std::size_t aPartirDe) {
    for (std::size_t k = aPartirDe; k < populacao.size(); ++k)
        populacao[k].avaliacao = avaliador_.avaliar(*populacao[k].arvore);
}

const Avaliacao& Motor::confirmar(const No& arvore, ResultadoEvolucao& r) {
    const std::string texto = paraTexto(arvore, gerador_.vocabulario());
    auto it = confirmadas_.find(texto);
    if (it != confirmadas_.end()) return it->second;
    ++r.confirmacoes;
    return confirmadas_.emplace(texto, avaliador_.avaliar(arvore, p_.parada.partidas_confirmacao)).first->second;
}

void Motor::escolherCampeao(ResultadoEvolucao& r) {
    if (p_.parada.partidas_confirmacao == 0) {
        r.campeao = copiarIndividuo(r.melhor);
        r.campeaoNaEvolucao = r.melhor.avaliacao.fitness;
        return;
    }
    // candidatos: o melhor de todos e as melhores da última geração (sem repetir árvore)
    std::vector<const Individuo*> candidatos = {&r.melhor};
    std::vector<Fitness> fit;
    for (const Individuo& i : r.populacao) fit.push_back(i.avaliacao.fitness);
    for (std::size_t k : melhores(fit, std::min<int>(CANDIDATOS_FINAIS, static_cast<int>(fit.size()))))
        candidatos.push_back(&r.populacao[k]);
    std::set<std::string> vistos;
    const Individuo* campeao = nullptr;
    Avaliacao avaliacaoCampeao;
    for (const Individuo* c : candidatos) {
        if (!vistos.insert(paraTexto(*c->arvore, gerador_.vocabulario())).second) continue;
        const Avaliacao& a = confirmar(*c->arvore, r);
        if (campeao == nullptr || avaliacaoCampeao.fitness < a.fitness) {  // empate: fica o primeiro
            campeao = c;
            avaliacaoCampeao = a;
        }
    }
    r.campeao = copiarIndividuo(*campeao);
    r.campeaoNaEvolucao = campeao->avaliacao.fitness;
    r.campeao.avaliacao = avaliacaoCampeao;
}

std::vector<Individuo> Motor::proximaGeracao(const std::vector<Individuo>& atual, ContadoresOperadores& contadores) {
    Aleatorio& aleatorio = gerador_.aleatorio();
    std::vector<Fitness> fit;
    fit.reserve(atual.size());
    for (const Individuo& i : atual) fit.push_back(i.avaliacao.fitness);

    std::vector<Individuo> nova;
    nova.reserve(static_cast<std::size_t>(p_.populacao));

    // 1. elite: cópia exata das melhores (com o fitness que tinham)
    for (std::size_t k : melhores(fit, p_.selecao.elitismo)) nova.push_back(copiarIndividuo(atual[k]));

    // 2. filhos. Cada sorteio fica na sua própria linha: a ordem dos sorteios
    //    precisa ser sempre a mesma para a semente repetir a evolução.
    while (static_cast<int>(nova.size()) < p_.populacao) {
        const std::size_t i1 = torneio(fit, p_.selecao.torneio_tamanho, aleatorio);
        Arvore filho;
        const bool cruza = aleatorio.chance(p_.cruzamento.probabilidade);
        if (cruza) {
            const std::size_t i2 = torneio(fit, p_.selecao.torneio_tamanho, aleatorio);
            filho = cruzar(*atual[i1].arvore, *atual[i2].arvore, p_, aleatorio, &contadores);
        } else {
            filho = copiar(*atual[i1].arvore);
        }
        const bool muta = aleatorio.chance(p_.mutacao.probabilidade);
        if (muta) filho = mutar(*filho, p_, gerador_, &contadores);
        nova.push_back(novoIndividuo(std::move(filho)));
    }
    return nova;
}

ResultadoEvolucao Motor::evoluir(const AoFimDaGeracao& aoFimDaGeracao) {
    using relogio = std::chrono::steady_clock;
    ResultadoEvolucao r;
    int semMelhora = 0;
    confirmadas_.clear();

    // fecha uma geração: estatísticas, melhor de todos, callback e critérios de parada.
    // Devolve true se a evolução deve parar.
    auto fechar = [&](int g, const std::vector<Individuo>& pop, const ContadoresOperadores& c,
                      relogio::time_point inicio) {
        EstatisticasGeracao e = calcularEstatisticas(g, pop, gerador_.vocabulario());
        e.cruzamentos = c.cruzamentos;
        e.mutacoes = c.mutacoes;
        std::vector<Fitness> fit;
        for (const Individuo& i : pop) fit.push_back(i.avaliacao.fitness);
        const Individuo& melhorDaGeracao = pop[melhores(fit, 1)[0]];
        if (g == 0 || r.melhor.avaliacao.fitness < melhorDaGeracao.avaliacao.fitness) {
            r.melhor = copiarIndividuo(melhorDaGeracao);
            r.geracaoDoMelhor = g;
            semMelhora = 0;
        } else {
            ++semMelhora;
        }

        // alvo: sem confirmação, vale o número medido; com confirmação, só se a
        // reavaliação com partidas novas também chegar lá.
        bool alvo = melhorDaGeracao.avaliacao.fitness.minimo >= p_.parada.winrate_alvo;
        if (alvo && p_.parada.partidas_confirmacao > 0) {
            const std::string texto = paraTexto(*melhorDaGeracao.arvore, gerador_.vocabulario());
            const bool jaRejeitada = confirmadas_.count(texto) == 1 &&
                                     confirmadas_.at(texto).fitness.minimo < p_.parada.winrate_alvo;
            alvo = !jaRejeitada && confirmar(*melhorDaGeracao.arvore, r).fitness.minimo >= p_.parada.winrate_alvo;
            if (!alvo && !jaRejeitada) {
                ++r.rejeitadas;
                ++e.rejeitadas;
            }
        }

        e.segundos = std::chrono::duration<double>(relogio::now() - inicio).count();
        r.historico.push_back(e);
        if (aoFimDaGeracao) aoFimDaGeracao(e, pop);

        if (alvo) {
            r.motivo = MotivoParada::ALVO_ATINGIDO;
            return true;
        }
        if (semMelhora >= p_.parada.geracoes_sem_melhora) {
            r.motivo = MotivoParada::SEM_MELHORA;
            return true;
        }
        if (g >= p_.geracoes_max) {
            r.motivo = MotivoParada::GERACOES_MAX;
            return true;
        }
        return false;
    };

    // geração 0
    auto inicio = relogio::now();
    std::vector<Individuo> pop;
    for (Arvore& a : gerador_.gerarPopulacao(p_.populacao, p_.profundidade_min, p_.profundidade_max))
        pop.push_back(novoIndividuo(std::move(a)));
    avaliarTodos(pop, 0);
    bool parar = fechar(0, pop, ContadoresOperadores{}, inicio);

    for (int g = 1; !parar; ++g) {
        inicio = relogio::now();
        ContadoresOperadores c;
        std::vector<Individuo> nova = proximaGeracao(pop, c);
        if (static_cast<int>(nova.size()) != p_.populacao)
            throw std::logic_error("Motor: a população mudou de tamanho");  // nunca deveria acontecer
        const std::size_t elite = static_cast<std::size_t>(p_.selecao.elitismo);
        avaliarTodos(nova, REAVALIAR_ELITE ? 0 : elite);
        pop = std::move(nova);
        parar = fechar(g, pop, c, inicio);
    }
    r.populacao = std::move(pop);
    escolherCampeao(r);
    return r;
}
