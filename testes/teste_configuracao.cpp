// Testes da leitura do parametros.json (LeitorJSON + lerParametros).
#include <filesystem>
#include <fstream>
#include <optional>

#include "LeitorJSON.h"
#include "configuracao.h"
#include "nomes.h"
#include "teste.h"

using nlohmann::json;

// Grava `conteudo` num arquivo da pasta temporária do sistema e devolve o caminho.
static std::string arquivoTemporario(const std::string& nome, const std::string& conteudo) {
    const std::string caminho = (std::filesystem::temp_directory_path() / nome).string();
    std::ofstream f(caminho);
    f << conteudo;
    VERIFICAR(f.good());  // se não gravasse, o leitor falharia por "não abriu" (motivo errado)
    return caminho;
}

int main() {
    // ---- arquivos reais ----
    // Ordem dos argumentos: parametros, AVALIADOR, spec.
    const Opcoes o = LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_pig.json",
                                           "dataset/specs_regras_pig.json");
    const Parametros& p = o.parametros;  // já convertido e conferido pelo LeitorJSON
    VERIFICAR(p.populacao == 100);
    VERIFICAR(p.geracoes_max == 50);
    VERIFICAR(p.semente == 42);
    VERIFICAR(p.profundidade_min == 2 && p.profundidade_max == 5 && p.profundidade_limite_filhos == 6);
    VERIFICAR(p.selecao.torneio_tamanho == 3 && p.selecao.elitismo == 2);
    VERIFICAR(p.cruzamento.probabilidade == 0.85 && p.cruzamento.prob_ponto_interno == 0.9);
    VERIFICAR(p.mutacao.probabilidade == 0.05 && p.mutacao.prob_so_constante == 0.5);
    VERIFICAR(p.mutacao.profundidade_subarvore == 2 && p.mutacao.variacao_constante == 3.0);
    VERIFICAR(p.parada.winrate_alvo == 0.60 && p.parada.geracoes_sem_melhora == 10);
    VERIFICAR(p.parada.partidas_confirmacao == 20000);

    // Avaliador e spec do Pig.
    VERIFICAR(o.avaliador.partidas_por_arvore == 500 && o.avaliador.alternar_quem_comeca);
    VERIFICAR(o.avaliador.fitness == nomes::FITNESS_MINIMO_DESEMPATE_MEDIA);
    VERIFICAR(o.avaliador.adversarios.size() == 2);
    VERIFICAR(o.avaliador.adversarios[0].tipo == nomes::ALEATORIO && !o.avaliador.adversarios[0].valor);
    VERIFICAR(o.avaliador.adversarios[1].tipo == nomes::LIMIAR);
    VERIFICAR(o.avaliador.adversarios[1].variavel == std::optional<std::string>("pontos_turno"));
    VERIFICAR(o.avaliador.adversarios[1].valor == std::optional<int>(20));
    VERIFICAR(o.vocabulario.variaveis.size() == 3 && o.vocabulario.indiceDe("pontos_turno") == 0);
    // Opcoes::regras é só o bloco "regras" (o nome "jogo" e o vocabulário ficam de fora).
    VERIFICAR(o.regras.at("pontos_para_vencer") == 100 && !o.regras.contains(nomes::VARIAVEIS) && !o.regras.contains("jogo"));

    // Blackjack.
    const Opcoes ob = LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_blackjack.json",
                                            "dataset/specs_regras_blackjack.json");
    VERIFICAR(ob.regras.at("limite_de_estouro") == 21);
    VERIFICAR(ob.regras.at("empate") == nomes::EMPATE_MEIO_PONTO);
    VERIFICAR(ob.avaliador.adversarios.size() == 3);
    VERIFICAR(ob.avaliador.adversarios[2].tipo == nomes::SUPERAR);
    VERIFICAR(ob.avaliador.adversarios[2].limiar_se_primeiro == std::optional<int>(17));
    VERIFICAR(ob.vocabulario.indiceDe("sou_segundo") == 2);

    // A ordem antiga (parametros, spec, avaliador) tem de falhar, não passar calada.
    // E a mensagem diz qual arquivo está no lugar errado.
    try {
        LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/specs_regras_pig.json", "dataset/avaliador_pig.json");
        VERIFICAR(false);
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        VERIFICAR_MSG(msg.find("dataset/specs_regras_pig.json") != std::string::npos, msg);
    }

    // ---- LeitorJSON: arquivo que não existe ----
    VERIFICAR_LANCA(std::runtime_error, LeitorJSON::lerOpcoes("dataset/nao_existe.json", "dataset/avaliador_pig.json",
                                                              "dataset/specs_regras_pig.json"));

    // ---- LeitorJSON: json malformado (vírgula sobrando) ----
    // Agora vira std::runtime_error e a mensagem diz qual arquivo está quebrado.
    {
        const std::string malformado = arquivoTemporario("pg_malformado.json", "{ \"populacao\": 100, }");
        std::string mensagem;
        try {
            LeitorJSON::lerOpcoes(malformado, "dataset/avaliador_pig.json", "dataset/specs_regras_pig.json");
        } catch (const std::runtime_error& e) {
            mensagem = e.what();
        }
        std::filesystem::remove(malformado);
        VERIFICAR_MSG(mensagem.find("parse error") != std::string::npos, mensagem);  // falhou por estar malformado
        VERIFICAR_MSG(mensagem.find(malformado) != std::string::npos, mensagem);     // e diz qual arquivo
    }

    // ---- LeitorJSON passa pela conferência: 100.7 é recusado (antes virava 100) ----
    {
        json j = teste::lerJson("dataset/parametros.json");
        j["populacao"] = 100.7;
        const std::string caminho = arquivoTemporario("pg_parametros_quebrado.json", j.dump());
        std::string mensagem;
        try {
            LeitorJSON::lerOpcoes(caminho, "dataset/avaliador_pig.json", "dataset/specs_regras_pig.json");
        } catch (const std::runtime_error& e) {
            mensagem = e.what();
        }
        std::filesystem::remove(caminho);
        VERIFICAR_MSG(mensagem.find("/populacao") != std::string::npos && mensagem.find(caminho) != std::string::npos,
                      mensagem);
    }

    // ---- LeitorJSON passa pela validação da spec: ação desconhecida é recusada ----
    {
        json j = teste::lerJson("dataset/specs_regras_pig.json");
        j["acoes"] = {"CONTINUAR", "PULAR"};
        const std::string caminho = arquivoTemporario("pg_spec_quebrada.json", j.dump());
        VERIFICAR_LANCA(std::runtime_error,
                        LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_pig.json", caminho));
        j = teste::lerJson("dataset/specs_regras_pig.json");
        j.erase("regras");
        std::ofstream(caminho) << j.dump();
        VERIFICAR_LANCA(std::runtime_error,
                        LeitorJSON::lerOpcoes("dataset/parametros.json", "dataset/avaliador_pig.json", caminho));
        std::filesystem::remove(caminho);
    }

    // ---- valores errados: todos recusados ----
    const json base = teste::lerJson("dataset/parametros.json");
    auto alterado = [&](auto alterar) {
        json j = base;
        alterar(j);
        return j;
    };
    VERIFICAR_LANCA(std::runtime_error, lerParametros(json::array()));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["populacao"] = 100.5; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["populacao"] = true; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["populacao"] = "100"; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["populacao"] = 0; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["popolacao"] = 100; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["mutacao"]["prob_constante"] = 0.5; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j.erase("semente"); })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["semente"] = -1; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["selecao"]["elitismo"] = 100; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["selecao"]["elitismo"] = -1; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["selecao"]["torneio_tamanho"] = 0; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["cruzamento"]["probabilidade"] = 1.5; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["mutacao"]["probabilidade"] = -0.1; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["profundidade_min"] = 1; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["profundidade_min"] = 6; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["profundidade_max"] = 7; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["mutacao"]["variacao_constante"] = 2.5; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["mutacao"]["variacao_constante"] = 0; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["mutacao"]["profundidade_subarvore"] = 0; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["parada"]["winrate_alvo"] = 60; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["parada"]["geracoes_sem_melhora"] = 0; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["parada"]["partidas_confirmacao"] = -2; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["parada"]["partidas_confirmacao"] = 999; })));
    VERIFICAR_LANCA(std::runtime_error, lerParametros(alterado([](json& j) { j["parada"].erase("partidas_confirmacao"); })));
    VERIFICAR(lerParametros(alterado([](json& j) { j["parada"]["partidas_confirmacao"] = 0; })).parada.partidas_confirmacao == 0);

    // 100.0 é um inteiro escrito com ponto: aceito.
    VERIFICAR(lerParametros(alterado([](json& j) { j["populacao"] = 100.0; })).populacao == 100);

    // Demonstração do problema que a conferência de ida e volta resolve:
    // só com a macro do nlohmann, 100.7 vira 100 sem nenhum aviso, e chave errada é ignorada.
    VERIFICAR(alterado([](json& j) { j["populacao"] = 100.7; }).get<Parametros>().populacao == 100);
    VERIFICAR(alterado([](json& j) { j["popolacao"] = 7; }).get<Parametros>().populacao == 100);

    // A mensagem lista todos os problemas de uma vez.
    try {
        lerParametros(alterado([](json& j) {
            j["selecao"]["elitismo"] = 500;
            j["cruzamento"]["probabilidade"] = 2;
        }));
        VERIFICAR(false);
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        VERIFICAR_MSG(msg.find("elitismo") != std::string::npos && msg.find("cruzamento") != std::string::npos, msg);
    }
    // E a de chave desconhecida diz qual é.
    try {
        lerParametros(alterado([](json& j) { j["popolacao"] = 7; }));
        VERIFICAR(false);
    } catch (const std::runtime_error& e) {
        const std::string msg = e.what();
        VERIFICAR_MSG(msg.find("/popolacao") != std::string::npos, msg);
    }

    return teste::fim("configuracao");
}
