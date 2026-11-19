// Testes do leitor de vocabulário: specs reais e specs quebradas de propósito.
#include "LeitorJSON.h"
#include "teste.h"
#include "vocabulario.h"

using nlohmann::json;

int main() {
    // ---- spec do Pig, lida pelo LeitorJSON do grupo ----
    // (o LeitorJSON chama lerVocabulario por dentro; ordem: parametros, avaliador, spec)
    const Opcoes pig = LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_pig.json",
                                             "dataset/specs_regras_pig.json");
    const Vocabulario& vp = pig.vocabulario;
    VERIFICAR(vp.variaveis.size() == 3);
    VERIFICAR(vp.indiceDe("pontos_turno") == 0);
    VERIFICAR(vp.indiceDe("meu_placar") == 1);
    VERIFICAR(vp.indiceDe("placar_adv") == 2);
    VERIFICAR(vp.variaveis[0].min == 0 && vp.variaveis[0].max == 100);
    VERIFICAR(vp.acoes.size() == 2);
    VERIFICAR(vp.comparacoes.size() == 2);
    VERIFICAR(vp.compativeis(0, 1) && vp.compativeis(1, 2) && vp.compativeis(2, 0));
    VERIFICAR(!vp.compativeis(0, 0));
    VERIFICAR_LANCA(std::runtime_error, vp.indiceDe("turno"));

    // ---- spec do blackjack ----
    const Vocabulario vb = lerVocabulario(teste::lerJson("dataset/specs_regras_blackjack.json"));
    const int MS = vb.indiceDe("minha_soma"), SA = vb.indiceDe("soma_adv"), S2 = vb.indiceDe("sou_segundo");
    VERIFICAR(vb.compativeis(MS, SA));
    VERIFICAR(!vb.compativeis(MS, S2) && !vb.compativeis(S2, SA) && !vb.compativeis(S2, S2));

    // ---- faixa das constantes ----
    {
        const Faixa a = faixaConstante(vp.variaveis[0], Comparacao::MAIOR_IGUAL);
        const Faixa b = faixaConstante(vp.variaveis[0], Comparacao::MAIOR);
        VERIFICAR(a.min == 1 && a.max == 100);
        VERIFICAR(b.min == 0 && b.max == 99);
        const Faixa c = faixaConstante(vb.variaveis[static_cast<std::size_t>(S2)], Comparacao::MAIOR_IGUAL);
        const Faixa d = faixaConstante(vb.variaveis[static_cast<std::size_t>(S2)], Comparacao::MAIOR);
        VERIFICAR(c.min == 1 && c.max == 1);  // sou_segundo >= 1
        VERIFICAR(d.min == 0 && d.max == 0);  // sou_segundo > 0
    }

    // ---- specs quebradas: todas devem ser recusadas com std::runtime_error ----
    const json base = teste::lerJson("dataset/specs_regras_pig.json");
    auto quebrada = [&](auto alterar) {
        json j = base;
        alterar(j);
        return j;
    };
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(json::array()));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j.erase("variaveis"); })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"] = json::array(); })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"] = "x"; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0]["min"] = "0"; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0]["min"] = 1.5; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0]["min"] = true; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0]["min"] = 100; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0]["max"] = 3000000000u; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0].erase("max"); })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0]["nome"] = ""; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][1]["nome"] = "pontos_turno"; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["variaveis"][0] = 5; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["acoes"] = {"CONTINUAR"}; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["acoes"] = {"CONTINUAR", "PULAR"}; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["acoes"] = {"continuar", "PARAR"}; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["acoes"] = {"CONTINUAR", "PARAR", "PARAR"}; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["comparacoes"] = {"<"}; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["comparacoes"] = {">=", ">="}; })));
    VERIFICAR_LANCA(std::runtime_error, lerVocabulario(quebrada([](json& j) { j["comparacoes"] = json::array(); })));

    // A mensagem diz onde está o problema.
    try {
        lerVocabulario(quebrada([](json& j) { j["variaveis"][2]["min"] = 1.5; }));
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        VERIFICAR_MSG(msg.find("variaveis[2].min") != std::string::npos, msg);
    }

    // Uma única comparação ainda é aceita (o vocabulário pode ser menor).
    VERIFICAR(lerVocabulario(quebrada([](json& j) { j["comparacoes"] = {">"}; })).comparacoes.size() == 1);

    return teste::fim("vocabulario");
}
