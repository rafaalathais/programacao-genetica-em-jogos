#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_OPERADORES_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_OPERADORES_H

#include "aleatorio.h"
#include "arvore.h"
#include "gerador.h"
#include "parametros.h"

// Operadores genéticos. Os dois:
//   - NUNCA alteram os pais (trabalham numa cópia profunda);
//   - sempre devolvem um filho válido com profundidade <= profundidade_limite_filhos;
//   - são aplicados SEMPRE que chamados. Sortear SE vai cruzar/mutar (com
//     cruzamento.probabilidade e mutacao.probabilidade) é trabalho do motor evolutivo,
//     uma vez por filho.
//
// Onde a troca pode acontecer (pontos de corte):
//   - lugar de ação (SE ou folha ACAO) só troca com lugar de ação;
//   - comparação inteira só troca com comparação inteira.
// Variáveis e constantes soltas NÃO são pontos de corte: trocar só o lado esquerdo
// de uma comparação poderia criar "sou_segundo > 19" (constante fora da faixa) ou
// comparar variáveis de faixas diferentes.

// Quantas vezes sortear pontos de corte antes de desistir ("descartar e refazer").
inline constexpr int TENTATIVAS_OPERADOR = 20;

// Contadores opcionais, para estatística e testes (quantas vezes desistiu etc.).
struct ContadoresOperadores {
    long cruzamentos = 0;
    long tentativasCruzamento = 0;
    long cruzamentosDesistidos = 0;   // nenhuma troca coube no limite: filho = cópia do pai1
    long mutacoes = 0;
    long mutacoesConstante = 0;
    long semConstanteMutavel = 0;     // sorteou "só constante" mas não havia constante que pudesse mudar
    long tentativasMutacao = 0;
    long mutacoesDesistidas = 0;      // nenhuma subárvore nova coube: filho = cópia do original
};

// Cruzamento de subárvore: o filho é uma cópia do pai1 com um pedaço trocado por
// uma cópia de um pedaço do mesmo tipo do pai2.
// Com prob_ponto_interno, o ponto escolhido é interno (SE ou comparação);
// senão é uma folha de ação. Se o resultado passar do limite, sorteia de novo.
// Lança std::invalid_argument se algum pai já estiver acima do limite.
Arvore cruzar(const No& pai1, const No& pai2, const Parametros& p, Aleatorio& aleatorio,
              ContadoresOperadores* contadores = nullptr);

// Mutação. Com prob_so_constante: soma ±1..±variacao_constante a uma constante
// (sem sair da faixa e sempre mudando o valor). Senão, ou se não houver constante
// que possa mudar: troca um ponto de corte por uma subárvore nova do mesmo tipo
// (de profundidade até profundidade_subarvore).
// Usa o Aleatorio do gerador.
Arvore mutar(const No& arvore, const Parametros& p, Gerador& gerador, ContadoresOperadores* contadores = nullptr);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_OPERADORES_H
