#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_TIPOS_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_TIPOS_H

#include <optional>
#include <string>

// Tipos básicos compartilhados pelo núcleo e pelos jogos.

// O que um jogador pode decidir. Todos os jogos do trabalho são de decisão binária.
enum class Acao { CONTINUAR, PARAR };

// Operadores de comparação que podem aparecer nas árvores.
enum class Comparacao { MAIOR_IGUAL, MAIOR };

// Conversão texto do json -> enum. Devolve std::nullopt se o texto não for
// conhecido; quem chama monta a mensagem de erro com o contexto (arquivo, posição).
std::optional<Acao> acaoDeTexto(const std::string& texto);
std::optional<Comparacao> comparacaoDeTexto(const std::string& texto);

// Conversão enum -> texto (para imprimir árvores e mensagens).
std::string textoDe(Acao acao);
std::string textoDe(Comparacao comparacao);

// Aplica a comparação: comparar(Comparacao::MAIOR_IGUAL, a, b) == (a >= b).
bool comparar(Comparacao comparacao, int a, int b);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_TIPOS_H
