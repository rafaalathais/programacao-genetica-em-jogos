//
// Created by leon on 16/11/2026.
// Juntado com a versão local (validação, indiceDe, compativeis, faixaConstante).
//

#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_VOCABULARIO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_VOCABULARIO_H

#include <string>
#include <vector>

#include "nlohmann/json.hpp"
#include "tipos.h"

// Uma variável do jogo que a árvore pode consultar, com a faixa de valores possíveis.
struct Variavel {
    std::string nome;
    int min;
    int max;
};

// Intervalo fechado [min, max].
struct Faixa {
    int min;
    int max;
};

// Vocabulário de um jogo: tudo o que o núcleo genético precisa saber da spec.
// O bloco "regras" da spec NÃO entra aqui; ele vai bruto para o simulador do jogo
// (Opcoes::regras, preenchido pelo LeitorJSON).
//
// Ações e comparações ficam como enum (Acao, Comparacao), não como texto: assim um
// erro de digitação no json ("continuar", "=>") é pego na leitura, e não no meio da
// evolução.
//
// A ordem das variáveis é a ordem do json. A árvore guarda o ÍNDICE da variável,
// e o simulador descobre esse índice pelo nome com indiceDe() — nunca por posição fixa.
//
// Não há NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE aqui de propósito: a macro aceitaria
// "min": 1.5 (vira 1) e ação desconhecida sem reclamar. A conversão é feita por
// lerVocabulario(), que confere cada campo.
struct Vocabulario {
    std::vector<Variavel> variaveis;
    std::vector<Acao> acoes;
    std::vector<Comparacao> comparacoes;

    // Índice da variável com esse nome. Lança std::runtime_error se não existir.
    int indiceDe(const std::string& nome) const;

    // Duas variáveis podem ser comparadas entre si se forem diferentes e tiverem
    // a mesma faixa (ex.: minha_soma com soma_adv). Comparar uma variável com ela
    // mesma (x >= x) dá sempre o mesmo resultado, então não é permitido.
    bool compativeis(int i, int j) const;
};

// Faixa de constantes que faz sentido comparar com a variável `v` usando `op`.
// Fica dentro de [min, max] da variável, mas exclui os valores em que a
// comparação daria sempre o mesmo resultado:
//   x >= min  é sempre verdadeiro  -> para ">=" a faixa é [min+1, max]
//   x >  max  é sempre falso       -> para ">"  a faixa é [min,   max-1]
Faixa faixaConstante(const Variavel& v, Comparacao op);

// Lê o vocabulário (variaveis, acoes, comparacoes) de uma spec já carregada.
// Confere tudo e lança std::runtime_error com uma mensagem dizendo o que está errado.
Vocabulario lerVocabulario(const nlohmann::json& spec);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_VOCABULARIO_H
