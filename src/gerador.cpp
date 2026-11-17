#include "gerador.h"

#include <set>
#include <stdexcept>
#include <string>

// ATENÇÃO à ordem dos sorteios: em C++ a ordem em que os argumentos de uma função
// são calculados NÃO é definida. Escrever criarSe(gerarCondicao(), gerarX(), gerarY())
// faria cada compilador consumir os números aleatórios numa ordem diferente, e a
// mesma semente daria árvores diferentes. Por isso cada sorteio é guardado numa
// variável, uma linha depois da outra, antes de montar o nó.

Gerador::Gerador(const Vocabulario& vocab, Aleatorio& aleatorio) : vocab_(vocab), aleatorio_(aleatorio) {
    if (vocab_.variaveis.empty() || vocab_.acoes.empty() || vocab_.comparacoes.empty())
        throw std::invalid_argument("Gerador: vocabulário incompleto");
}

Arvore Gerador::gerarAcaoFolha() {
    return criarAcao(vocab_.acoes[aleatorio_.indice(vocab_.acoes.size())]);
}

Arvore Gerador::gerarCondicao() {
    const int esquerda = static_cast<int>(aleatorio_.indice(vocab_.variaveis.size()));
    const Comparacao op = vocab_.comparacoes[aleatorio_.indice(vocab_.comparacoes.size())];

    std::vector<int> compativeis;
    for (int j = 0; j < static_cast<int>(vocab_.variaveis.size()); ++j)
        if (vocab_.compativeis(esquerda, j)) compativeis.push_back(j);

    Arvore direita;
    if (!compativeis.empty() && aleatorio_.chance(CHANCE_COMPARAR_COM_VARIAVEL)) {
        direita = criarVariavel(compativeis[aleatorio_.indice(compativeis.size())]);
    } else {
        // A constante sai da faixa que não deixa a comparação trivial (ver faixaConstante).
        const Faixa faixa = faixaConstante(vocab_.variaveis[static_cast<std::size_t>(esquerda)], op);
        direita = criarConstante(aleatorio_.inteiro(faixa.min, faixa.max));
    }
    return criarComparacao(op, criarVariavel(esquerda), std::move(direita));
}

Arvore Gerador::gerarSubarvoreAcao(int profundidadeMax, Metodo metodo) {
    // Um SE precisa de 2 níveis abaixo dele (SE -> comparação -> variável).
    // Se não cabe, só resta a folha.
    if (profundidadeMax < 2) return gerarAcaoFolha();
    const bool fazerSe = (metodo == Metodo::COMPLETO) || aleatorio_.chance(CHANCE_SE_NO_CRESCER);
    if (!fazerSe) return gerarAcaoFolha();

    Arvore condicao = gerarCondicao();
    Arvore entao = gerarSubarvoreAcao(profundidadeMax - 1, metodo);
    Arvore senao = gerarSubarvoreAcao(profundidadeMax - 1, metodo);
    return criarSe(std::move(condicao), std::move(entao), std::move(senao));
}

Arvore Gerador::gerarArvore(int profundidade, Metodo metodo) {
    if (profundidade < 2) throw std::invalid_argument("gerarArvore: profundidade deve ser >= 2");
    // A raiz é sempre SE (uma folha sozinha teria profundidade 0 < profundidade_min).
    Arvore condicao = gerarCondicao();
    Arvore entao = gerarSubarvoreAcao(profundidade - 1, metodo);
    Arvore senao = gerarSubarvoreAcao(profundidade - 1, metodo);
    return criarSe(std::move(condicao), std::move(entao), std::move(senao));
}

std::vector<Arvore> Gerador::gerarPopulacao(int tamanho, int profMin, int profMax) {
    if (tamanho < 0) throw std::invalid_argument("gerarPopulacao: tamanho negativo");
    if (profMin < 2 || profMin > profMax) throw std::invalid_argument("gerarPopulacao: profundidades inválidas");

    const int quantasProfundidades = profMax - profMin + 1;
    std::vector<Arvore> populacao;
    populacao.reserve(static_cast<std::size_t>(tamanho));
    std::set<std::string> vistas;  // texto das árvores já criadas, para evitar repetidas

    for (int i = 0; i < tamanho; ++i) {
        // i = 0, 1, 2, 3 -> profundidades 2, 3, 4, 5 (COMPLETO)
        // i = 4, 5, 6, 7 -> profundidades 2, 3, 4, 5 (CRESCER), e assim por diante.
        const int prof = profMin + i % quantasProfundidades;
        const Metodo metodo = ((i / quantasProfundidades) % 2 == 0) ? Metodo::COMPLETO : Metodo::CRESCER;

        Arvore escolhida;
        for (int t = 0; t < TENTATIVAS_SEM_REPETIR; ++t) {
            Arvore candidata = gerarArvore(prof, metodo);
            // CRESCER pode sair mais rasa que profMin quando profMin > 2.
            if (profundidade(*candidata) < profMin) continue;
            const bool nova = vistas.insert(paraTexto(*candidata, vocab_)).second;
            escolhida = std::move(candidata);
            if (nova) break;  // se for repetida, tenta de novo (fica com ela se acabarem as tentativas)
        }
        if (!escolhida) {
            escolhida = gerarArvore(prof, Metodo::COMPLETO);  // COMPLETO sempre tem profundidade prof
            vistas.insert(paraTexto(*escolhida, vocab_));
        }
        populacao.push_back(std::move(escolhida));
    }
    return populacao;
}
