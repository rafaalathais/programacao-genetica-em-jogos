#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_ESTATISTICAS_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_ESTATISTICAS_H

#include <string>
#include <vector>

#include "fitness.h"
#include "individuo.h"
#include "operadores.h"
#include "vocabulario.h"

// Números de uma geração, para acompanhar a evolução e o bloat (crescimento das árvores).
struct EstatisticasGeracao {
    int geracao = 0;
    int individuos = 0;                 // tamanho da população (tem de ser sempre o mesmo)

    // o melhor da geração (pela comparação do Fitness: pior winrate, depois média)
    Fitness melhor;
    std::vector<double> winratesMelhor;  // um por adversário, na ordem do avaliador
    int nosMelhor = 0;
    int profundidadeMelhor = 0;

    // a população inteira
    double mediaMinimo = 0;   // média do "pior winrate" dos indivíduos
    double mediaMedia = 0;    // média da "média dos winrates"
    double nosMedio = 0;
    int nosMax = 0;
    double profundidadeMedia = 0;
    int profundidadeMax = 0;
    int distintas = 0;        // árvores diferentes entre si (diversidade)

    // o que os operadores fizeram para criar esta geração (0 na geração 0)
    long cruzamentos = 0;
    long mutacoes = 0;
    int rejeitadas = 0;       // candidatas ao alvo que a confirmação recusou nesta geração

    double segundos = 0;      // tempo para criar e avaliar a geração (único campo que muda entre execuções)
};

// Calcula as estatísticas de uma população já avaliada.
EstatisticasGeracao calcularEstatisticas(int geracao, const std::vector<Individuo>& populacao,
                                         const Vocabulario& vocab);

// Grava o histórico em CSV (separador vírgula, ponto decimal), uma linha por geração.
// `adversarios` dá o nome das colunas de winrate do melhor (ex.: "aleatorio", "limiar").
// Lança std::runtime_error se não conseguir gravar.
void escreverCsv(const std::string& caminho, const std::vector<EstatisticasGeracao>& historico,
                 const std::vector<std::string>& adversarios);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_ESTATISTICAS_H
