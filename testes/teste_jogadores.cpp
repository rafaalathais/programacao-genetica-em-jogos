// Testes dos jogadores: árvore, aleatório, limiar, superar e a fábrica criarAdversario.
#include <iostream>

#include "LeitorJSON.h"
#include "gerador.h"
#include "jogador.h"
#include "teste.h"

using nlohmann::json;

static Opcoes ler(const std::string& jogo) {
    return LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_" + jogo + ".json",
                                 "dataset/specs_regras_" + jogo + ".json");
}

int main() {
    const Opcoes pig = ler("pig");
    const Opcoes bj = ler("blackjack");
    const Vocabulario& vp = pig.vocabulario;
    const Vocabulario& vb = bj.vocabulario;
    const Acao C = Acao::CONTINUAR, P = Acao::PARAR;

    // ---- limiar: continua abaixo do valor, para a partir dele ----
    {
        JogadorLimiar l(0, 20);
        VERIFICAR(l.decidir({0, 50, 50}) == C);
        VERIFICAR(l.decidir({19, 99, 0}) == C);
        VERIFICAR(l.decidir({20, 0, 0}) == P);
        VERIFICAR(l.decidir({57, 0, 0}) == P);
        JogadorLimiar meio(1, 5);  // olha só a variável 1
        VERIFICAR(meio.decidir({100, 4, 100}) == C && meio.decidir({0, 5, 0}) == P);
    }

    // ---- superar ----
    {
        JogadorSuperar s(0, 1, 2, 17);  // minha_soma, soma_adv, sou_segundo
        // primeiro: limiar 17, ignora soma_adv
        VERIFICAR(s.decidir({16, 0, 0}) == C && s.decidir({17, 0, 0}) == P && s.decidir({20, 0, 0}) == P);
        // segundo: continua até igualar a soma do outro
        VERIFICAR(s.decidir({15, 19, 1}) == C);
        VERIFICAR(s.decidir({18, 19, 1}) == C);
        VERIFICAR(s.decidir({19, 19, 1}) == P);  // igualou: para (empate vale meio ponto)
        VERIFICAR(s.decidir({20, 19, 1}) == P);
        VERIFICAR(s.decidir({10, 8, 1}) == P);   // já está na frente, mesmo abaixo de 17
    }

    // ---- aleatório: 50%, reproduzível e sem olhar o estado ----
    {
        Aleatorio a(5), b(5);
        JogadorAleatorio ja(a), jb(b);
        int continuar = 0;
        bool iguais = true;
        for (int i = 0; i < 100000; ++i) {
            const Acao x = ja.decidir({i % 100, 0, 0});
            iguais &= (x == jb.decidir({0, 0, 0}));
            continuar += x == C;
        }
        VERIFICAR(iguais);
        VERIFICAR_MSG(std::abs(continuar - 50000) < 1500, continuar);
    }

    // ---- árvore: decide como avaliar() ----
    {
        Aleatorio a(42);
        Gerador g(vp, a);
        const std::vector<Arvore> pop = g.gerarPopulacao(100, 2, 5);
        bool iguais = true;
        for (const Arvore& t : pop) {
            JogadorArvore j(*t);
            for (int k = 0; k < 50; ++k) {
                const Estado e = {a.inteiro(0, 100), a.inteiro(0, 100), a.inteiro(0, 100)};
                iguais &= j.decidir(e) == avaliar(*t, e);
            }
        }
        VERIFICAR(iguais);
    }

    // ---- fábrica com os avaliadores reais ----
    {
        Aleatorio a(1);
        VERIFICAR(pig.avaliador.adversarios.size() == 2 && bj.avaliador.adversarios.size() == 3);
        auto alP = criarAdversario(pig.avaliador.adversarios[0], vp, a);
        auto limP = criarAdversario(pig.avaliador.adversarios[1], vp, a);
        VERIFICAR(dynamic_cast<JogadorAleatorio*>(alP.get()) != nullptr);
        // limiar(pontos_turno, 20): pontos_turno é achado pelo nome
        VERIFICAR(limP->decidir({19, 0, 0}) == C && limP->decidir({20, 0, 0}) == P);
        VERIFICAR(limP->decidir({5, 99, 99}) == C);

        auto limB = criarAdversario(bj.avaliador.adversarios[1], vb, a);
        auto supB = criarAdversario(bj.avaliador.adversarios[2], vb, a);
        VERIFICAR(limB->decidir({16, 20, 1}) == C && limB->decidir({17, 0, 0}) == P);
        VERIFICAR(dynamic_cast<JogadorSuperar*>(supB.get()) != nullptr);
        VERIFICAR(supB->decidir({16, 0, 0}) == C && supB->decidir({17, 0, 0}) == P);
        VERIFICAR(supB->decidir({18, 20, 1}) == C && supB->decidir({20, 20, 1}) == P);
    }

    // ---- variáveis em outra ordem: o adversário continua olhando a variável certa ----
    {
        json spec = teste::lerJson("dataset/specs_regras_pig.json");
        std::swap(spec["variaveis"][0], spec["variaveis"][2]);  // placar_adv, meu_placar, pontos_turno
        const Vocabulario troc = lerVocabulario(spec);
        Aleatorio a(1);
        auto lim = criarAdversario(pig.avaliador.adversarios[1], troc, a);
        VERIFICAR(lim->decidir({0, 0, 19}) == C && lim->decidir({0, 0, 20}) == P);
        VERIFICAR(lim->decidir({99, 99, 5}) == C);

        json specB = teste::lerJson("dataset/specs_regras_blackjack.json");
        std::swap(specB["variaveis"][0], specB["variaveis"][2]);  // sou_segundo, soma_adv, minha_soma
        const Vocabulario trocB = lerVocabulario(specB);
        auto sup = criarAdversario(bj.avaliador.adversarios[2], trocB, a);
        VERIFICAR(sup->decidir({1, 20, 18}) == C && sup->decidir({1, 20, 20}) == P);
        VERIFICAR(sup->decidir({0, 0, 16}) == C && sup->decidir({0, 0, 17}) == P);
    }

    // ---- fábrica: descrições erradas são recusadas ----
    {
        Aleatorio a(1);
        auto adv = [](const json& j) { return j.get<Adversario>(); };
        auto recusa = [&](const json& j, const Vocabulario& v) {
            try {
                criarAdversario(adv(j), v, a);
            } catch (const std::runtime_error& e) {
                return true;
            }
            return false;
        };
        VERIFICAR(recusa({{"tipo", "ganancioso"}}, vp));
        VERIFICAR(recusa({{"tipo", "Limiar"}, {"variavel", "pontos_turno"}, {"valor", 20}}, vp));  // maiúscula
        VERIFICAR(recusa({{"tipo", "limiar"}, {"variavel", "pontos_turno"}}, vp));                 // sem valor
        VERIFICAR(recusa({{"tipo", "limiar"}, {"variavel", "pontos_turno"}, {"valr", 20}}, vp));   // erro de digitação
        VERIFICAR(recusa({{"tipo", "limiar"}, {"variavel", "pontos"}, {"valor", 20}}, vp));       // variável inexistente
        VERIFICAR(recusa({{"tipo", "limiar"}, {"variavel", "pontos_turno"}, {"valor", 101}}, vp));  // fora da faixa
        VERIFICAR(recusa({{"tipo", "limiar"}, {"variavel", "minha_soma"}, {"valor", 17}}, vp));     // variável do outro jogo
        VERIFICAR(recusa({{"tipo", "limiar"}, {"variavel", "minha_soma"}, {"valor", 17}, {"limiar_se_primeiro", 17}}, vb));
        VERIFICAR(recusa({{"tipo", "aleatorio"}, {"valor", 3}}, vp));
        VERIFICAR(recusa({{"tipo", "superar"}}, vb));                                // sem limiar_se_primeiro
        VERIFICAR(recusa({{"tipo", "superar"}, {"limiar_se_primeiro", 17}}, vp));   // pig não tem minha_soma
        VERIFICAR(recusa({{"tipo", "superar"}, {"limiar_se_primeiro", 30}}, vb));   // fora da faixa
        VERIFICAR(!recusa({{"tipo", "superar"}, {"limiar_se_primeiro", 17}}, vb));  // controle: este é válido
        try {
            criarAdversario(adv({{"tipo", "limiar"}, {"variavel", "pontos"}, {"valor", 20}}), vp, a);
        } catch (const std::runtime_error& e) {
            std::cout << "  [info] exemplo de mensagem: " << e.what() << "\n";
        }
    }

    return teste::fim("jogadores");
}
