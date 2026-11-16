//
// Created by leon on 14/11/2026.
//

#include "../include/LeitorJSON.h"
#include <iostream>
#include <fstream>

using namespace std;
using namespace nlohmann;


json LeitorJSON::lerJSON(const string& caminho) {

    std::ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        throw std::runtime_error("ERRO: Não foi possível abrir o arquivo. Caminho: " + caminho);
    }

    json dados;
    arquivo >> dados;
    return dados;

}

Parametros LeitorJSON::lerParametros(const string& caminho) {
    json dados = lerJSON(caminho);
    Parametros parametros = dados.get<Parametros>();
    return parametros;
}

CriterioAvaliador LeitorJSON::lerCriterio(const string& caminho) {
    json dados = lerJSON(caminho);
    CriterioAvaliador criterio = dados.get<CriterioAvaliador>();
    return criterio;
}

std::tuple<Vocabulario, json> LeitorJSON::lerSpec(const string& caminho) {
    json dados = lerJSON(caminho);
    Vocabulario vocabulario = dados.get<Vocabulario>();
    json regras = dados.at("regras");
    return std::make_tuple(vocabulario, regras);
}


Opcoes LeitorJSON::lerOpcoes(const string& caminhoParametros, const string& caminhoAvaliador, const string& caminhoSpec){
    Parametros parametros = lerParametros(caminhoParametros);
    CriterioAvaliador criterio = lerCriterio(caminhoAvaliador);
    auto [vocabulario, regras] = lerSpec(caminhoSpec);

    return {parametros, criterio, vocabulario, regras};
}
