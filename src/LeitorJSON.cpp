//
// Created by leon on 14/11/2026.
//

#include "../include/LeitorJSON.h"

#include <iostream>

nlohmann::json LeitorJSON::lerParametros(const std::string &caminho) {
    std::ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        throw std::runtime_error("ERRO: Não foi possível abriro arquivo. Caminho: " + caminho);
    }

    nlohmann::json dados;
    arquivo >> dados;
    return dados;
}
