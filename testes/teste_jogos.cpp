// Testes dos simuladores (Pig, BlackjackDados) e da Partida.
#include <cmath>
#include <iostream>
#include <vector>

#include "LeitorJSON.h"
#include "blackjack.h"
#include "partida.h"
#include "pig.h"
#include "teste.h"

using nlohmann::json;

// Jogador que decide sempre a mesma coisa.
struct Sempre : Jogador {
    Acao acao;
    explicit Sempre(Acao a) : acao(a) {}
    Acao decidir(const Estado&) override { return acao; }
};

static Opcoes ler(const std::string& jogo, const std::string& spec) {
    return LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_" + jogo + ".json",
                                 "dataset/specs_regras_" + spec + ".json");
}

// Lança ao montar o jogo com estas regras/esta spec?
template <typename J>
static bool recusa(const json& regras, const Vocabulario& v) {
    Aleatorio a(1);
    try {
        J jogo(regras, v, a);
    } catch (const std::runtime_error&) {
        return true;
    }
    return false;
}

static bool perto(double x, double alvo, double tol) { return std::abs(x - alvo) < tol; }

static void testarPig() {
    const Opcoes o = ler("pig", "pig");
    const Vocabulario& v = o.vocabulario;
    const int PT = v.indiceDe("pontos_turno"), MP = v.indiceDe("meu_placar"), PA = v.indiceDe("placar_adv");

    // ---- regras ----
    const RegrasPig r = lerRegrasPig(o.regras);
    VERIFICAR(r.pontosParaVencer == 100 && r.faces == 6 && r.faceQueZera == 1 && r.vitoriaImediata);
    auto mudar = [&](auto f) { json j = o.regras; f(j); return j; };
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["dado"]["faces"] = 1; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["dado"]["faces"] = 6.5; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["dado"]["quantidade"] = 2; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["dado"] = 6; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["face_q_zera_turno"] = 7; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["face_q_zera_turno"] = 0; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["pontos_para_vencer"] = 0; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["vitoria_imediata"] = "true"; }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j.erase("vitoria_imediata"); }), v));
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["face_que_zera_turno"] = 1; }), v));  // chave com erro de digitação
    VERIFICAR(recusa<Pig>(mudar([](json& j) { j["dado"]["lados"] = 6; }), v));
    try {
        lerRegrasPig(mudar([](json& j) { j["face_que_zera_turno"] = 1; }));
    } catch (const std::runtime_error& e) {
        std::cout << "  [info] exemplo de mensagem: " << e.what() << "\n";
    }

    // ---- vocabulário: variável faltando ou sobrando ----
    {
        Vocabulario sem = v;
        sem.variaveis[static_cast<std::size_t>(PT)].nome = "pontos";
        VERIFICAR(recusa<Pig>(o.regras, sem));
        Vocabulario mais = v;
        mais.variaveis.push_back({"extra", 0, 1});
        VERIFICAR(recusa<Pig>(o.regras, mais));
    }

    // ---- uso errado ----
    {
        Aleatorio a(1);
        Pig pig(o.regras, v, a);
        VERIFICAR(pig.terminou());  // antes de reiniciar não há partida
        VERIFICAR_LANCA(std::logic_error, pig.aplicar(Acao::CONTINUAR));
        VERIFICAR_LANCA(std::logic_error, pig.resultado());
        VERIFICAR_LANCA(std::invalid_argument, pig.reiniciar(2));
        BlackjackDados bj(teste::lerJson("dataset/specs_regras_blackjack.json").at("regras"),
                          lerVocabulario(teste::lerJson("dataset/specs_regras_blackjack.json")), a);
        VERIFICAR(bj.terminou());
        VERIFICAR_LANCA(std::logic_error, bj.aplicar(Acao::PARAR));
        VERIFICAR_LANCA(std::logic_error, bj.resultado());
        VERIFICAR_LANCA(std::invalid_argument, bj.reiniciar(-1));
    }

    // ---- invariantes em 5000 partidas (aleatório x limiar 20), conduzidas à mão ----
    {
        Aleatorio a(11);
        Pig pig(o.regras, v, a);
        JogadorAleatorio al(a);
        JogadorLimiar l20(PT, 20);
        Jogador* jog[2] = {&al, &l20};
        bool sempreOk = true, pararGuarda = true, fimOk = true;
        int minPontos = 1000;
        for (int p = 0; p < 5000; ++p) {
            pig.reiniciar(p % 2);
            while (!pig.terminou()) {
                const int vez = pig.daVez();
                const Estado e = pig.estado();
                // estado do ponto de vista de quem está na vez
                if (e[static_cast<std::size_t>(MP)] != pig.placar(vez) ||
                    e[static_cast<std::size_t>(PA)] != pig.placar(1 - vez) ||
                    e[static_cast<std::size_t>(PT)] != pig.pontosTurno())
                    sempreOk = false;
                // vitória imediata: ninguém decide já tendo alcançado 100
                if (pig.placar(vez) + pig.pontosTurno() >= 100) sempreOk = false;
                minPontos = std::min(minPontos, pig.pontosTurno());
                const Acao acao = jog[vez]->decidir(e);
                const int guardaria = pig.placar(vez) + pig.pontosTurno();
                pig.aplicar(acao);
                // PARAR guarda exatamente os pontos do turno. Exceção legítima: o outro tira 1
                // na rolagem obrigatória, a vez volta, e a rolagem obrigatória seguinte já dá
                // vitória imediata, somando mais pontos (a partida termina com `vez` vencendo).
                if (acao == Acao::PARAR &&
                    (pig.terminou() ? pig.placar(vez) < guardaria : pig.placar(vez) != guardaria))
                    pararGuarda = false;
            }
            const Resultado res = pig.resultado();
            const int venc = res == Resultado::VITORIA_J1 ? 0 : 1;
            if (res == Resultado::EMPATE || pig.placar(venc) < 100 || pig.placar(1 - venc) >= 100) fimOk = false;
        }
        VERIFICAR(sempreOk);
        VERIFICAR(pararGuarda);
        VERIFICAR(fimOk);
        // rolagem obrigatória: com a face 1 zerando, quem decide tem pelo menos 2 pontos no turno
        VERIFICAR_MSG(minPontos == 2, minPontos);
    }

    // ---- variáveis em outra ordem: o simulador acha pelo nome ----
    {
        json spec = teste::lerJson("dataset/specs_regras_pig.json");
        std::swap(spec["variaveis"][0], spec["variaveis"][2]);  // placar_adv, meu_placar, pontos_turno
        const Vocabulario troc = lerVocabulario(spec);
        Aleatorio a(3);
        Pig pig(o.regras, troc, a);
        pig.reiniciar(0);
        pig.aplicar(Acao::PARAR);
        const Estado& e = pig.estado();
        VERIFICAR(e[2] == pig.pontosTurno() && e[1] == pig.placar(pig.daVez()) && e[0] == pig.placar(1 - pig.daVez()));
    }

    // ---- extremos: sempre PARAR e sempre CONTINUAR terminam ----
    {
        Aleatorio a(5);
        Pig pig(o.regras, v, a);
        Sempre para(Acao::PARAR), para2(Acao::PARAR), cont(Acao::CONTINUAR), cont2(Acao::CONTINUAR);
        const Placar pp = jogarSerie(pig, para, para2, 2000);
        VERIFICAR(pp.partidas() == 2000 && pp.empates == 0);
        long turnos = 0;
        int empatesCont = 0;
        for (int i = 0; i < 2000; ++i) {
            empatesCont += jogarPartida(pig, cont, cont2, i % 2 == 0) == Resultado::EMPATE;
            turnos += pig.turnos();
        }
        std::cout << "  [info] pig sempre CONTINUAR x sempre CONTINUAR: " << turnos / 2000.0
                  << " turnos por partida, empates por limite: " << empatesCont << "\n";
        VERIFICAR(empatesCont == 0);
    }

    // ---- sem vitória imediata: quem nunca para nunca marca -> limite de turnos -> empate ----
    {
        json reg = o.regras;
        reg["vitoria_imediata"] = false;
        Aleatorio a(9);
        Pig pig(reg, v, a);
        Sempre cont(Acao::CONTINUAR), cont2(Acao::CONTINUAR);
        VERIFICAR(jogarPartida(pig, cont, cont2, true) == Resultado::EMPATE);
        VERIFICAR(pig.turnos() == Pig::LIMITE_TURNOS + 1);
        VERIFICAR(pig.placar(0) == 0 && pig.placar(1) == 0);
        // e o limiar, que para, vence normalmente (precisa parar para marcar)
        JogadorLimiar l20(PT, 20);
        bool ok = true, dentroDaFaixa = true;
        for (int i = 0; i < 500; ++i) {
            const Resultado res = jogarPartida(pig, l20, cont, i % 2 == 0);
            if (res != Resultado::VITORIA_J1 || pig.placar(0) < 100) ok = false;
        }
        // estado sempre dentro da faixa, mesmo com pontos do turno passando de 100
        pig.reiniciar(0);
        for (int i = 0; i < 400 && !pig.terminou(); ++i) {
            for (int x : pig.estado()) dentroDaFaixa &= (x >= 0 && x <= 100);
            pig.aplicar(Acao::CONTINUAR);
        }
        VERIFICAR(ok);
        VERIFICAR(dentroDaFaixa);
    }

    // ---- sanidade e reprodutibilidade ----
    {
        Aleatorio a(42);
        Pig pig(o.regras, v, a);
        JogadorAleatorio al(a), al2(a);
        JogadorLimiar l20(PT, 20), l20b(PT, 20);
        const Placar x = jogarSerie(pig, l20, al, 20000);
        const Placar clones = jogarSerie(pig, l20, l20b, 20000);
        const Placar alea = jogarSerie(pig, al, al2, 20000);
        VERIFICAR_MSG(x.winrate() > 0.85, x.winrate());           // "parar em 20" vence o aleatório
        VERIFICAR_MSG(perto(clones.winrate(), 0.5, 0.02), clones.winrate());  // clones empatam perto de 50%
        VERIFICAR_MSG(perto(alea.winrate(), 0.5, 0.02), alea.winrate());
        int comecouVenceu = 0;
        for (int i = 0; i < 20000; ++i) comecouVenceu += jogarPartida(pig, l20, l20b, true) == Resultado::VITORIA_J1;
        std::cout << "  [info] pig: limiar20 x aleatório " << x.winrate() << ", clones " << clones.winrate()
                  << ", aleatório x aleatório " << alea.winrate()
                  << "\n         vantagem de começar (limiar20 x limiar20): " << comecouVenceu / 20000.0 << "\n";

        Aleatorio b1(77), b2(77);
        Pig p1(o.regras, v, b1), p2(o.regras, v, b2);
        JogadorAleatorio a1(b1), a2(b2);
        bool iguais = true;
        for (int i = 0; i < 2000; ++i)
            iguais &= jogarPartida(p1, l20, a1, i % 2 == 0) == jogarPartida(p2, l20, a2, i % 2 == 0) &&
                      p1.placar(0) == p2.placar(0) && p1.placar(1) == p2.placar(1);
        VERIFICAR(iguais);
    }
}

static void testarBlackjack() {
    const Opcoes o = ler("blackjack", "blackjack");
    const Vocabulario& v = o.vocabulario;
    const int MS = v.indiceDe("minha_soma"), SA = v.indiceDe("soma_adv"), S2 = v.indiceDe("sou_segundo");

    // ---- regras ----
    const RegrasBlackjack r = lerRegrasBlackjack(o.regras);
    VERIFICAR(r.limiteEstouro == 21 && r.faces == 6 && r.quantidade == 2 && r.quantidadeReduzida == 1);
    VERIFICAR(r.reduzAPartirDe == 16 && r.rolagemObrigatoria);
    auto mudar = [&](auto f) { json j = o.regras; f(j); return j; };
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j["empate"] = "conta_como_derrota"; }), v));
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j["dado"]["faces"] = 1; }), v));
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j["dado"]["quantidade"] = 0; }), v));
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j["dado"]["reduz_a_partir_de"] = -1; }), v));
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j["rolagem_obrigatoria"] = 1; }), v));
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j.erase("limite_de_estouro"); }), v));
    VERIFICAR(recusa<BlackjackDados>(mudar([](json& j) { j["limite_estouro"] = 21; }), v));
    {
        Vocabulario sem = v;
        sem.variaveis[static_cast<std::size_t>(S2)].nome = "segundo";
        VERIFICAR(recusa<BlackjackDados>(o.regras, sem));
    }

    // ---- invariantes em 20000 partidas, conduzidas à mão ----
    {
        Aleatorio a(13);
        BlackjackDados bj(o.regras, v, a);
        JogadorAleatorio al(a);
        JogadorLimiar l17(MS, 17);
        JogadorSuperar sup(MS, SA, S2, 17);
        Jogador* pares[3][2] = {{&al, &l17}, {&sup, &al}, {&l17, &sup}};
        bool visaoOk = true, somaOk = true, fimOk = true, segundoNaoJoga = true, primeiraRolagem = true;
        int dMin2 = 99, dMax2 = 0, dMin1 = 99, dMax1 = 0, empates = 0, estourosPrimeiro = 0;
        for (int p = 0; p < 20000; ++p) {
            Jogador** jog = pares[p % 3];
            bj.reiniciar(p % 2);
            const int prim = bj.primeiro(), seg = 1 - prim;
            int decisoes[2] = {0, 0};
            while (!bj.terminou()) {
                const int vez = bj.daVez();
                const Estado e = bj.estado();
                const int minha = e[static_cast<std::size_t>(MS)];
                if (minha != bj.soma(vez) || minha > 21) somaOk = false;
                if (decisoes[vez] == 0 && (minha < 2 || minha > 12)) primeiraRolagem = false;  // 2 dados
                if (vez == prim && (e[static_cast<std::size_t>(SA)] != 0 || e[static_cast<std::size_t>(S2)] != 0))
                    visaoOk = false;  // o primeiro não sabe nada do outro
                if (vez == seg &&
                    (e[static_cast<std::size_t>(SA)] != bj.soma(prim) || e[static_cast<std::size_t>(S2)] != 1))
                    visaoOk = false;
                ++decisoes[vez];
                const Acao acao = jog[vez]->decidir(e);
                bj.aplicar(acao);
                if (acao == Acao::CONTINUAR) {
                    const int d = bj.soma(vez) - minha;
                    if (minha < 16) dMin2 = std::min(dMin2, d), dMax2 = std::max(dMax2, d);
                    else dMin1 = std::min(dMin1, d), dMax1 = std::max(dMax1, d);
                }
            }
            const Resultado res = bj.resultado();
            const int s0 = bj.soma(0), s1 = bj.soma(1);
            if (bj.estourou(0) && bj.estourou(1)) fimOk = false;  // nunca os dois
            if (bj.estourou(prim)) {
                ++estourosPrimeiro;
                if (decisoes[seg] != 0 || bj.soma(seg) != 0) segundoNaoJoga = false;
                if (res != (seg == 0 ? Resultado::VITORIA_J1 : Resultado::VITORIA_J2)) fimOk = false;
            } else if (bj.estourou(seg)) {
                if (res != (prim == 0 ? Resultado::VITORIA_J1 : Resultado::VITORIA_J2)) fimOk = false;
            } else {
                const Resultado esperado =
                    s0 > s1 ? Resultado::VITORIA_J1 : s1 > s0 ? Resultado::VITORIA_J2 : Resultado::EMPATE;
                if (res != esperado) fimOk = false;
                empates += res == Resultado::EMPATE;
            }
        }
        VERIFICAR(visaoOk);
        VERIFICAR(somaOk);
        VERIFICAR(primeiraRolagem);
        VERIFICAR(fimOk);
        VERIFICAR(segundoNaoJoga);
        // abaixo de 16 rola 2 dados (2..12); de 16 em diante, 1 dado (1..6)
        VERIFICAR_MSG(dMin2 == 2 && dMax2 == 12, dMin2 << ".." << dMax2);
        VERIFICAR_MSG(dMin1 == 1 && dMax1 == 6, dMin1 << ".." << dMax1);
        VERIFICAR(empates > 0 && estourosPrimeiro > 0);
        std::cout << "  [info] blackjack, 20000 partidas: empates " << empates << ", primeiro estourou "
                  << estourosPrimeiro << "\n";
    }

    // ---- sem rolagem obrigatória: a primeira decisão é com soma 0 ----
    {
        json reg = o.regras;
        reg["rolagem_obrigatoria"] = false;
        Aleatorio a(2);
        BlackjackDados bj(reg, v, a);
        bj.reiniciar(1);
        VERIFICAR(!bj.terminou() && bj.daVez() == 1 && bj.estado()[static_cast<std::size_t>(MS)] == 0);
        bj.aplicar(Acao::PARAR);  // o primeiro para com 0
        VERIFICAR(bj.daVez() == 0 && bj.estado()[static_cast<std::size_t>(SA)] == 0 &&
                  bj.estado()[static_cast<std::size_t>(S2)] == 1);
        bj.aplicar(Acao::PARAR);  // o segundo também: 0 x 0 = empate
        VERIFICAR(bj.terminou() && bj.resultado() == Resultado::EMPATE);
    }

    // ---- sanidade e reprodutibilidade ----
    {
        Aleatorio a(42);
        BlackjackDados bj(o.regras, v, a);
        JogadorAleatorio al(a);
        JogadorLimiar l17(MS, 17), l17b(MS, 17);
        JogadorSuperar sup(MS, SA, S2, 17), sup2(MS, SA, S2, 17);
        const Placar lxa = jogarSerie(bj, l17, al, 20000);
        const Placar clones = jogarSerie(bj, l17, l17b, 20000);
        const Placar sxs = jogarSerie(bj, sup, sup2, 20000);
        const Placar sxl = jogarSerie(bj, sup, l17, 20000);
        VERIFICAR_MSG(lxa.winrate() > 0.65, lxa.winrate());
        VERIFICAR_MSG(perto(clones.winrate(), 0.5, 0.02), clones.winrate());
        VERIFICAR_MSG(perto(sxs.winrate(), 0.5, 0.02), sxs.winrate());
        VERIFICAR_MSG(sxl.winrate() > 0.5, sxl.winrate());  // saber a soma do outro ajuda
        std::cout << "  [info] blackjack: limiar17 x aleatório " << lxa.winrate() << ", clones limiar17 "
                  << clones.winrate() << ", clones superar " << sxs.winrate() << ", superar x limiar17 "
                  << sxl.winrate() << " (empates " << sxl.empates << ")\n";

        Aleatorio b1(77), b2(77);
        BlackjackDados j1(o.regras, v, b1), j2(o.regras, v, b2);
        JogadorAleatorio a1(b1), a2(b2);
        bool iguais = true;
        for (int i = 0; i < 2000; ++i)
            iguais &= jogarPartida(j1, sup, a1, i % 2 == 0) == jogarPartida(j2, sup, a2, i % 2 == 0) &&
                      j1.soma(0) == j2.soma(0) && j1.soma(1) == j2.soma(1);
        VERIFICAR(iguais);
    }
}

static void testarPartida() {
    const Opcoes o = ler("blackjack", "blackjack");
    Aleatorio a(4);
    BlackjackDados bj(o.regras, o.vocabulario, a);
    JogadorLimiar l17(0, 17);
    JogadorAleatorio al(a);
    // quem começa
    jogarPartida(bj, l17, al, true);
    VERIFICAR(bj.primeiro() == 0);
    jogarPartida(bj, l17, al, false);
    VERIFICAR(bj.primeiro() == 1);
    // a série soma o total certo e é igual a alternar à mão
    Aleatorio x(8), y(8);
    BlackjackDados bx(o.regras, o.vocabulario, x), by(o.regras, o.vocabulario, y);
    JogadorAleatorio ax(x), ay(y);
    const Placar s = jogarSerie(bx, l17, ax, 501);
    Placar m;
    for (int i = 0; i < 501; ++i) {
        const Resultado r = jogarPartida(by, l17, ay, i % 2 == 0);
        if (r == Resultado::VITORIA_J1) ++m.vitorias;
        else if (r == Resultado::VITORIA_J2) ++m.derrotas;
        else ++m.empates;
    }
    VERIFICAR(s.partidas() == 501);
    VERIFICAR(s.vitorias == m.vitorias && s.derrotas == m.derrotas && s.empates == m.empates);
    // winrate = (V + 0.5 E) / N
    Placar p;
    p.vitorias = 3;
    p.derrotas = 5;
    p.empates = 2;
    VERIFICAR(p.winrate() == 0.4);
    VERIFICAR(Placar{}.winrate() == 0.0);
}

int main() {
    testarPig();
    testarBlackjack();
    testarPartida();
    return teste::fim("jogos");
}
