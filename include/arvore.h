#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_ARVORE_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_ARVORE_H

#include <memory>
#include <string>
#include <unordered_set>
#include <vector>

#include "tipos.h"
#include "vocabulario.h"

// Árvore de decisão fortemente tipada.
//
//   SE(condição, ação, ação)                     -> devolve uma ação
//   COMPARACAO(variável, variável ou constante)  -> devolve verdadeiro/falso
//   VARIAVEL, CONSTANTE                          -> operandos da comparação (folhas)
//   ACAO                                         -> folha: CONTINUAR ou PARAR
//
// "Lugar de ação" (a raiz e os 2º e 3º filhos de um SE) aceita SE ou ACAO.
// "Lugar de condição" (o 1º filho de um SE) aceita só COMPARACAO.
// Exemplo:  SE(pontos_turno >= 20, PARAR, CONTINUAR)
//
//   SE                              profundidade 0
//   +-- >=                          profundidade 1
//   |   +-- pontos_turno            profundidade 2
//   |   +-- 20                      profundidade 2
//   +-- PARAR                       profundidade 1
//   +-- CONTINUAR                   profundidade 1   -> a árvore tem profundidade 2
// (cuidado: uma barra invertida no fim de um comentário "//" emenda a linha de baixo)
enum class TipoNo { SE, COMPARACAO, VARIAVEL, CONSTANTE, ACAO };

struct No;
// Cada nó é dono dos filhos (unique_ptr): apagar a raiz apaga a árvore inteira,
// e um nó nunca pertence a duas árvores ao mesmo tempo.
using Arvore = std::unique_ptr<No>;

struct No {
    TipoNo tipo = TipoNo::ACAO;
    // O significado de `valor` depende do tipo:
    //   VARIAVEL   -> índice da variável no vocabulário
    //   CONSTANTE  -> o número
    //   ACAO       -> static_cast<int>(Acao)
    //   COMPARACAO -> static_cast<int>(Comparacao)
    //   SE         -> não usado (0)
    int valor = 0;
    // SE: 3 filhos (condição, então, senão). COMPARACAO: 2 (esquerda, direita). Folhas: 0.
    std::vector<Arvore> filhos;
};

// Estado do jogo visto pela árvore: um valor por variável, na ordem do vocabulário.
using Estado = std::vector<int>;

// Construtores de nós.
Arvore criarSe(Arvore condicao, Arvore entao, Arvore senao);
Arvore criarComparacao(Comparacao op, Arvore esquerda, Arvore direita);
Arvore criarVariavel(int indice);
Arvore criarConstante(int valor);
Arvore criarAcao(Acao acao);

// Percorre a árvore com o estado e devolve a decisão.
// Lança std::out_of_range se o estado tiver menos valores que o vocabulário.
Acao avaliar(const No& arvore, const Estado& estado);

// Igual a avaliar, mas anota em `visitados` os nós por onde a decisão passou
// (os SEs do caminho e a folha final). Serve para medir o bloat: nós que nenhuma
// decisão real visita não mudam o comportamento da árvore.
Acao avaliarMarcando(const No& arvore, const Estado& estado, std::unordered_set<const No*>& visitados);

// "Nós efetivos": quantos nós da árvore participam de alguma decisão anotada em
// `visitados` (cada SE visitado conta com a sua comparação inteira; folhas, 1 cada).
// Sempre <= tamanho(arvore).
int nosEfetivos(const No& arvore, const std::unordered_set<const No*>& visitados);

// Cópia profunda: cria nós novos para tudo (a cópia não divide nada com o original).
Arvore copiar(const No& arvore);

int profundidade(const No& arvore);  // em arestas: uma folha sozinha tem profundidade 0
int tamanho(const No& arvore);       // número de nós

// Texto legível, ex.: "SE(pontos_turno >= 20, PARAR, CONTINUAR)".
std::string paraTexto(const No& arvore, const Vocabulario& vocab);

// Confere a tipagem e os valores (índices, faixas das constantes, variáveis
// compatíveis). Devolve "" se a árvore for válida, ou a descrição do primeiro erro.
std::string validar(const No& arvore, const Vocabulario& vocab);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_ARVORE_H
