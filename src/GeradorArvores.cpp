// GeradorDeArvores.cpp
#include "GeradorArvores.h"
#include <stdexcept>

GeradorDeArvores::GeradorDeArvores(unsigned int semente,
                                   const std::vector<SpecVariavel>& vars,
                                   const std::vector<std::string>& acs,
                                   const std::vector<std::string>& comps)
    : rng(semente), variaveis(vars), acoes(acs), comparacoes(comps) {
    if (variaveis.empty() || acoes.empty() || comparacoes.empty()) {
        throw std::invalid_argument("Vocabulario do jogo (variaveis, acoes ou comparacoes) nao pode ser vazio.");
    }
}

std::shared_ptr<No> GeradorDeArvores::criarNoAcao() {
    std::uniform_int_distribution<size_t> distAcao(0, acoes.size() - 1);
    auto no = std::make_shared<No>();
    no->tipo = TipoNo::ACAO;
    no->valorStr = acoes[distAcao(rng)];
    return no;
}

std::shared_ptr<No> GeradorDeArvores::criarNoComparacao() {
    // 1. Sorteia o operador (>= ou >)
    std::uniform_int_distribution<size_t> distComp(0, comparacoes.size() - 1);
    auto noComp = std::make_shared<No>();
    noComp->tipo = TipoNo::COMPARADOR;
    noComp->valorStr = comparacoes[distComp(rng)];

    // 2. Sorteia qual variável do estado será testada (ex: pontos_turno)
    std::uniform_int_distribution<size_t> distVar(0, variaveis.size() - 1);
    size_t idxVar = distVar(rng);

    auto noVar = std::make_shared<No>();
    noVar->tipo = TipoNo::VARIAVEL;
    noVar->valorStr = variaveis[idxVar].nome;
    noVar->valorNum = static_cast<int>(idxVar);

    // 3. Sorteia uma constante válida dentro do [min, max] daquela variável no JSON
    std::uniform_int_distribution<int> distConst(variaveis[idxVar].min, variaveis[idxVar].max);
    auto noConst = std::make_shared<No>();
    noConst->tipo = TipoNo::CONSTANTE;
    noConst->valorNum = distConst(rng);

    // Anexa variável e constante como filhos do comparador (profundidade +1 a partir do comparador)
    noComp->filhos.push_back(noVar);
    noComp->filhos.push_back(noConst);

    return noComp;
}

std::shared_ptr<No> GeradorDeArvores::gerarNoRecursivo(int profAtual, int profAlvo, int profMin) {
    // Como um nó SE + Comparador + Folha consome 2 níveis de profundidade,
    // se faltam menos de 2 níveis para o teto, somos obrigados a encerrar com uma AÇÃO.
    if (profAtual + 2 > profAlvo) {
        return criarNoAcao();
    }

    // Se ainda não atingimos a profundidade mínima exigida, somos obrigados a expandir com um SE
    bool forcarExpansao = (profAtual + 2 <= profMin);

    // Sorteia 50% de chance de expandir com SE ou encerrar com AÇÃO (caso não seja forçado)
    std::bernoulli_distribution distExpandir(0.5);

    if (forcarExpansao || distExpandir(rng)) {
        auto noSe = std::make_shared<No>();
        noSe->tipo = TipoNo::SE;
        noSe->valorStr = "SE";

        // Filho 0: Condição de comparação (já adiciona +2 de profundidade)
        noSe->filhos.push_back(criarNoComparacao());
        // Filho 1: Ramo Verdadeiro
        noSe->filhos.push_back(gerarNoRecursivo(profAtual + 1, profAlvo, profMin));
        // Filho 2: Ramo Falso
        noSe->filhos.push_back(gerarNoRecursivo(profAtual + 1, profAlvo, profMin));

        return noSe;
    }

    return criarNoAcao();
}

Individuo GeradorDeArvores::gerarIndividuo(int profMin, int profMax) {
    // Sorteia uma profundidade alvo entre profMin (ex: 2) e profMax (ex: 5) para dar diversidade
    std::uniform_int_distribution<int> distProf(profMin, profMax);
    int profAlvo = distProf(rng);

    Individuo ind;
    ind.raiz = gerarNoRecursivo(0, profAlvo, profMin);
    ind.profundidade = ind.raiz->calcularProfundidade();
    ind.numNos = ind.raiz->contarNos();
    ind.fitness = 0.0;

    return ind;
}

std::shared_ptr<No> GeradorDeArvores::gerarSubarvore(int profMax) {
    // Na mutação, a profundidade mínima da subárvore pode ser 0 (uma ação direta) até profMax (ex: 2)
    return gerarNoRecursivo(0, profMax, 0);
}