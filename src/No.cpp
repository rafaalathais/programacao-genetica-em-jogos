#include "No.h"
#include <algorithm>
#include <stdexcept>

std::string No::avaliar(const Estado& estado) const {
    if (tipo == TipoNo::ACAO) {
        return valorStr; // "CONTINUAR" ou "PARAR"
    }

    if (tipo == TipoNo::SE) {
        // filhos[0] = comparador, filhos[1] = ramo verdadeiro, filhos[2] = ramo falso
        int valVar = estado.variaveis[filhos[0]->filhos[0]->valorNum];
        int valConst = filhos[0]->filhos[1]->valorNum;
        std::string op = filhos[0]->valorStr;

        bool condicao = false;
        if (op == ">=") condicao = (valVar >= valConst);
        else if (op == ">") condicao = (valVar > valConst);

        return condicao ? filhos[1]->avaliar(estado) : filhos[2]->avaliar(estado);
    }

    throw std::runtime_error("Avaliacao chamada em no invalido.");
}