//
// Created by leon on 14/11/2026.
//

#include "../include/LeitorJSON.h"

#include <iostream>

Parametros LeitorJSON::lerParametros(std::ifstream arquivo) {
    nlohmann::json dados;
    arquivo >> dados;

    Parametros parametros = dados.get<Parametros>();

    return parametros;
}
