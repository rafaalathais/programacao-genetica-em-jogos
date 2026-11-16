#include <iostream>

#include "../include/LeitorJSON.h"

using namespace std;

const char *ARQUIVOPARAMETROS("../dataset/parametros.json");

int main(int argc, char *argv[]) {

    nlohmann::json json;
    try {
        json = LeitorJSON::lerParametros(ARQUIVOPARAMETROS);
    } catch (const std::exception& e) {
        cerr << e.what() << endl;
    }

    //MotorEvolutivo motor(Parametros,Regras,Criterios);
    //motor.iniciar();
}

