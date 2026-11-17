// Testes do gerador: validade, profundidades, cobertura do vocabulário,
// repetição na população inicial e reprodutibilidade pela semente.
#include <map>
#include <set>

#include "configuracao.h"
#include "gerador.h"
#include "teste.h"

namespace {

struct Estatisticas {
    std::map<int, int> porProfundidade;
    std::set<int> variaveisEsquerda, operadores, acoes;
    int comparacoesVarVar = 0, comparacoesConstante = 0;
    // menor e maior constante vista para cada (variável, operador)
    std::map<std::pair<int, int>, std::pair<int, int>> constantes;
};

void coletar(const No& no, Estatisticas& e) {
    if (no.tipo == TipoNo::ACAO) e.acoes.insert(no.valor);
    if (no.tipo == TipoNo::COMPARACAO) {
        const int var = no.filhos[0]->valor;
        e.variaveisEsquerda.insert(var);
        e.operadores.insert(no.valor);
        if (no.filhos[1]->tipo == TipoNo::VARIAVEL) {
            ++e.comparacoesVarVar;
        } else {
            ++e.comparacoesConstante;
            const auto chave = std::make_pair(var, no.valor);
            const int c = no.filhos[1]->valor;
            auto it = e.constantes.find(chave);
            if (it == e.constantes.end()) e.constantes[chave] = {c, c};
            else it->second = {std::min(it->second.first, c), std::max(it->second.second, c)};
        }
    }
    for (const Arvore& f : no.filhos) coletar(*f, e);
}

std::vector<std::string> textos(const std::vector<Arvore>& pop, const Vocabulario& v) {
    std::vector<std::string> t;
    for (const Arvore& a : pop) t.push_back(paraTexto(*a, v));
    return t;
}

}  // namespace

int main() {
    const Parametros p = lerParametros(teste::lerJson("dataset/parametros.json"));
    const int k = p.profundidade_max - p.profundidade_min + 1;

    for (const char* caminho : {"dataset/specs_regras_pig.json", "dataset/specs_regras_blackjack.json"}) {
        const Vocabulario v = lerVocabulario(teste::lerJson(caminho));
        Estatisticas est;
        int repetidas = 0, arvores = 0;
        long somaTamanho = 0;

        // 30 sementes x 100 árvores.
        for (std::uint32_t semente = 1; semente <= 30; ++semente) {
            Aleatorio a(semente);
            Gerador g(v, a);
            const std::vector<Arvore> pop = g.gerarPopulacao(p.populacao, p.profundidade_min, p.profundidade_max);
            VERIFICAR(static_cast<int>(pop.size()) == p.populacao);
            std::set<std::string> vistas;
            for (std::size_t i = 0; i < pop.size(); ++i) {
                const No& arv = *pop[i];
                const std::string erro = validar(arv, v);
                VERIFICAR_MSG(erro.empty(), erro << " em " << paraTexto(arv, v));
                VERIFICAR(arv.tipo == TipoNo::SE);
                const int d = profundidade(arv);
                const int esperado = p.profundidade_min + static_cast<int>(i) % k;
                const bool completo = (static_cast<int>(i) / k) % 2 == 0;
                if (completo) VERIFICAR_MSG(d == esperado, "i=" << i << " d=" << d);
                else VERIFICAR_MSG(d >= p.profundidade_min && d <= esperado, "i=" << i << " d=" << d);
                if (!vistas.insert(paraTexto(arv, v)).second) ++repetidas;
                est.porProfundidade[d]++;
                somaTamanho += tamanho(arv);
                ++arvores;
                coletar(arv, est);
            }
        }

        std::cout << "  [info] " << caminho << ": " << arvores << " árvores, tamanho médio "
                  << static_cast<double>(somaTamanho) / arvores << " nós, repetidas dentro da população: " << repetidas
                  << "\n         profundidades:";
        for (const auto& [d, n] : est.porProfundidade) std::cout << " " << d << "->" << n;
        std::cout << "; comparações var-var: " << est.comparacoesVarVar << ", com constante: " << est.comparacoesConstante
                  << "\n";

        VERIFICAR(repetidas == 0);
        // Todas as variáveis, operadores e ações aparecem.
        VERIFICAR(est.variaveisEsquerda.size() == v.variaveis.size());
        VERIFICAR(est.operadores.size() == v.comparacoes.size());
        VERIFICAR(est.acoes.size() == v.acoes.size());
        VERIFICAR(est.comparacoesVarVar > 0 && est.comparacoesConstante > 0);
        // As constantes chegam às duas pontas da faixa permitida e não passam delas.
        for (const auto& [chave, mm] : est.constantes) {
            const Faixa f = faixaConstante(v.variaveis[static_cast<std::size_t>(chave.first)],
                                           static_cast<Comparacao>(chave.second));
            VERIFICAR_MSG(mm.first == f.min && mm.second == f.max,
                          v.variaveis[static_cast<std::size_t>(chave.first)].nome << " op " << chave.second << ": visto ["
                                                                                  << mm.first << ", " << mm.second
                                                                                  << "], faixa [" << f.min << ", "
                                                                                  << f.max << "]");
        }
    }

    const Vocabulario pig = lerVocabulario(teste::lerJson("dataset/specs_regras_pig.json"));
    const Vocabulario bj = lerVocabulario(teste::lerJson("dataset/specs_regras_blackjack.json"));

    // ---- reprodutibilidade: mesma semente -> mesma população; outra semente -> outra ----
    for (int rodada = 0; rodada < 3; ++rodada) {
        Aleatorio a1(42), a2(42), a3(43);
        Gerador g1(pig, a1), g2(pig, a2), g3(pig, a3);
        const auto t1 = textos(g1.gerarPopulacao(100, 2, 5), pig);
        const auto t2 = textos(g2.gerarPopulacao(100, 2, 5), pig);
        const auto t3 = textos(g3.gerarPopulacao(100, 2, 5), pig);
        VERIFICAR(t1 == t2);
        VERIFICAR(t1 != t3);
        if (rodada == 0) std::cout << "  [info] semente 42, primeira árvore: " << t1[0] << "\n";
    }

    // ---- COMPLETO tem forma exata: 2^(d-1)-1 SEs, cada um com comparação de 3 nós, e 2^(d-1) folhas ----
    {
        Aleatorio a(5);
        Gerador g(pig, a);
        for (int d = 2; d <= 7; ++d) {
            const Arvore arv = g.gerarArvore(d, Metodo::COMPLETO);
            const int ses = (1 << (d - 1)) - 1;
            VERIFICAR(profundidade(*arv) == d);
            VERIFICAR_MSG(tamanho(*arv) == 4 * ses + (1 << (d - 1)), "d=" << d << " tamanho=" << tamanho(*arv));
        }
    }

    // ---- blackjack: sou_segundo só pode ser comparado com constante, e só existe uma por operador ----
    {
        Aleatorio a(11);
        Gerador g(bj, a);
        const int s2 = bj.indiceDe("sou_segundo");
        bool ok = true;
        for (int i = 0; i < 20000; ++i) {
            const Arvore c = g.gerarCondicao();
            if (c->filhos[0]->valor != s2) continue;
            const bool ge = c->valor == static_cast<int>(Comparacao::MAIOR_IGUAL);
            ok &= c->filhos[1]->tipo == TipoNo::CONSTANTE && c->filhos[1]->valor == (ge ? 1 : 0);
        }
        VERIFICAR(ok);
    }

    // ---- profundidade mínima maior que 2: CRESCER não pode ficar raso demais ----
    {
        Aleatorio a(3);
        Gerador g(pig, a);
        const auto pop = g.gerarPopulacao(200, 4, 6);
        bool ok = true;
        for (const Arvore& arv : pop) ok &= profundidade(*arv) >= 4 && profundidade(*arv) <= 6 && validar(*arv, pig).empty();
        VERIFICAR(ok);
    }

    // ---- entradas inválidas ----
    {
        Aleatorio a(1);
        Gerador g(pig, a);
        VERIFICAR_LANCA(std::invalid_argument, g.gerarArvore(1, Metodo::COMPLETO));
        VERIFICAR_LANCA(std::invalid_argument, g.gerarPopulacao(10, 1, 5));
        VERIFICAR_LANCA(std::invalid_argument, g.gerarPopulacao(10, 5, 3));
        VERIFICAR_LANCA(std::invalid_argument, g.gerarPopulacao(-1, 2, 5));
        VERIFICAR(g.gerarPopulacao(0, 2, 5).empty());
        VERIFICAR(profundidade(*g.gerarSubarvoreAcao(1, Metodo::COMPLETO)) == 0);
    }

    return teste::fim("gerador");
}
