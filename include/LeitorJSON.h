//
// Created by leon on 14/11/2026.
//

#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
#include "parametros.h"
#include "nlohmann/json.hpp"
#include <fstream>

struct Opcoes {
    nlohmann::json parametros;
    nlohmann::json regras;
    nlohmann::json avaliador;
};

class LeitorJSON {
private:
    static nlohmann::json lerJSON(const std::string& caminho);

public:
    static Opcoes lerOpcoes(const std::string& caminhoParametros, const std::string &caminhoRegras,
                                const std::string& caminhoAvaliador);
};


#endif //PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
