//
// Created by leon on 14/11/2026.
//

#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
#include "parametros.h"
#include "nlohmann/json.hpp"
#include <fstream>

class LeitorJSON {
    public:
    static nlohmann::json lerParametros(const std::string& caminho);

    /*
     * Como os JSON de regras e de avaliadores dos dois jogos são diferentes, as opções são:
     * - Criar um struct baseado no jogo com mais atributos, e aí os dados que não se aplicarem ao outro jogo ficam como Null
     * - Padronizar os JSON ao invés do struct
     */

    //static Regras lerRegras(std::ifstream arquivo);
    //static CriterioAvaliador lerCriterios(std::ifstream arquivo);

};


#endif //PROGRAMACAO_GENETICA_EM_JOGOS_LEITORJSON_H
