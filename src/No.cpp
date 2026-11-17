#include "No.h"
#include <algorithm>
#include <stdexcept>

std::string No::avaliar(const Estado& estado) const {
    if (tipo == TipoNo::ACAO) {
        return valorStr; // Retorna "CONTINUAR" ou "PARAR"
    }

    if (tipo == TipoNo::SE) {
        // Estrutura esperada de um nó SE:
        // filhos[0] -> COMPARADOR (com filhos[0] = VARIAVEL e filhos[1] = CONSTANTE)
        // filhos[1] -> Ramo se Verdadeiro
        // filhos[2] -> Ramo se Falso
        if (filhos.size() < 3 || filhos[0]->filhos.size() < 2) {
            throw std::runtime_error("Estrutura invalida em no SE.");
        }

        int idxVar = filhos[0]->filhos[0]->valorNum;
        if (idxVar < 0 || idxVar >= static_cast<int>(estado.variaveis.size())) {
            throw std::out_of_range("Indice de variavel fora dos limites do Estado.");
        }

        int valVar = estado.variaveis[idxVar];
        int valConst = filhos[0]->filhos[1]->valorNum;
        const std::string& op = filhos[0]->valorStr;

        bool condicao = false;
        if (op == ">=") {
            condicao = (valVar >= valConst);
        } else if (op == ">") {
            condicao = (valVar > valConst);
        } else {
            throw std::runtime_error("Operador de comparacao desconhecido: " + op);
        }

        return condicao ? filhos[1]->avaliar(estado) : filhos[2]->avaliar(estado);
    }

    throw std::runtime_error("Avaliacao chamada em tipo de no nao suportado na raiz.");
}

int No::calcularProfundidade() const {
    if (filhos.empty()) {
        return 0; // Nó folha tem profundidade 0 (ou 1 se contar arestas/níveis, ajuste conforme convenção)
    }
    int maxFilho = 0;
    for (const auto& filho : filhos) {
        if (filho) {
            maxFilho = std::max(maxFilho, filho->calcularProfundidade());
        }
    }
    return 1 + maxFilho;
}

int No::contarNos() const {
    int total = 1; // Conta a si mesmo
    for (const auto& filho : filhos) {
        if (filho) {
            total += filho->contarNos();
        }
    }
    return total;
}