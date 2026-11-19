#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_TESTE_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_TESTE_H

// Mini "framework" de testes, sem dependências: algumas macros que contam
// verificações e falhas e imprimem o arquivo e a linha do que falhou.

#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>

#include "nlohmann/json.hpp"

namespace teste {

inline int falhas = 0;
inline int verificacoes = 0;

inline void reportar(const char* arquivo, int linha, const std::string& texto) {
    ++falhas;
    if (falhas <= 30) std::cerr << arquivo << ":" << linha << ": FALHOU: " << texto << "\n";
    else if (falhas == 31) std::cerr << "(demais falhas omitidas)\n";
}

inline nlohmann::json lerJson(const std::string& caminho) {
    std::ifstream f(caminho);
    if (!f) throw std::runtime_error("teste: não abriu " + caminho);
    return nlohmann::json::parse(f);
}

// Imprime o resumo e devolve o código de saída do programa de teste.
inline int fim(const char* nome) {
    std::cout << nome << ": " << verificacoes << " verificações, " << falhas << " falhas\n";
    return falhas == 0 ? 0 : 1;
}

}  // namespace teste

#define VERIFICAR(cond)                                                    \
    do {                                                                   \
        ++teste::verificacoes;                                             \
        if (!(cond)) teste::reportar(__FILE__, __LINE__, #cond);           \
    } while (0)

// Igual a VERIFICAR, mas imprime uma mensagem extra (ex.: a árvore que falhou).
#define VERIFICAR_MSG(cond, msg)                                           \
    do {                                                                   \
        ++teste::verificacoes;                                             \
        if (!(cond)) {                                                     \
            std::ostringstream os_;                                        \
            os_ << #cond << " | " << msg;                                  \
            teste::reportar(__FILE__, __LINE__, os_.str());                \
        }                                                                  \
    } while (0)

// Verifica que o código lança uma exceção do tipo indicado.
#define VERIFICAR_LANCA(Excecao, ...)                                      \
    do {                                                                   \
        ++teste::verificacoes;                                             \
        bool lancou_ = false;                                              \
        try {                                                              \
            __VA_ARGS__;                                                   \
        } catch (const Excecao&) {                                         \
            lancou_ = true;                                                \
        } catch (...) {                                                    \
        }                                                                  \
        if (!lancou_)                                                      \
            teste::reportar(__FILE__, __LINE__,                            \
                            "não lançou " #Excecao ": " #__VA_ARGS__);     \
    } while (0)

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_TESTE_H
