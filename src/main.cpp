#include "../include/LeitorJSON.h"

using namespace std;

const char *ARQUIVOPARAMETROS("../dataset/parametros.json");

int main(int argc, char *argv[]) {

    Parametros parametros = LeitorJSON::lerParametros(ifstream(ARQUIVOPARAMETROS));

    //MotorEvolutivo motor(Parametros,Regras,Criterios);
    //motor.iniciar();
}

