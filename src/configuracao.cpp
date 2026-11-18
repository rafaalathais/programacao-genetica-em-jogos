#include "configuracao.h"

#include <cmath>
#include <stdexcept>
#include <string>
#include <vector>

#include "nomes.h"

Parametros lerParametros(const nlohmann::json& json) {
    Parametros p;
    try {
        // Usa as macros NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE do parametros.h.
        // Chave faltando ou tipo incompatível (ex.: texto num campo int) lança aqui.
        p = json.get<Parametros>();
    } catch (const nlohmann::json::exception& e) {
        throw std::runtime_error(std::string("parametros: ") + e.what());
    }

    // Conferência de ida e volta (ver comentário no .h).
    // Obs.: 3 e 3.0 são considerados iguais pelo nlohmann, então "variacao_constante": 3
    // (inteiro no json, double na struct) passa sem problema.
    const nlohmann::json volta = p;
    if (volta != json) {
        std::string problemas;
        // diff(volta, json) lista as operações que transformariam `volta` em `json`.
        for (const nlohmann::json& op : nlohmann::json::diff(volta, json)) {
            const std::string tipo = op.at("op").get<std::string>();
            const std::string caminho = op.at("path").get<std::string>();
            if (!problemas.empty()) problemas += "; ";
            if (tipo == "add") {
                problemas += caminho + " (chave desconhecida)";
            } else {
                const nlohmann::json original = json.at(nlohmann::json::json_pointer(caminho));
                problemas += caminho + " (o valor " + original.dump() + " não cabe no tipo do campo)";
            }
        }
        throw std::runtime_error("parametros: " + problemas);
    }

    validarParametros(p);
    return p;
}

void validarParametros(const Parametros& p) {
    std::vector<std::string> problemas;
    auto exigir = [&](bool ok, const std::string& mensagem) {
        if (!ok) problemas.push_back(mensagem);
    };
    // `x >= 0 && x <= 1` também recusa NaN (toda comparação com NaN é falsa).
    auto probabilidade = [&](double x, const std::string& nome) {
        exigir(x >= 0.0 && x <= 1.0, nome + " deveria estar entre 0 e 1");
    };

    exigir(p.populacao >= 1, "populacao deveria ser pelo menos 1");
    exigir(p.geracoes_max >= 1, "geracoes_max deveria ser pelo menos 1");
    // A semente vai para um std::uint32_t; negativa viraria outro número sem aviso.
    exigir(p.semente >= 0, "semente deveria ser >= 0");

    // Um SE ocupa 2 níveis (SE -> comparação -> variável), então a menor árvore tem profundidade 2.
    exigir(p.profundidade_min >= 2, "profundidade_min deveria ser pelo menos 2 (um SE ocupa 2 níveis)");
    exigir(p.profundidade_min <= p.profundidade_max, "profundidade_min deveria ser <= profundidade_max");
    exigir(p.profundidade_max <= p.profundidade_limite_filhos,
           "profundidade_max deveria ser <= profundidade_limite_filhos");

    exigir(p.selecao.torneio_tamanho >= 1, "selecao.torneio_tamanho deveria ser pelo menos 1");
    exigir(p.selecao.torneio_tamanho <= p.populacao, "selecao.torneio_tamanho deveria ser <= populacao");
    // Elite igual à população não deixaria espaço para nenhum filho novo.
    exigir(p.selecao.elitismo >= 0 && p.selecao.elitismo < p.populacao,
           "selecao.elitismo deveria estar entre 0 e populacao - 1");

    probabilidade(p.cruzamento.probabilidade, "cruzamento.probabilidade");
    probabilidade(p.cruzamento.prob_ponto_interno, "cruzamento.prob_ponto_interno");
    probabilidade(p.mutacao.probabilidade, "mutacao.probabilidade");
    probabilidade(p.mutacao.prob_so_constante, "mutacao.prob_so_constante");
    exigir(p.mutacao.profundidade_subarvore >= 1, "mutacao.profundidade_subarvore deveria ser pelo menos 1");
    // As constantes são inteiras; a struct guarda double, então conferimos aqui.
    const double v = p.mutacao.variacao_constante;
    exigir(std::isfinite(v) && v >= 1.0 && std::floor(v) == v,
           "mutacao.variacao_constante deveria ser um inteiro >= 1");

    probabilidade(p.parada.winrate_alvo, "parada.winrate_alvo");
    exigir(p.parada.geracoes_sem_melhora >= 1, "parada.geracoes_sem_melhora deveria ser pelo menos 1");
    // par: na confirmação cada lado também começa metade das partidas.
    exigir(p.parada.partidas_confirmacao >= 0 && p.parada.partidas_confirmacao % 2 == 0,
           "parada.partidas_confirmacao deveria ser 0 (sem confirmação) ou um número par positivo");

    if (!problemas.empty()) {
        std::string mensagem = "parametros inválidos:";
        for (const std::string& problema : problemas) mensagem += "\n  - " + problema;
        throw std::runtime_error(mensagem);
    }
}

void validarCriterio(const CriterioAvaliador& c) {
    std::vector<std::string> problemas;
    if (c.partidas_por_arvore < 1) problemas.push_back("partidas_por_arvore deveria ser pelo menos 1");
    if (c.alternar_quem_comeca && c.partidas_por_arvore % 2 != 0)
        problemas.push_back("partidas_por_arvore deveria ser par (cada lado começa metade das vezes)");
    if (c.fitness != nomes::FITNESS_MINIMO_DESEMPATE_MEDIA)
        problemas.push_back("fitness \"" + c.fitness + "\" desconhecido (esperado \"" +
                            nomes::FITNESS_MINIMO_DESEMPATE_MEDIA + "\")");
    if (c.adversarios.empty()) problemas.push_back("adversarios deveria ter pelo menos um adversário");
    for (std::size_t i = 0; i < c.adversarios.size(); ++i) {
        const std::string& t = c.adversarios[i].tipo;
        if (t != nomes::ALEATORIO && t != nomes::LIMIAR && t != nomes::SUPERAR)
            problemas.push_back("adversarios[" + std::to_string(i) + "]: tipo \"" + t + "\" desconhecido");
    }
    if (!problemas.empty()) {
        std::string mensagem = "avaliador inválido:";
        for (const std::string& problema : problemas) mensagem += "\n  - " + problema;
        throw std::runtime_error(mensagem);
    }
}
