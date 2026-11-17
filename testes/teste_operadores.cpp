// Testes do cruzamento e da mutação.
#include <cstdlib>
#include <set>

#include "configuracao.h"
#include "operadores.h"
#include "teste.h"

namespace {

void enderecos(const No& no, std::set<const No*>& saida) {
    saida.insert(&no);
    for (const Arvore& f : no.filhos) enderecos(*f, saida);
}

// O filho não pode dividir nenhum nó com os pais.
bool disjunto(const No& filho, const No& a, const No* b = nullptr) {
    std::set<const No*> ef, ep;
    enderecos(filho, ef);
    enderecos(a, ep);
    if (b) enderecos(*b, ep);
    for (const No* x : ef)
        if (ep.count(x)) return false;
    return true;
}

// Compara duas árvores nó a nó. Conta constantes diferentes (e o maior |delta|)
// e quaisquer outras diferenças (tipo, valor de outro nó, estrutura).
struct Diferenca {
    int constantes = 0;
    int outras = 0;
    int maiorDelta = 0;
};
void comparar(const No& a, const No& b, Diferenca& d) {
    if (a.tipo != b.tipo || a.filhos.size() != b.filhos.size()) {
        ++d.outras;
        return;
    }
    if (a.valor != b.valor) {
        if (a.tipo == TipoNo::CONSTANTE) {
            ++d.constantes;
            d.maiorDelta = std::max(d.maiorDelta, std::abs(a.valor - b.valor));
        } else {
            ++d.outras;
        }
    }
    for (std::size_t i = 0; i < a.filhos.size(); ++i) comparar(*a.filhos[i], *b.filhos[i], d);
}

Arvore se(Comparacao op, int var, int c, Acao a, Acao b) {
    return criarSe(criarComparacao(op, criarVariavel(var), criarConstante(c)), criarAcao(a), criarAcao(b));
}

double taxa(long parte, long total) { return total ? 100.0 * static_cast<double>(parte) / static_cast<double>(total) : 0.0; }

}  // namespace

int main() {
    const Parametros p = lerParametros(teste::lerJson("dataset/parametros.json"));
    const int LIMITE = p.profundidade_limite_filhos;

    for (const char* caminho : {"dataset/specs_regras_pig.json", "dataset/specs_regras_blackjack.json"}) {
        const Vocabulario v = lerVocabulario(teste::lerJson(caminho));
        ContadoresOperadores cont;
        long filhosIguaisAoPai = 0, mutantesIguais = 0, filhosFolha = 0;

        for (std::uint32_t semente = 1; semente <= 10; ++semente) {
            Aleatorio a(semente);
            Gerador g(v, a);
            const std::vector<Arvore> pop = g.gerarPopulacao(p.populacao, p.profundidade_min, p.profundidade_max);

            // ---- cruzamento: 1000 por semente ----
            for (int i = 0; i < 1000; ++i) {
                const No& p1 = *pop[a.indice(pop.size())];
                const No& p2 = *pop[a.indice(pop.size())];
                const std::string t1 = paraTexto(p1, v), t2 = paraTexto(p2, v);
                const Arvore f = cruzar(p1, p2, p, a, &cont);
                const std::string erro = validar(*f, v);
                VERIFICAR_MSG(erro.empty(), erro << " | " << t1 << " x " << t2);
                VERIFICAR(profundidade(*f) <= LIMITE);
                VERIFICAR(paraTexto(p1, v) == t1 && paraTexto(p2, v) == t2);  // pais intactos
                VERIFICAR(disjunto(*f, p1, &p2));                              // nada compartilhado
                if (paraTexto(*f, v) == t1) ++filhosIguaisAoPai;
                if (f->tipo == TipoNo::ACAO) ++filhosFolha;
            }

            // ---- mutação: uma por indivíduo, 10 vezes ----
            for (int rep = 0; rep < 10; ++rep)
                for (const Arvore& orig : pop) {
                    const std::string t = paraTexto(*orig, v);
                    const Arvore m = mutar(*orig, p, g, &cont);
                    const std::string erro = validar(*m, v);
                    VERIFICAR_MSG(erro.empty(), erro << " | " << t);
                    VERIFICAR(profundidade(*m) <= LIMITE);
                    VERIFICAR(paraTexto(*orig, v) == t);
                    VERIFICAR(disjunto(*m, *orig));
                    if (paraTexto(*m, v) == t) ++mutantesIguais;
                }

            // ---- cadeia de 40 gerações sem fitness (pais sorteados), para estressar ----
            // Cada filho: cruzamento com 85% e mutação com 5%, como no parametros.json.
            if (semente <= 3) {
                std::vector<Arvore> atual;
                for (const Arvore& x : pop) atual.push_back(copiar(*x));
                for (int geracao = 1; geracao <= 40; ++geracao) {
                    std::vector<Arvore> nova;
                    for (std::size_t i = 0; i < atual.size(); ++i) {
                        const No& pa = *atual[a.indice(atual.size())];
                        Arvore filho = a.chance(p.cruzamento.probabilidade) ? cruzar(pa, *atual[a.indice(atual.size())], p, a)
                                                                            : copiar(pa);
                        if (a.chance(p.mutacao.probabilidade)) filho = mutar(*filho, p, g);
                        nova.push_back(std::move(filho));
                    }
                    atual = std::move(nova);  // a geração anterior é apagada aqui
                    bool ok = true;
                    long soma = 0;
                    int maxProf = 0;
                    for (const Arvore& x : atual) {
                        ok &= validar(*x, v).empty() && profundidade(*x) <= LIMITE;
                        soma += tamanho(*x);
                        maxProf = std::max(maxProf, profundidade(*x));
                    }
                    VERIFICAR_MSG(ok, "geração " << geracao);
                    if (semente == 1 && (geracao == 1 || geracao % 10 == 0))
                        std::cout << "  [info] " << caminho << " cadeia sem seleção, geração " << geracao
                                  << ": tamanho médio " << static_cast<double>(soma) / static_cast<double>(atual.size())
                                  << ", profundidade máxima " << maxProf << "\n";
                }
            }
        }

        std::cout << "  [info] " << caminho << "\n"
                  << "         cruzamentos: " << cont.cruzamentos << ", tentativas por cruzamento: "
                  << static_cast<double>(cont.tentativasCruzamento) / static_cast<double>(cont.cruzamentos)
                  << ", desistências: " << cont.cruzamentosDesistidos << " (" << taxa(cont.cruzamentosDesistidos, cont.cruzamentos)
                  << "%), filho igual ao pai1: " << taxa(filhosIguaisAoPai, cont.cruzamentos)
                  << "%, filho virou folha: " << taxa(filhosFolha, cont.cruzamentos) << "%\n"
                  << "         mutações: " << cont.mutacoes << ", só constante: " << taxa(cont.mutacoesConstante, cont.mutacoes)
                  << "%, sem constante mutável: " << taxa(cont.semConstanteMutavel, cont.mutacoes)
                  << "%, desistências: " << cont.mutacoesDesistidas << ", mutante igual ao original: "
                  << taxa(mutantesIguais, cont.mutacoes) << "%\n";
        VERIFICAR(cont.mutacoesDesistidas == 0);
    }

    const Vocabulario pig = lerVocabulario(teste::lerJson("dataset/specs_regras_pig.json"));
    const Vocabulario bj = lerVocabulario(teste::lerJson("dataset/specs_regras_blackjack.json"));
    const int PT = pig.indiceDe("pontos_turno"), MP = pig.indiceDe("meu_placar"), PA = pig.indiceDe("placar_adv");
    const Acao C = Acao::CONTINUAR, P = Acao::PARAR;

    // ---- mutação só de constante: muda exatamente 1 constante, por 1 a 3, dentro da faixa ----
    {
        Parametros q = p;
        q.mutacao.prob_so_constante = 1.0;
        Aleatorio a(7);
        Gerador g(pig, a);
        const Arvore p20 = se(Comparacao::MAIOR_IGUAL, PT, 20, P, C);
        std::set<int> novos;
        bool ok = true;
        for (int i = 0; i < 3000; ++i) {
            const Arvore m = mutar(*p20, q, g);
            Diferenca d;
            comparar(*p20, *m, d);
            ok &= d.constantes == 1 && d.outras == 0 && d.maiorDelta >= 1 && d.maiorDelta <= 3;
            novos.insert(m->filhos[0]->filhos[1]->valor);
        }
        VERIFICAR(ok);
        VERIFICAR((novos == std::set<int>{17, 18, 19, 21, 22, 23}));  // todos os vizinhos aparecem, 20 nunca

        // Na borda: pontos_turno >= 100 só pode ir para 97, 98, 99.
        const Arvore borda = se(Comparacao::MAIOR_IGUAL, PT, 100, P, C);
        std::set<int> naBorda;
        for (int i = 0; i < 2000; ++i) naBorda.insert(mutar(*borda, q, g)->filhos[0]->filhos[1]->valor);
        VERIFICAR((naBorda == std::set<int>{97, 98, 99}));

        // Sem constante (só variável com variável): cai para a mutação de subárvore.
        ContadoresOperadores c;
        const Arvore vv = criarSe(criarComparacao(Comparacao::MAIOR, criarVariavel(MP), criarVariavel(PA)), criarAcao(P),
                                  criarAcao(C));
        for (int i = 0; i < 500; ++i) VERIFICAR(validar(*mutar(*vv, q, g, &c), pig).empty());
        VERIFICAR(c.semConstanteMutavel == 500 && c.mutacoesConstante == 0);

        // Blackjack: sou_segundo >= 1 não tem para onde ir -> subárvore.
        Aleatorio ab(8);
        Gerador gb(bj, ab);
        ContadoresOperadores cb;
        const Arvore s2 = se(Comparacao::MAIOR_IGUAL, bj.indiceDe("sou_segundo"), 1, P, C);
        for (int i = 0; i < 500; ++i) VERIFICAR(validar(*mutar(*s2, q, gb, &cb), bj).empty());
        VERIFICAR(cb.semConstanteMutavel == 500);
    }

    // ---- cruzamento com folhas ----
    {
        Aleatorio a(3);
        const Arvore folha = criarAcao(P);
        const Arvore p20 = se(Comparacao::MAIOR_IGUAL, PT, 20, P, C);
        ContadoresOperadores c;
        bool ok = true;
        for (int i = 0; i < 500; ++i) {
            ok &= validar(*cruzar(*p20, *folha, p, a, &c), pig).empty();
            ok &= validar(*cruzar(*folha, *p20, p, a, &c), pig).empty();
            ok &= validar(*cruzar(*folha, *folha, p, a, &c), pig).empty();
        }
        VERIFICAR(ok);
        VERIFICAR(c.cruzamentosDesistidos == 0);
    }

    // ---- pais já no limite (profundidade 6, cheios): o limite segura ----
    {
        Aleatorio a(21);
        Gerador g(pig, a);
        ContadoresOperadores c;
        bool ok = true;
        for (int i = 0; i < 1000; ++i) {
            const Arvore x = g.gerarArvore(LIMITE, Metodo::COMPLETO);
            const Arvore y = g.gerarArvore(LIMITE, Metodo::COMPLETO);
            const Arvore f = cruzar(*x, *y, p, a, &c);
            ok &= profundidade(*f) <= LIMITE && validar(*f, pig).empty();
            const Arvore m = mutar(*x, p, g, &c);
            ok &= profundidade(*m) <= LIMITE && validar(*m, pig).empty();
        }
        VERIFICAR(ok);
        std::cout << "  [info] pais cheios no limite: tentativas por cruzamento "
                  << static_cast<double>(c.tentativasCruzamento) / static_cast<double>(c.cruzamentos)
                  << ", desistências " << c.cruzamentosDesistidos << " de " << c.cruzamentos << "\n";
    }

    // ---- entrada acima do limite é recusada ----
    {
        Aleatorio a(1);
        Gerador g(pig, a);
        const Arvore funda = g.gerarArvore(LIMITE + 1, Metodo::COMPLETO);
        const Arvore rasa = g.gerarArvore(2, Metodo::COMPLETO);
        VERIFICAR_LANCA(std::invalid_argument, cruzar(*funda, *rasa, p, a));
        VERIFICAR_LANCA(std::invalid_argument, cruzar(*rasa, *funda, p, a));
        VERIFICAR_LANCA(std::invalid_argument, mutar(*funda, p, g));
    }

    // ---- reprodutibilidade: mesma semente -> mesma sequência de filhos ----
    for (int rodada = 0; rodada < 3; ++rodada) {
        std::vector<std::string> saida[2];
        for (int k = 0; k < 2; ++k) {
            Aleatorio a(42);
            Gerador g(pig, a);
            const auto pop = g.gerarPopulacao(100, 2, 5);
            for (int i = 0; i < 300; ++i) {
                // Dois sorteios em linhas separadas: dentro dos argumentos de uma
                // mesma chamada, a ordem mudaria entre g++ e clang.
                const std::size_t i1 = a.indice(100);
                const std::size_t i2 = a.indice(100);
                const Arvore f = cruzar(*pop[i1], *pop[i2], p, a);
                saida[k].push_back(paraTexto(*f, pig));
                saida[k].push_back(paraTexto(*mutar(*f, p, g), pig));
            }
        }
        VERIFICAR(saida[0] == saida[1]);
    }

    return teste::fim("operadores");
}
