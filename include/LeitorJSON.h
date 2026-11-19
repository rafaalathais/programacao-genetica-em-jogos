//
// Created by leon on 14/11/2026.
//

#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
#include "parametros.h"
#include "criterioAvaliador.h"
#include "nlohmann/json.hpp"

#include "vocabulario.h"

struct Opcoes {
    Parametros parametros;
    CriterioAvaliador avaliador;
    Vocabulario vocabulario;
    nlohmann::json regras;
};

class LeitorJSON {
private:
    static nlohmann::json lerJSON(const std::string& caminho);
    static Parametros lerParametros(const std::string& caminho);
    static CriterioAvaliador lerCriterio(const std::string& caminho);
    static std::tuple<Vocabulario,nlohmann::json> lerSpec(const std::string& caminho);

public:
    static Opcoes lerOpcoes(const std::string& caminhoParametros, const std::string& caminhoAvaliador, const std::string& caminhoSpec);
};


#endif //PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
