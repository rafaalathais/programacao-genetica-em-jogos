// Testes do Avaliador e da validação do avaliador_*.json.
#include <cmath>
#include <filesystem>
#include <fstream>
#include <iostream>

#include "LeitorJSON.h"
#include "avaliador.h"
#include "blackjack.h"
#include "configuracao.h"
#include "gerador.h"
#include "pig.h"
#include "teste.h"

using nlohmann::json;

static Opcoes ler(const std::string& jogo) {
    return LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_" + jogo + ".json",
                                 "dataset/specs_regras_" + jogo + ".json");
}

// Árvore SE(var op c, entao, senao).
static Arvore se(int var, Comparacao op, int c, Acao entao, Acao senao) {
    return criarSe(criarComparacao(op, criarVariavel(var), criarConstante(c)), criarAcao(entao), criarAcao(senao));
}

// Jogo "espião": repassa tudo para o jogo de verdade e anota quem começou cada partida.
struct Espiao : Jogo {
    Jogo& real;
    int comecouJ1 = 0, comecouJ2 = 0;
    explicit Espiao(Jogo& j) : real(j) {}
    void reiniciar(int primeiro) override {
        (primeiro == 0 ? comecouJ1 : comecouJ2)++;
        real.reiniciar(primeiro);
    }
    bool terminou() const override { return real.terminou(); }
    int daVez() const override { return real.daVez(); }
    const Estado& estado() override { return real.estado(); }
    void aplicar(Acao a) override { real.aplicar(a); }
    Resultado resultado() const override { return real.resultado(); }
};

int main() {
    const Opcoes pig = ler("pig");
    const Opcoes bj = ler("blackjack");
    const Acao C = Acao::CONTINUAR, P = Acao::PARAR;

    // ---- validação do critério ----
    validarCriterio(pig.avaliador);
    validarCriterio(bj.avaliador);
    {
        auto com = [&](auto f) { CriterioAvaliador c = pig.avaliador; f(c); return c; };
        VERIFICAR_LANCA(std::runtime_error, validarCriterio(com([](CriterioAvaliador& c) { c.partidas_por_arvore = 0; })));
        VERIFICAR_LANCA(std::runtime_error, validarCriterio(com([](CriterioAvaliador& c) { c.partidas_por_arvore = 501; })));
        VERIFICAR_LANCA(std::runtime_error, validarCriterio(com([](CriterioAvaliador& c) { c.fitness = "media"; })));
        VERIFICAR_LANCA(std::runtime_error, validarCriterio(com([](CriterioAvaliador& c) { c.adversarios.clear(); })));
        VERIFICAR_LANCA(std::runtime_error, validarCriterio(com([](CriterioAvaliador& c) { c.adversarios[0].tipo = "x"; })));
        // ímpar é aceito quando não se alterna quem começa
        validarCriterio(com([](CriterioAvaliador& c) {
            c.partidas_por_arvore = 501;
            c.alternar_quem_comeca = false;
        }));
        // o LeitorJSON também recusa (e cita o arquivo)
        json j = teste::lerJson("dataset/avaliador_pig.json");
        j["partidas_por_arvore"] = 499;
        const std::string caminho = (std::filesystem::temp_directory_path() / "pg_avaliador_impar.json").string();
        std::ofstream(caminho) << j.dump();
        std::string msg;
        try {
            LeitorJSON::lerOpcoes("dataset/parametros.json", caminho, "dataset/specs_regras_pig.json");
        } catch (const std::runtime_error& e) {
            msg = e.what();
        }
        std::filesystem::remove(caminho);
        VERIFICAR_MSG(msg.find(caminho) != std::string::npos && msg.find("par") != std::string::npos, msg);
    }

    // ---- Pig: árvore igual ao limiar 20 ----
    {
        Aleatorio a(42);
        Pig jogo(pig.regras, pig.vocabulario, a);
        Avaliador av(pig.avaliador, pig.vocabulario, jogo, a);
        VERIFICAR(av.numAdversarios() == 2);
        const Arvore limiar20 = se(0, Comparacao::MAIOR_IGUAL, 20, P, C);  // SE(pontos_turno >= 20, PARAR, CONTINUAR)
        const Avaliacao r = av.avaliar(*limiar20);
        VERIFICAR(r.placares.size() == 2);
        VERIFICAR(r.placares[0].partidas() == 500 && r.placares[1].partidas() == 500);
        const double w0 = r.placares[0].winrate(), w1 = r.placares[1].winrate();
        VERIFICAR(r.fitness.minimo == std::min(w0, w1));
        VERIFICAR(r.fitness.media == (w0 + w1) / 2);
        VERIFICAR_MSG(w0 > 0.8, w0);                        // contra o aleatório vence quase sempre
        VERIFICAR_MSG(std::abs(w1 - 0.5) < 0.08, w1);      // contra ele mesmo, perto de 50%
        std::cout << "  [info] pig, árvore = limiar 20: x aleatório " << w0 << ", x limiar " << w1 << "\n";

        // sempre PARAR: perde para os dois -> fitness baixo
        const Avaliacao ruim = av.avaliar(*criarAcao(P));
        VERIFICAR(ruim.fitness < r.fitness);
    }

    // ---- Blackjack: a árvore que para logo nunca empata com o limiar ----
    {
        Aleatorio a(42);
        BlackjackDados jogo(bj.regras, bj.vocabulario, a);
        Avaliador av(bj.avaliador, bj.vocabulario, jogo, a);
        VERIFICAR(av.numAdversarios() == 3);
        const Avaliacao r = av.avaliar(*criarAcao(P));
        // sempre para com 2..12; o limiar só para com 17+ (ou estoura): empate impossível
        VERIFICAR(r.placares[1].empates == 0);
        VERIFICAR(r.placares[2].empates > 0);  // o superar, em segundo, para ao igualar
        const Arvore l17 = se(0, Comparacao::MAIOR_IGUAL, 17, P, C);
        const Avaliacao rl = av.avaliar(*l17);
        VERIFICAR(r.fitness < rl.fitness);
    }

    // ---- quem começa: alterna (250/250) ou a árvore começa todas ----
    {
        Aleatorio a(1);
        Pig real(pig.regras, pig.vocabulario, a);
        Espiao esp(real);
        Avaliador av(pig.avaliador, pig.vocabulario, esp, a);
        av.avaliar(*criarAcao(P));
        VERIFICAR_MSG(esp.comecouJ1 == 500 && esp.comecouJ2 == 500, esp.comecouJ1 << "/" << esp.comecouJ2);
        CriterioAvaliador c = pig.avaliador;
        c.alternar_quem_comeca = false;
        Espiao esp2(real);
        Avaliador av2(c, pig.vocabulario, esp2, a);
        av2.avaliar(*criarAcao(P));
        VERIFICAR(esp2.comecouJ1 == 1000 && esp2.comecouJ2 == 0);
    }

    // ---- população: mesma semente, mesmo fitness; e igual a avaliar uma por uma ----
    {
        auto avaliarPop = [&](std::uint32_t semente) {
            Aleatorio a(semente);
            Gerador g(pig.vocabulario, a);
            const std::vector<Arvore> pop = g.gerarPopulacao(30, 2, 5);
            Pig jogo(pig.regras, pig.vocabulario, a);
            Avaliador av(pig.avaliador, pig.vocabulario, jogo, a);
            return av.avaliar(pop);
        };
        const std::vector<Avaliacao> x = avaliarPop(7), y = avaliarPop(7), z = avaliarPop(8);
        bool iguais = x.size() == 30, diferentes = false, faixa = true;
        for (std::size_t i = 0; i < x.size(); ++i) {
            iguais &= x[i].fitness == y[i].fitness;
            diferentes |= !(x[i].fitness == z[i].fitness);
            faixa &= x[i].fitness.minimo >= 0 && x[i].fitness.minimo <= x[i].fitness.media && x[i].fitness.media <= 1;
        }
        VERIFICAR(iguais);
        VERIFICAR(diferentes);
        VERIFICAR(faixa);

        Aleatorio a(7);
        Gerador g(pig.vocabulario, a);
        const std::vector<Arvore> pop = g.gerarPopulacao(30, 2, 5);
        Pig jogo(pig.regras, pig.vocabulario, a);
        Avaliador av(pig.avaliador, pig.vocabulario, jogo, a);
        bool umaPorUma = true;
        for (std::size_t i = 0; i < pop.size(); ++i) umaPorUma &= av.avaliar(*pop[i]).fitness == x[i].fitness;
        VERIFICAR(umaPorUma);
    }

    // ---- adversário inválido derruba o Avaliador na construção ----
    {
        Aleatorio a(1);
        Pig jogo(pig.regras, pig.vocabulario, a);
        CriterioAvaliador c = pig.avaliador;
        c.adversarios[1].variavel = "minha_soma";  // variável do blackjack
        VERIFICAR_LANCA(std::runtime_error, Avaliador(c, pig.vocabulario, jogo, a));
    }

    return teste::fim("avaliador");
}
