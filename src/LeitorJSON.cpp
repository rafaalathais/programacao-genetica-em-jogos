//
// Created by leon on 14/11/2026.
//

#include "../include/LeitorJSON.h"

#include <iostream>
nlohmann::json LeitorJSON::lerJSON(const std::string& caminho) {
    std::ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        throw std::runtime_error("ERRO: Não foi possível abriro arquivo. Caminho: " + caminho);
    }

    nlohmann::json dados;
    arquivo >> dados;
    return dados;
}

Opcoes LeitorJSON::lerOpcoes(const std::string& caminhoParametros, const std::string& caminhoRegras, const std::string& caminhoAvaliador){
    return {lerJSON(caminhoParametros), lerJSON(caminhoRegras), lerJSON(caminhoAvaliador)};
}
