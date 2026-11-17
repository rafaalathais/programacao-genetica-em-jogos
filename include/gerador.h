#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_GERADOR_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_GERADOR_H

#include <vector>

#include "aleatorio.h"
#include "arvore.h"
#include "vocabulario.h"

// Dois jeitos clássicos de gerar uma árvore aleatória com profundidade máxima D:
//   COMPLETO: todo lugar de ação vira SE até a profundidade D (árvore "cheia",
//             com profundidade exatamente D).
//   CRESCER:  em cada lugar de ação sorteia SE ou folha, então os ramos têm
//             tamanhos variados (profundidade entre 2 e D).
enum class Metodo { COMPLETO, CRESCER };

// Gera árvores aleatórias válidas para um vocabulário.
//
// Guarda REFERÊNCIAS para o vocabulário e para o Aleatorio: os dois precisam
// continuar existindo enquanto o Gerador for usado (crie-os antes, na main).
class Gerador {
public:
    // Chance de um lugar de ação virar SE no método CRESCER.
    static constexpr double CHANCE_SE_NO_CRESCER = 0.5;
    // Chance de comparar com outra variável (quando existe uma compatível) em vez de constante.
    static constexpr double CHANCE_COMPARAR_COM_VARIAVEL = 0.5;
    // Quantas vezes tentar gerar uma árvore diferente das já criadas na população inicial.
    static constexpr int TENTATIVAS_SEM_REPETIR = 20;

    Gerador(const Vocabulario& vocab, Aleatorio& aleatorio);

    // Comparação aleatória: variável (>= ou >) outra variável compatível ou constante.
    Arvore gerarCondicao();
    // Folha CONTINUAR ou PARAR.
    Arvore gerarAcaoFolha();
    // Algo que cabe num lugar de ação (SE ou folha), com profundidade <= profundidadeMax.
    // Com COMPLETO e profundidadeMax >= 2, a profundidade é exatamente profundidadeMax.
    Arvore gerarSubarvoreAcao(int profundidadeMax, Metodo metodo);
    // Árvore inteira, sempre com SE na raiz. Exige profundidade >= 2.
    Arvore gerarArvore(int profundidade, Metodo metodo);

    // População inicial "ramped half-and-half": as profundidades de profMin a profMax
    // se revezam, e em cada profundidade metade é COMPLETO e metade CRESCER.
    // Evita árvores repetidas (até TENTATIVAS_SEM_REPETIR tentativas por árvore).
    std::vector<Arvore> gerarPopulacao(int tamanho, int profMin, int profMax);

    const Vocabulario& vocabulario() const { return vocab_; }
    Aleatorio& aleatorio() { return aleatorio_; }

private:
    const Vocabulario& vocab_;
    Aleatorio& aleatorio_;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_GERADOR_H
