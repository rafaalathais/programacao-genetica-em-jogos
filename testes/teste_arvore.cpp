// Testes da árvore: avaliação, medidas, cópia profunda, texto e validação.
#include <set>

#include "arvore.h"
#include "teste.h"

namespace {

// Junta o endereço de todos os nós de uma árvore.
void enderecos(const No& no, std::set<const No*>& saida) {
    saida.insert(&no);
    for (const Arvore& f : no.filhos) enderecos(*f, saida);
}

Arvore se(Comparacao op, int var, int constante, Acao a, Acao b) {
    return criarSe(criarComparacao(op, criarVariavel(var), criarConstante(constante)), criarAcao(a), criarAcao(b));
}

}  // namespace

int main() {
    const Vocabulario pig = lerVocabulario(teste::lerJson("dataset/specs_regras_pig.json"));
    const Vocabulario bj = lerVocabulario(teste::lerJson("dataset/specs_regras_blackjack.json"));
    const int PT = pig.indiceDe("pontos_turno"), MP = pig.indiceDe("meu_placar"), PA = pig.indiceDe("placar_adv");
    const Acao C = Acao::CONTINUAR, P = Acao::PARAR;

    // ---- "parar em 20" ----
    const Arvore p20 = se(Comparacao::MAIOR_IGUAL, PT, 20, P, C);
    VERIFICAR(validar(*p20, pig).empty());
    VERIFICAR(paraTexto(*p20, pig) == "SE(pontos_turno >= 20, PARAR, CONTINUAR)");
    VERIFICAR(profundidade(*p20) == 2);
    VERIFICAR(tamanho(*p20) == 6);
    for (int pts = 0; pts <= 100; ++pts) {
        Estado e(3, 0);
        e[static_cast<std::size_t>(PT)] = pts;
        VERIFICAR_MSG(avaliar(*p20, e) == (pts >= 20 ? P : C), "pts=" << pts);
    }

    // ---- ">" na fronteira: 20 > 20 é falso ----
    const Arvore m20 = se(Comparacao::MAIOR, PT, 20, P, C);
    VERIFICAR(avaliar(*m20, Estado{20, 0, 0}) == C);
    VERIFICAR(avaliar(*m20, Estado{21, 0, 0}) == P);

    // ---- variável com variável ----
    const Arvore vv = criarSe(criarComparacao(Comparacao::MAIOR, criarVariavel(MP), criarVariavel(PA)), criarAcao(P),
                              criarAcao(C));
    VERIFICAR(validar(*vv, pig).empty());
    VERIFICAR(paraTexto(*vv, pig) == "SE(meu_placar > placar_adv, PARAR, CONTINUAR)");
    VERIFICAR(avaliar(*vv, Estado{0, 50, 40}) == P);
    VERIFICAR(avaliar(*vv, Estado{0, 40, 40}) == C);

    // ---- aninhada: SE(placar_adv >= 71, SE(pontos_turno >= 30, ...), SE(pontos_turno >= 20, ...)) ----
    const Arvore aninhada = criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarVariavel(PA), criarConstante(71)),
                                    se(Comparacao::MAIOR_IGUAL, PT, 30, P, C), se(Comparacao::MAIOR_IGUAL, PT, 20, P, C));
    VERIFICAR(validar(*aninhada, pig).empty());
    VERIFICAR(profundidade(*aninhada) == 3);
    VERIFICAR(tamanho(*aninhada) == 16);
    {
        bool tudoCerto = true;
        for (int pa = 0; pa <= 100; pa += 7)
            for (int pt = 0; pt <= 100; ++pt) {
                const Acao esperado = (pa >= 71) ? (pt >= 30 ? P : C) : (pt >= 20 ? P : C);
                if (avaliar(*aninhada, Estado{pt, 0, pa}) != esperado) tudoCerto = false;
            }
        VERIFICAR(tudoCerto);
    }

    // ---- folha sozinha ----
    const Arvore folha = criarAcao(P);
    VERIFICAR(validar(*folha, pig).empty());
    VERIFICAR(profundidade(*folha) == 0 && tamanho(*folha) == 1);
    VERIFICAR(avaliar(*folha, Estado{}) == P);  // não lê o estado

    // ---- cópia profunda ----
    {
        Arvore c = copiar(*aninhada);
        VERIFICAR(paraTexto(*c, pig) == paraTexto(*aninhada, pig));
        std::set<const No*> a, b;
        enderecos(*aninhada, a);
        enderecos(*c, b);
        bool disjuntos = true;
        for (const No* x : b) disjuntos &= (a.count(x) == 0);
        VERIFICAR(disjuntos);
        VERIFICAR(a.size() == 16 && b.size() == 16);
        // mexer na cópia não mexe no original
        c->filhos[1]->filhos[0]->filhos[1]->valor = 99;
        VERIFICAR(paraTexto(*aninhada, pig).find("99") == std::string::npos);
        VERIFICAR(paraTexto(*c, pig).find("pontos_turno >= 99") != std::string::npos);
    }

    // ---- estado curto demais ----
    VERIFICAR_LANCA(std::out_of_range, avaliar(*p20, Estado{}));

    // ---- validar recusa árvores mal formadas ----
    auto invalida = [&](const No& arv, const Vocabulario& v) {
        const std::string e = validar(arv, v);
        return !e.empty();
    };
    VERIFICAR(invalida(*criarSe(criarAcao(P), criarAcao(P), criarAcao(C)), pig));  // condição é ação
    VERIFICAR(invalida(*criarSe(criarComparacao(Comparacao::MAIOR, criarVariavel(PT), criarConstante(5)),
                                criarComparacao(Comparacao::MAIOR, criarVariavel(PT), criarConstante(5)), criarAcao(C)),
                       pig));                                                                  // ação é comparação
    VERIFICAR(invalida(*criarComparacao(Comparacao::MAIOR, criarVariavel(PT), criarConstante(5)), pig));  // raiz comparação
    VERIFICAR(invalida(*criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarConstante(5), criarVariavel(PT)),
                                criarAcao(P), criarAcao(C)),
                       pig));                                                                  // constante à esquerda
    VERIFICAR(invalida(*se(Comparacao::MAIOR_IGUAL, PT, 0, P, C), pig));    // pontos_turno >= 0: sempre verdadeiro
    VERIFICAR(invalida(*se(Comparacao::MAIOR, PT, 100, P, C), pig));        // pontos_turno > 100: sempre falso
    VERIFICAR(invalida(*se(Comparacao::MAIOR, PT, 250, P, C), pig));        // fora da faixa
    VERIFICAR(invalida(*criarSe(criarComparacao(Comparacao::MAIOR, criarVariavel(PT), criarVariavel(PT)), criarAcao(P),
                                criarAcao(C)),
                       pig));                                                                  // x > x
    VERIFICAR(invalida(*criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarVariavel(bj.indiceDe("sou_segundo")),
                                                criarVariavel(bj.indiceDe("minha_soma"))),
                                criarAcao(P), criarAcao(C)),
                       bj));                                                                   // faixas diferentes
    VERIFICAR(invalida(*se(Comparacao::MAIOR, 7, 5, P, C), pig));           // variável que não existe
    {
        Arvore a = copiar(*p20);
        a->filhos[1]->valor = 5;  // ação que não existe
        VERIFICAR(invalida(*a, pig));
    }
    {
        Arvore a = copiar(*p20);
        a->filhos[0]->valor = 9;  // operador que não existe
        VERIFICAR(invalida(*a, pig));
    }
    {
        Arvore a = copiar(*p20);
        a->filhos[2].reset();  // filho nulo
        VERIFICAR(invalida(*a, pig));
    }
    {
        Arvore a = copiar(*p20);
        a->filhos.pop_back();  // SE com 2 filhos
        VERIFICAR(invalida(*a, pig));
    }
    {
        Arvore a = copiar(*p20);
        a->filhos[1]->filhos.push_back(criarAcao(P));  // folha com filho
        VERIFICAR(invalida(*a, pig));
    }
    std::cout << "  [info] exemplo de mensagem: " << validar(*se(Comparacao::MAIOR, PT, 100, P, C), pig) << "\n";

    // ---- nós efetivos (bloat) ----
    {
        // SE(pontos_turno >= 20, SE(pontos_turno >= 10, PARAR, CONTINUAR), CONTINUAR)
        // o ramo "pontos_turno >= 10" é sempre verdadeiro quando chega lá: o CONTINUAR de dentro nunca é visitado.
        Arvore t = criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarVariavel(0), criarConstante(20)),
                           criarSe(criarComparacao(Comparacao::MAIOR_IGUAL, criarVariavel(0), criarConstante(10)),
                                   criarAcao(Acao::PARAR), criarAcao(Acao::CONTINUAR)),
                           criarAcao(Acao::CONTINUAR));
        std::unordered_set<const No*> vis;
        VERIFICAR(nosEfetivos(*t, vis) == 0);
        bool igual = true;
        for (int pt = 0; pt <= 100; ++pt) {
            const Estado e = {pt, 0, 0};
            igual &= avaliarMarcando(*t, e, vis) == avaliar(*t, e);
        }
        VERIFICAR(igual);
        VERIFICAR(tamanho(*t) == 11);
        VERIFICAR_MSG(nosEfetivos(*t, vis) == 10, nosEfetivos(*t, vis));  // 11 menos o CONTINUAR inalcançável
        std::unordered_set<const No*> so20;
        avaliarMarcando(*t, Estado{5, 0, 0}, so20);  // só o caminho "falso" da raiz
        VERIFICAR(nosEfetivos(*t, so20) == 5);       // SE + comparação (3) + folha
    }

    return teste::fim("arvore");
}
