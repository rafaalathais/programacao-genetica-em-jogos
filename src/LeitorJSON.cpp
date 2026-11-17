//
// Created by leon on 14/11/2026.
//

#include "../include/LeitorJSON.h"
#include <iostream>
#include <fstream>

#include "configuracao.h"
#include "nomes.h"

using namespace std;
using namespace nlohmann;


json LeitorJSON::lerJSON(const string& caminho) {

    std::ifstream arquivo(caminho);

    if (!arquivo.is_open()) {
        throw std::runtime_error("ERRO: Não foi possível abrir o arquivo. Caminho: " + caminho);
    }

    // Sem o try, um json malformado lança parse_error dizendo só linha e coluna,
    // sem dizer QUAL dos três arquivos está quebrado.
    try {
        json dados;
        arquivo >> dados;
        return dados;
    } catch (const json::parse_error& e) {
        throw std::runtime_error("ERRO: json malformado em " + caminho + ": " + e.what());
    }

}

// As conversões ficam nas funções livres ::lerParametros (configuracao.h) e
// lerVocabulario (vocabulario.h), que conferem cada valor. O "::" é obrigatório:
// sem ele, dentro da classe, "lerParametros" chamaria este próprio método.
Parametros LeitorJSON::lerParametros(const string& caminho) {
    json dados = lerJSON(caminho);
    try {
        return ::lerParametros(dados);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(caminho + ": " + e.what());
    }
}

CriterioAvaliador LeitorJSON::lerCriterio(const string& caminho) {
    json dados = lerJSON(caminho);
    CriterioAvaliador criterio;
    try {
        criterio = dados.get<CriterioAvaliador>();
    } catch (const json::exception& e) {
        // sem isso a mensagem seria só "key 'partidas_por_arvore' not found", sem dizer o arquivo
        throw std::runtime_error(caminho + ": " + e.what());
    }
    try {
        validarCriterio(criterio);  // configuracao.h
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(caminho + ": " + e.what());
    }
    return criterio;
}

std::tuple<Vocabulario, json> LeitorJSON::lerSpec(const string& caminho) {
    json dados = lerJSON(caminho);
    Vocabulario vocabulario;
    try {
        vocabulario = lerVocabulario(dados);
    } catch (const std::runtime_error& e) {
        throw std::runtime_error(caminho + ": " + e.what());
    }
    if (!dados.contains(nomes::REGRAS) || !dados.at(nomes::REGRAS).is_object()) {
        throw std::runtime_error(caminho + ": falta o objeto \"" + string(nomes::REGRAS) + "\"");
    }
    json regras = dados.at(nomes::REGRAS);
    return std::make_tuple(vocabulario, regras);
}


Opcoes LeitorJSON::lerOpcoes(const string& caminhoParametros, const string& caminhoAvaliador, const string& caminhoSpec){
    Parametros parametros = lerParametros(caminhoParametros);
    CriterioAvaliador criterio = lerCriterio(caminhoAvaliador);
    auto [vocabulario, regras] = lerSpec(caminhoSpec);

    return {parametros, criterio, vocabulario, regras};
}
