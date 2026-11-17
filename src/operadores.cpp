#include "operadores.h"

#include <algorithm>
#include <stdexcept>
#include <vector>

namespace {

enum class Categoria { ACAO, CONDICAO };

// Um ponto de corte: o "lugar" onde o nó está pendurado (o unique_ptr que é dono
// dele), a profundidade desse lugar, o tipo e se é interno (SE/comparação) ou folha.
// Guardar o lugar (Arvore*) permite trocar o nó inteiro com uma atribuição:
//     *ponto.lugar = std::move(outraSubarvore);
struct Ponto {
    Arvore* lugar;
    int profundidade;
    Categoria categoria;
    bool interno;
};

void coletarPontos(Arvore& lugar, int prof, std::vector<Ponto>& saida) {
    No& no = *lugar;
    switch (no.tipo) {
        case TipoNo::SE:
            saida.push_back({&lugar, prof, Categoria::ACAO, true});
            for (Arvore& filho : no.filhos) coletarPontos(filho, prof + 1, saida);
            break;
        case TipoNo::ACAO:
            saida.push_back({&lugar, prof, Categoria::ACAO, false});
            break;
        case TipoNo::COMPARACAO:
            // A comparação entra inteira; os operandos dela não são pontos de corte.
            saida.push_back({&lugar, prof, Categoria::CONDICAO, true});
            break;
        case TipoNo::VARIAVEL:
        case TipoNo::CONSTANTE:
            break;
    }
}

// Escolhe um ponto: com probabilidade probInterno entre os internos, senão entre as
// folhas. Se o grupo sorteado estiver vazio, usa o outro.
std::size_t escolherPonto(const std::vector<Ponto>& pontos, double probInterno, Aleatorio& aleatorio) {
    std::vector<std::size_t> internos, folhas;
    for (std::size_t i = 0; i < pontos.size(); ++i) (pontos[i].interno ? internos : folhas).push_back(i);
    const bool querInterno = aleatorio.chance(probInterno);  // sorteia sempre, para a sequência não depender do caso
    const std::vector<std::size_t>& grupo = ((querInterno && !internos.empty()) || folhas.empty()) ? internos : folhas;
    return grupo[aleatorio.indice(grupo.size())];
}

void exigirDentroDoLimite(const No& arvore, int limite, const char* quem) {
    if (profundidade(arvore) > limite)
        throw std::invalid_argument(std::string(quem) + ": árvore de entrada acima de profundidade_limite_filhos");
}

// Muda uma constante; devolve false se a árvore não tem constante que possa mudar.
bool mutarConstante(No& raiz, int variacao, const Vocabulario& vocab, Aleatorio& aleatorio) {
    struct Candidata {
        No* constante;
        int min;
        int max;
    };
    std::vector<Candidata> candidatas;
    std::vector<No*> pilha{&raiz};  // percurso sem recursão
    while (!pilha.empty()) {
        No* no = pilha.back();
        pilha.pop_back();
        if (no->tipo == TipoNo::COMPARACAO && no->filhos[1]->tipo == TipoNo::CONSTANTE) {
            const Variavel& v = vocab.variaveis.at(static_cast<std::size_t>(no->filhos[0]->valor));
            const Faixa f = faixaConstante(v, static_cast<Comparacao>(no->valor));
            No* k = no->filhos[1].get();
            // Novos valores possíveis: [k-variacao, k+variacao] cortado pela faixa permitida.
            const int lo = std::max(f.min, k->valor - variacao);
            const int hi = std::min(f.max, k->valor + variacao);
            // Precisa existir pelo menos um valor diferente do atual.
            // (Ex.: sou_segundo >= 1 só aceita 1, então não pode mudar.)
            if (k->valor >= f.min && k->valor <= f.max && hi > lo) candidatas.push_back({k, lo, hi});
        }
        for (Arvore& filho : no->filhos) pilha.push_back(filho.get());
    }
    if (candidatas.empty()) return false;

    Candidata& c = candidatas[aleatorio.indice(candidatas.size())];
    // Sorteia em [lo, hi-1] e "pula" o valor atual: se cair nele ou acima, soma 1.
    // Assim o novo valor é uniforme entre os possíveis e nunca igual ao atual.
    const int r = aleatorio.inteiro(c.min, c.max - 1);
    c.constante->valor = (r < c.constante->valor) ? r : r + 1;
    return true;
}

}  // namespace

Arvore cruzar(const No& pai1, const No& pai2, const Parametros& p, Aleatorio& aleatorio,
              ContadoresOperadores* contadores) {
    const int limite = p.profundidade_limite_filhos;
    exigirDentroDoLimite(pai1, limite, "cruzar");
    exigirDentroDoLimite(pai2, limite, "cruzar");

    // Cópias profundas: o filho começa igual ao pai1; o doador é uma cópia do pai2
    // de onde o pedaço é "arrancado" (move), sem precisar copiar de novo.
    Arvore filho = copiar(pai1);
    Arvore doador = copiar(pai2);
    std::vector<Ponto> pontosFilho, pontosDoador;
    coletarPontos(filho, 0, pontosFilho);
    coletarPontos(doador, 0, pontosDoador);
    if (contadores) ++contadores->cruzamentos;

    for (int t = 0; t < TENTATIVAS_OPERADOR; ++t) {
        if (contadores) ++contadores->tentativasCruzamento;
        const Ponto& alvo = pontosFilho[escolherPonto(pontosFilho, p.cruzamento.prob_ponto_interno, aleatorio)];

        std::vector<Ponto> compativeis;
        for (const Ponto& q : pontosDoador)
            if (q.categoria == alvo.categoria) compativeis.push_back(q);
        if (compativeis.empty()) continue;  // ex.: pai2 é uma folha e o alvo é uma comparação
        const Ponto& origem = compativeis[escolherPonto(compativeis, p.cruzamento.prob_ponto_interno, aleatorio)];

        // O resto do filho tem no máximo a profundidade do pai1 (<= limite);
        // só o ramo novo pode passar do limite.
        if (alvo.profundidade + profundidade(**origem.lugar) > limite) continue;  // descarta e sorteia de novo

        *alvo.lugar = std::move(*origem.lugar);  // o nó antigo do filho é apagado aqui (unique_ptr)
        return filho;
    }
    if (contadores) ++contadores->cruzamentosDesistidos;
    return filho;  // nenhuma troca coube: cópia intacta do pai1
}

Arvore mutar(const No& arvore, const Parametros& p, Gerador& gerador, ContadoresOperadores* contadores) {
    const int limite = p.profundidade_limite_filhos;
    exigirDentroDoLimite(arvore, limite, "mutar");
    Aleatorio& aleatorio = gerador.aleatorio();
    Arvore filho = copiar(arvore);
    if (contadores) ++contadores->mutacoes;

    if (aleatorio.chance(p.mutacao.prob_so_constante)) {
        const int variacao = static_cast<int>(p.mutacao.variacao_constante);
        if (mutarConstante(*filho, variacao, gerador.vocabulario(), aleatorio)) {
            if (contadores) ++contadores->mutacoesConstante;
            return filho;
        }
        if (contadores) ++contadores->semConstanteMutavel;  // cai para a mutação de subárvore
    }

    std::vector<Ponto> pontos;
    coletarPontos(filho, 0, pontos);
    for (int t = 0; t < TENTATIVAS_OPERADOR; ++t) {
        if (contadores) ++contadores->tentativasMutacao;
        const Ponto& alvo = pontos[aleatorio.indice(pontos.size())];
        Arvore nova = (alvo.categoria == Categoria::CONDICAO)
                          ? gerador.gerarCondicao()
                          : gerador.gerarSubarvoreAcao(p.mutacao.profundidade_subarvore, Metodo::CRESCER);
        if (alvo.profundidade + profundidade(*nova) > limite) continue;
        *alvo.lugar = std::move(nova);
        return filho;
    }
    if (contadores) ++contadores->mutacoesDesistidas;
    return filho;
}
