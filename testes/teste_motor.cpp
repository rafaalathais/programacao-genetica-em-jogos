// Testes do motor evolutivo e das estatísticas.
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <set>
#include <sstream>

#include "LeitorJSON.h"
#include "blackjack.h"
#include "motor.h"
#include "pig.h"
#include "selecao.h"
#include "teste.h"

static Opcoes ler(const std::string& jogo) {
    return LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_" + jogo + ".json",
                                 "dataset/specs_regras_" + jogo + ".json");
}

// Monta tudo com um Aleatorio só e roda a evolução.
// Para os testes ficarem rápidos: população pequena e poucas partidas por adversário.
template <typename J>
static ResultadoEvolucao rodar(const Opcoes& o, const Parametros& p, int partidas, std::uint32_t semente,
                               const Motor::AoFimDaGeracao& cb = nullptr) {
    Aleatorio a(semente);
    J jogo(o.regras, o.vocabulario, a);
    CriterioAvaliador c = o.avaliador;
    c.partidas_por_arvore = partidas;
    Avaliador av(c, o.vocabulario, jogo, a);
    Gerador g(o.vocabulario, a);
    Motor m(p, g, av);
    return m.evoluir(cb);
}

static Parametros pequeno(const Opcoes& o, int populacao, int geracoes) {
    Parametros p = o.parametros;
    p.populacao = populacao;
    p.geracoes_max = geracoes;
    p.parada.winrate_alvo = 1.0;           // inalcançável: não para por alvo
    p.parada.geracoes_sem_melhora = 1000;  // não para por falta de melhora
    p.parada.partidas_confirmacao = 0;     // sem confirmação (testada à parte, mais abaixo)
    return p;
}

static std::vector<Fitness> fitnessDe(const std::vector<Individuo>& pop) {
    std::vector<Fitness> f;
    for (const Individuo& i : pop) f.push_back(i.avaliacao.fitness);
    return f;
}

static bool mesmasEstatisticas(const EstatisticasGeracao& a, const EstatisticasGeracao& b) {
    return a.geracao == b.geracao && a.individuos == b.individuos && a.melhor == b.melhor &&
           a.winratesMelhor == b.winratesMelhor && a.nosMelhor == b.nosMelhor &&
           a.profundidadeMelhor == b.profundidadeMelhor && a.mediaMinimo == b.mediaMinimo &&
           a.mediaMedia == b.mediaMedia && a.nosMedio == b.nosMedio && a.nosMax == b.nosMax &&
           a.profundidadeMedia == b.profundidadeMedia && a.profundidadeMax == b.profundidadeMax &&
           a.distintas == b.distintas && a.cruzamentos == b.cruzamentos && a.mutacoes == b.mutacoes;
}

int main() {
    const Opcoes bj = ler("blackjack");
    const Opcoes pig = ler("pig");
    const Vocabulario& vb = bj.vocabulario;

    // ---- regras rígidas, conferidas a cada geração ----
    {
        Parametros p = pequeno(bj, 40, 8);
        bool tamanhoOk = true, limiteOk = true, camposOk = true, eliteOk = true;
        std::vector<std::string> eliteAnterior;
        int geracoes = 0;
        rodar<BlackjackDados>(bj, p, 100, 1, [&](const EstatisticasGeracao& e, const std::vector<Individuo>& pop) {
            ++geracoes;
            tamanhoOk &= static_cast<int>(pop.size()) == p.populacao && e.individuos == p.populacao;
            for (const Individuo& i : pop) {
                limiteOk &= i.profundidade <= p.profundidade_limite_filhos;
                camposOk &= i.profundidade == profundidade(*i.arvore) && i.nos == tamanho(*i.arvore) &&
                            validar(*i.arvore, vb).empty() && i.avaliacao.placares.size() == 3;
            }
            // a elite da geração anterior abre esta geração, sem nenhuma alteração e na mesma ordem
            for (std::size_t k = 0; k < eliteAnterior.size(); ++k) eliteOk &= paraTexto(*pop[k].arvore, vb) == eliteAnterior[k];
            eliteAnterior.clear();
            for (std::size_t k : melhores(fitnessDe(pop), p.selecao.elitismo)) eliteAnterior.push_back(paraTexto(*pop[k].arvore, vb));
        });
        VERIFICAR(geracoes == 9);  // geração 0 + 8
        VERIFICAR(tamanhoOk);
        VERIFICAR(limiteOk);
        VERIFICAR(camposOk);
        VERIFICAR(eliteOk);
    }

    // ---- mesma semente, mesma evolução; outra semente, outra ----
    {
        Parametros p = pequeno(bj, 30, 6);
        const ResultadoEvolucao x = rodar<BlackjackDados>(bj, p, 100, 5);
        const ResultadoEvolucao y = rodar<BlackjackDados>(bj, p, 100, 5);
        const ResultadoEvolucao z = rodar<BlackjackDados>(bj, p, 100, 6);
        bool iguais = x.historico.size() == y.historico.size();
        for (std::size_t g = 0; iguais && g < x.historico.size(); ++g) iguais = mesmasEstatisticas(x.historico[g], y.historico[g]);
        VERIFICAR(iguais);
        VERIFICAR(paraTexto(*x.melhor.arvore, vb) == paraTexto(*y.melhor.arvore, vb));
        bool popIgual = true;
        for (std::size_t k = 0; k < x.populacao.size(); ++k)
            popIgual &= paraTexto(*x.populacao[k].arvore, vb) == paraTexto(*y.populacao[k].arvore, vb);
        VERIFICAR(popIgual);
        VERIFICAR(!mesmasEstatisticas(x.historico.back(), z.historico.back()));
    }

    // ---- critérios de parada ----
    {
        Parametros p = pequeno(bj, 20, 50);
        p.parada.winrate_alvo = 0.0;  // qualquer árvore já alcança
        const ResultadoEvolucao a = rodar<BlackjackDados>(bj, p, 50, 2);
        VERIFICAR(a.motivo == MotivoParada::ALVO_ATINGIDO && a.historico.size() == 1);

        p = pequeno(bj, 20, 3);
        const ResultadoEvolucao b = rodar<BlackjackDados>(bj, p, 50, 2);
        VERIFICAR(b.motivo == MotivoParada::GERACOES_MAX && b.historico.size() == 4);  // gerações 0..3
        VERIFICAR(b.historico.back().geracao == 3);

        // sem melhora: refaz a contagem a partir do histórico e confere que parou na hora certa
        p = pequeno(bj, 20, 200);
        p.parada.geracoes_sem_melhora = 3;
        const ResultadoEvolucao c = rodar<BlackjackDados>(bj, p, 50, 3);
        VERIFICAR(c.motivo == MotivoParada::SEM_MELHORA);
        Fitness recorde = c.historico[0].melhor;
        int contador = 0, paraEm = -1, geracaoRecorde = 0;
        for (std::size_t g = 1; g < c.historico.size() && paraEm < 0; ++g) {
            if (recorde < c.historico[g].melhor) {
                recorde = c.historico[g].melhor;
                geracaoRecorde = static_cast<int>(g);
                contador = 0;
            } else if (++contador >= 3) {
                paraEm = static_cast<int>(g);
            }
        }
        VERIFICAR_MSG(paraEm == static_cast<int>(c.historico.size()) - 1, paraEm << " x " << c.historico.size());
        // o melhor de todos é o recorde, achado na geração certa
        VERIFICAR(c.melhor.avaliacao.fitness == recorde && c.geracaoDoMelhor == geracaoRecorde);
        VERIFICAR(paraTexto(*c.melhor.arvore, vb).size() > 0);
    }

    // ---- probabilidades por filho ----
    {
        // sem cruzamento nem mutação: a geração nova só tem cópias de árvores da anterior
        Parametros p = pequeno(bj, 30, 4);
        p.cruzamento.probabilidade = 0;
        p.mutacao.probabilidade = 0;
        std::set<std::string> anterior;
        bool soCopias = true, contadoresZero = true;
        rodar<BlackjackDados>(bj, p, 50, 4, [&](const EstatisticasGeracao& e, const std::vector<Individuo>& pop) {
            std::set<std::string> atual;
            for (const Individuo& i : pop) atual.insert(paraTexto(*i.arvore, vb));
            if (e.geracao > 0)
                for (const std::string& t : atual) soCopias &= anterior.count(t) == 1;
            contadoresZero &= e.cruzamentos == 0 && e.mutacoes == 0;
            anterior = atual;
        });
        VERIFICAR(soCopias);
        VERIFICAR(contadoresZero);

        // sempre cruza / sempre muta: um operador por filho (a elite não passa por eles)
        p.cruzamento.probabilidade = 1;
        bool todosCruzam = true;
        rodar<BlackjackDados>(bj, p, 50, 4, [&](const EstatisticasGeracao& e, const std::vector<Individuo>&) {
            if (e.geracao > 0) todosCruzam &= e.cruzamentos == p.populacao - p.selecao.elitismo && e.mutacoes == 0;
        });
        VERIFICAR(todosCruzam);
        p.cruzamento.probabilidade = 0;
        p.mutacao.probabilidade = 1;
        bool todosMutam = true;
        rodar<BlackjackDados>(bj, p, 50, 4, [&](const EstatisticasGeracao& e, const std::vector<Individuo>&) {
            if (e.geracao > 0) todosMutam &= e.mutacoes == p.populacao - p.selecao.elitismo && e.cruzamentos == 0;
        });
        VERIFICAR(todosMutam);

        // com os valores do parametros.json (0.85 e 0.05): perto disso, mas sorteado por filho
        p = pequeno(bj, 60, 10);
        long filhos = 0, cx = 0, mu = 0;
        rodar<BlackjackDados>(bj, p, 50, 7, [&](const EstatisticasGeracao& e, const std::vector<Individuo>&) {
            if (e.geracao == 0) return;
            filhos += p.populacao - p.selecao.elitismo;
            cx += e.cruzamentos;
            mu += e.mutacoes;
        });
        const double fc = static_cast<double>(cx) / filhos, fm = static_cast<double>(mu) / filhos;
        VERIFICAR_MSG(std::abs(fc - 0.85) < 0.05, fc);
        VERIFICAR_MSG(std::abs(fm - 0.05) < 0.03, fm);
        std::cout << "  [info] " << filhos << " filhos: cruzaram " << 100 * fc << "%, mutaram " << 100 * fm << "%\n";
    }

    // ---- a elite é reavaliada (a árvore não muda, o número medido sim) ----
    {
        Parametros p = pequeno(bj, 20, 6);
        std::vector<std::string> textoElite;
        std::vector<Fitness> fitElite;
        int mudou = 0;
        rodar<BlackjackDados>(bj, p, 50, 8, [&](const EstatisticasGeracao&, const std::vector<Individuo>& pop) {
            for (std::size_t k = 0; k < textoElite.size(); ++k)
                if (paraTexto(*pop[k].arvore, vb) == textoElite[k] && !(pop[k].avaliacao.fitness == fitElite[k])) ++mudou;
            textoElite.clear();
            fitElite.clear();
            for (std::size_t k : melhores(fitnessDe(pop), p.selecao.elitismo)) {
                textoElite.push_back(paraTexto(*pop[k].arvore, vb));
                fitElite.push_back(pop[k].avaliacao.fitness);
            }
        });
        VERIFICAR(Motor::REAVALIAR_ELITE);
        VERIFICAR(mudou > 0);
    }

    // ---- a seleção funciona: a população melhora (nos dois jogos) ----
    {
        const ResultadoEvolucao b = rodar<BlackjackDados>(bj, pequeno(bj, 40, 10), 100, 11);
        const ResultadoEvolucao q = rodar<Pig>(pig, pequeno(pig, 40, 10), 100, 11);
        const double db = b.historico.back().mediaMinimo - b.historico.front().mediaMinimo;
        const double dq = q.historico.back().mediaMinimo - q.historico.front().mediaMinimo;
        VERIFICAR_MSG(db > 0.1, db);
        VERIFICAR_MSG(dq > 0.1, dq);
        std::cout << "  [info] média do pior winrate, geração 0 -> 10: blackjack "
                  << b.historico.front().mediaMinimo << " -> " << b.historico.back().mediaMinimo << ", pig "
                  << q.historico.front().mediaMinimo << " -> " << q.historico.back().mediaMinimo << "\n";
    }

    // ---- estatísticas de uma população montada à mão ----
    {
        auto ind = [](Arvore a, double mn, double md) {
            Individuo i;
            i.profundidade = profundidade(*a);
            i.nos = tamanho(*a);
            i.arvore = std::move(a);
            i.avaliacao.fitness = {mn, md};
            Placar pl;
            pl.vitorias = 1;
            i.avaliacao.placares = {pl};
            return i;
        };
        auto se = [](int c) {
            return criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarVariavel(0), criarConstante(c)),
                           criarAcao(Acao::PARAR), criarAcao(Acao::CONTINUAR));
        };
        std::vector<Individuo> pop;
        pop.push_back(ind(se(17), 0.4, 0.5));
        pop.push_back(ind(se(17), 0.4, 0.7));  // mesma árvore, média maior: é o melhor
        pop.push_back(ind(criarAcao(Acao::PARAR), 0.1, 0.3));
        const EstatisticasGeracao e = calcularEstatisticas(4, pop, vb);
        VERIFICAR(e.geracao == 4 && e.individuos == 3);
        VERIFICAR(e.melhor == (Fitness{0.4, 0.7}));
        VERIFICAR(e.distintas == 2);
        VERIFICAR(e.nosMax == 6 && e.nosMelhor == 6 && e.profundidadeMax == 2);
        VERIFICAR(std::abs(e.nosMedio - 13.0 / 3) < 1e-12 && std::abs(e.mediaMinimo - 0.3) < 1e-12);
        VERIFICAR(e.winratesMelhor.size() == 1 && e.winratesMelhor[0] == 1.0);
    }

    // ---- CSV ----
    {
        const ResultadoEvolucao r = rodar<BlackjackDados>(bj, pequeno(bj, 20, 3), 50, 9);
        const std::string caminho = (std::filesystem::temp_directory_path() / "pg_evolucao.csv").string();
        escreverCsv(caminho, r.historico, {"aleatorio", "limiar", "superar"});
        std::ifstream f(caminho);
        std::string linha;
        std::vector<std::string> linhas;
        while (std::getline(f, linha)) linhas.push_back(linha);
        std::filesystem::remove(caminho);
        auto colunas = [](const std::string& s) { return static_cast<int>(std::count(s.begin(), s.end(), ',')) + 1; };
        VERIFICAR(linhas.size() == r.historico.size() + 1);
        VERIFICAR(linhas[0].rfind("geracao,individuos,melhor_minimo,melhor_media,melhor_w_aleatorio", 0) == 0);
        bool mesmasColunas = true;
        for (const std::string& l : linhas) mesmasColunas &= colunas(l) == colunas(linhas[0]);
        // 4 + 3 adversários + 13 colunas da população, dos operadores e da confirmação
        VERIFICAR(mesmasColunas && colunas(linhas[0]) == 20);
        VERIFICAR(linhas[0].find(",rejeitadas,segundos") != std::string::npos);
        VERIFICAR(linhas[1].rfind("0,20,", 0) == 0 && linhas.back().rfind("3,20,", 0) == 0);
        VERIFICAR_LANCA(std::runtime_error, escreverCsv("/pasta/que/nao/existe/x.csv", r.historico, {}));
    }

    // ---- reavaliação: separar as árvores boas das sortudas ----
    {
        // Avaliador com outro número de partidas
        Aleatorio a(1);
        BlackjackDados jogo(bj.regras, vb, a);
        Avaliador av(bj.avaliador, vb, jogo, a);
        const Arvore l17 = criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarVariavel(0), criarConstante(17)),
                                   criarAcao(Acao::PARAR), criarAcao(Acao::CONTINUAR));
        const Avaliacao x = av.avaliar(*l17, 1234 * 2);
        VERIFICAR(x.placares.size() == 3 && x.placares[0].partidas() == 2468 && x.placares[2].partidas() == 2468);
        VERIFICAR_LANCA(std::invalid_argument, av.avaliar(*l17, 0));
    }
    {
        // Com 50 partidas o número medido tem muita sorte: sem confirmação, várias rodadas
        // param por "alvo" com árvores que não valem isso; com confirmação, nenhuma.
        int paraSem = 0, paraCom = 0, rejeitadas = 0;
        bool somaOk = true, cacheOk = true, campeaoOk = true, semConfOk = true;
        for (std::uint32_t s = 1; s <= 10; ++s) {
            Parametros p = pequeno(bj, 20, 6);
            p.parada.winrate_alvo = 0.52;  // o teto real contra o superar é ~51,5%
            const ResultadoEvolucao sem = rodar<BlackjackDados>(bj, p, 50, s);
            paraSem += sem.motivo == MotivoParada::ALVO_ATINGIDO;
            // sem confirmação, o campeão é o melhor medido
            semConfOk &= sem.confirmacoes == 0 && sem.rejeitadas == 0 &&
                         paraTexto(*sem.campeao.arvore, vb) == paraTexto(*sem.melhor.arvore, vb) &&
                         sem.campeao.avaliacao.fitness == sem.melhor.avaliacao.fitness;

            p.parada.partidas_confirmacao = 4000;
            std::set<std::string> candidatas;  // melhores da geração que mediram >= alvo
            const ResultadoEvolucao com = rodar<BlackjackDados>(
                bj, p, 50, s, [&](const EstatisticasGeracao& e, const std::vector<Individuo>& pop) {
                    if (e.melhor.minimo < p.parada.winrate_alvo) return;
                    candidatas.insert(paraTexto(*pop[melhores(fitnessDe(pop), 1)[0]].arvore, vb));
                });
            paraCom += com.motivo == MotivoParada::ALVO_ATINGIDO;
            rejeitadas += com.rejeitadas;
            int soma = 0;
            for (const EstatisticasGeracao& e : com.historico) soma += e.rejeitadas;
            somaOk &= soma == com.rejeitadas;
            // cada árvore é reavaliada uma vez só: rejeitadas = candidatas distintas
            if (com.motivo != MotivoParada::ALVO_ATINGIDO) cacheOk &= com.rejeitadas == static_cast<int>(candidatas.size());
            // o campeão vem da reavaliação (4000 partidas por adversário)
            campeaoOk &= com.campeao.avaliacao.placares.size() == 3 &&
                         com.campeao.avaliacao.placares[0].partidas() == 4000 && validar(*com.campeao.arvore, vb).empty() &&
                         com.confirmacoes >= 1;
        }
        VERIFICAR_MSG(paraSem >= 3, paraSem);
        VERIFICAR_MSG(paraCom == 0, paraCom);
        VERIFICAR(rejeitadas > 0);
        VERIFICAR(somaOk);
        VERIFICAR(cacheOk);
        VERIFICAR(campeaoOk);
        VERIFICAR(semConfOk);
        std::cout << "  [info] alvo 52%, 50 partidas: pararam por alvo " << paraSem << "/10 sem confirmação, "
                  << paraCom << "/10 com confirmação (" << rejeitadas << " candidatas sortudas barradas)\n";
    }
    {
        // Uma árvore boa de verdade passa: alvo baixo, confirmado já na geração 0.
        Parametros p = pequeno(bj, 20, 6);
        p.parada.winrate_alvo = 0.30;
        p.parada.partidas_confirmacao = 2000;
        const ResultadoEvolucao r = rodar<BlackjackDados>(bj, p, 50, 3);
        VERIFICAR(r.motivo == MotivoParada::ALVO_ATINGIDO && r.historico.size() == 1 && r.rejeitadas == 0);
        VERIFICAR(r.campeao.avaliacao.fitness.minimo >= 0.30);
        // o campeão é o melhor entre os candidatos pela reavaliação, não pelo número medido
        VERIFICAR(r.campeao.avaliacao.placares[0].partidas() == 2000);
    }

    return teste::fim("motor");
}
