#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_EXPERIMENTO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_EXPERIMENTO_H

#include <optional>
#include <ostream>
#include <string>
#include <vector>

// Uma rodada de experimento: um jogo, uma semente, uma configuração
// (ou uma rodada por limite de profundidade, se vier uma lista em --limite).
// Lê os 3 json, aplica as trocas pedidas na linha de comando (sem editar os arquivos),
// roda o motor e grava, em <saida>/<jogo>/<rotulo>/:
//   semente_<n>.csv          histórico por geração (escreverCsv)
//   semente_<n>_campeao.txt  a árvore campeã e as medidas dela
//   semente_<n>.resumo.csv   uma linha com o resultado da rodada (o script da bateria junta todas)
//
// Este módulo faz o papel da "main": é o único lugar que conhece os nomes dos jogos.
struct ConfigExperimento {
    std::string jogo;  // "pig" ou "blackjack"
    std::string parametros = "dataset/parametros.json";
    std::string avaliador;  // vazio = dataset/avaliador_<jogo>.json
    std::string spec;       // vazio = dataset/specs_regras_<jogo>.json
    std::string saida = "resultados";
    std::string rotulo;     // vazio = "limite_<L>_partidas_<P>" (só pode ser trocado com um limite)

    // trocas opcionais (sem valor = fica o do json)
    std::optional<int> semente, partidas, confirmacao, geracoes, semMelhora, populacao;
    std::optional<double> alvo;
    // limites de profundidade (profundidade_limite_filhos), na ordem em que serão rodados.
    // Vazio = o do json. "--limite 3,5,7,10" vira {3, 5, 7, 10}: uma rodada por limite.
    std::vector<int> limites;
};

// Texto de ajuda com todas as opções.
std::string usoExperimento();

// Lê "--jogo pig --semente 3 ..." (sem o nome do programa). Lança std::runtime_error
// com uma mensagem clara para opção desconhecida, valor faltando ou número inválido.
ConfigExperimento lerArgumentos(const std::vector<std::string>& argumentos);

// Roda uma rodada por limite (ou uma só, com o limite do json) e grava os arquivos.
// Escreve uma linha de progresso por rodada em `log`. Devolve os caminhos dos resumos,
// na ordem dos limites. Confere todos os limites ANTES de rodar o primeiro: um limite
// inválido no fim da lista não desperdiça as rodadas anteriores. Lança std::runtime_error.
std::vector<std::string> rodarExperimento(const ConfigExperimento& config, std::ostream& log);

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_EXPERIMENTO_H
