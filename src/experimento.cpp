#include "experimento.h"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <unordered_set>

#include "LeitorJSON.h"
#include "blackjack.h"
#include "configuracao.h"
#include "motor.h"
#include "pig.h"

namespace {

// Nomes dos jogos na linha de comando (só este módulo conhece os jogos pelo nome).
constexpr const char* JOGO_PIG = "pig";
constexpr const char* JOGO_BLACKJACK = "blackjack";

// Partidas por adversário usadas para medir os nós efetivos.
constexpr int PARTIDAS_NOS_EFETIVOS_CAMPEAO = 2000;
constexpr int PARTIDAS_NOS_EFETIVOS_POPULACAO = 200;

int lerInteiro(const std::string& opcao, const std::string& texto) {
    std::size_t usados = 0;
    int v = 0;
    try {
        v = std::stoi(texto, &usados);
    } catch (const std::exception&) {
        usados = 0;
    }
    if (usados == 0 || usados != texto.size())
        throw std::runtime_error("valor inválido para " + opcao + ": \"" + texto + "\" (esperava um inteiro)");
    return v;
}

double lerReal(const std::string& opcao, const std::string& texto) {
    std::size_t usados = 0;
    double v = 0;
    try {
        v = std::stod(texto, &usados);
    } catch (const std::exception&) {
        usados = 0;
    }
    if (usados == 0 || usados != texto.size())
        throw std::runtime_error("valor inválido para " + opcao + ": \"" + texto + "\" (esperava um número)");
    return v;
}

// Jogador que decide pela árvore e anota os nós visitados (para os nós efetivos).
class JogadorMarcador : public Jogador {
public:
    JogadorMarcador(const No& arvore, std::unordered_set<const No*>& visitados) : arvore_(&arvore), vis_(visitados) {}
    Acao decidir(const Estado& estado) override { return avaliarMarcando(*arvore_, estado, vis_); }

private:
    const No* arvore_;
    std::unordered_set<const No*>& vis_;
};

// Nós efetivos: joga `partidas` contra cada adversário anotando os nós usados nas decisões.
int medirNosEfetivos(Jogo& jogo, const No& arvore, std::vector<std::unique_ptr<Jogador>>& advs, int partidas) {
    std::unordered_set<const No*> vis;
    JogadorMarcador eu(arvore, vis);
    for (auto& adv : advs) jogarSerie(jogo, eu, *adv, partidas);
    return nosEfetivos(arvore, vis);
}

std::string csvTexto(const std::string& s) {  // entre aspas, dobrando as aspas internas
    std::string r = "\"";
    for (char c : s) r += c == '"' ? std::string("\"\"") : std::string(1, c);
    return r + "\"";
}

template <typename J>
std::string rodar(const ConfigExperimento& cfg, const Opcoes& o, Parametros p, CriterioAvaliador c, std::ostream& log) {
    namespace fs = std::filesystem;
    const Vocabulario& v = o.vocabulario;
    const std::string rotulo = cfg.rotulo.empty() ? "limite_" + std::to_string(p.profundidade_limite_filhos) +
                                                        "_partidas_" + std::to_string(c.partidas_por_arvore)
                                                  : cfg.rotulo;
    const fs::path pasta = fs::path(cfg.saida) / cfg.jogo / rotulo;
    std::error_code erro;
    fs::create_directories(pasta, erro);  // várias rodadas em paralelo podem criar a mesma pasta
    if (!fs::is_directory(pasta)) throw std::runtime_error("não foi possível criar a pasta " + pasta.string());
    const std::string base = (pasta / ("semente_" + std::to_string(p.semente))).string();

    const auto inicio = std::chrono::steady_clock::now();
    Aleatorio aleatorio(static_cast<std::uint32_t>(p.semente));
    J jogo(o.regras, v, aleatorio);
    Avaliador avaliador(c, v, jogo, aleatorio);
    Gerador gerador(v, aleatorio);
    Motor motor(p, gerador, avaliador);
    const ResultadoEvolucao r = motor.evoluir();

    std::vector<std::string> nomesAdv;
    for (const Adversario& a : c.adversarios) nomesAdv.push_back(a.tipo);
    escreverCsv(base + ".csv", r.historico, nomesAdv);

    // nós efetivos do campeão e da população final
    std::vector<std::unique_ptr<Jogador>> advs;
    for (const Adversario& a : c.adversarios) advs.push_back(criarAdversario(a, v, aleatorio));
    const int efetCampeao = medirNosEfetivos(jogo, *r.campeao.arvore, advs, PARTIDAS_NOS_EFETIVOS_CAMPEAO);
    double efetPop = 0;
    for (const Individuo& i : r.populacao)
        efetPop += medirNosEfetivos(jogo, *i.arvore, advs, PARTIDAS_NOS_EFETIVOS_POPULACAO);
    efetPop /= static_cast<double>(r.populacao.size());
    const double segundos = std::chrono::duration<double>(std::chrono::steady_clock::now() - inicio).count();

    const Avaliacao& ca = r.campeao.avaliacao;
    const std::string arvore = paraTexto(*r.campeao.arvore, v);
    std::string winrates;
    for (std::size_t i = 0; i < ca.placares.size(); ++i) {
        std::ostringstream w;
        w << std::fixed << std::setprecision(6) << ca.placares[i].winrate();
        winrates += (i ? ";" : "") + nomesAdv[i] + "=" + w.str();
    }

    {  // a árvore campeã, legível
        std::ofstream f(base + "_campeao.txt");
        f << arvore << "\n\n"
          << "jogo " << cfg.jogo << ", semente " << p.semente << ", rotulo " << rotulo << "\n"
          << "reavaliado com " << p.parada.partidas_confirmacao << " partidas por adversario: pior "
          << ca.fitness.minimo << ", media " << ca.fitness.media << " (" << winrates << ")\n"
          << "na evolucao: pior " << r.campeaoNaEvolucao.minimo << ", media " << r.campeaoNaEvolucao.media << "\n"
          << "nos " << r.campeao.nos << ", efetivos " << efetCampeao << ", profundidade " << r.campeao.profundidade
          << "\n";
        if (!f) throw std::runtime_error("erro ao gravar " + base + "_campeao.txt");
    }

    const EstatisticasGeracao& ini = r.historico.front();
    const EstatisticasGeracao& fim = r.historico.back();
    const std::string resumo = base + ".resumo.csv";
    {
        std::ofstream f(resumo);
        f << "jogo,rotulo,semente,populacao,limite_profundidade,partidas,partidas_confirmacao,geracoes,motivo,"
             "rejeitadas,campeao_minimo,campeao_media,campeao_winrates,campeao_minimo_na_evolucao,campeao_nos,"
             "campeao_nos_efetivos,campeao_profundidade,nos_medios_inicio,nos_medios_fim,nos_efetivos_medios_fim,"
             "distintas_fim,segundos,campeao_arvore\n";
        f << std::fixed << std::setprecision(6) << cfg.jogo << ',' << csvTexto(rotulo) << ',' << p.semente << ','
          << p.populacao << ',' << p.profundidade_limite_filhos << ',' << c.partidas_por_arvore << ','
          << p.parada.partidas_confirmacao << ',' << fim.geracao << ',' << csvTexto(textoDe(r.motivo)) << ','
          << r.rejeitadas << ',' << ca.fitness.minimo << ',' << ca.fitness.media << ',' << csvTexto(winrates) << ','
          << r.campeaoNaEvolucao.minimo << ',' << r.campeao.nos << ',' << efetCampeao << ',' << r.campeao.profundidade
          << ',' << ini.nosMedio << ',' << fim.nosMedio << ',' << efetPop << ',' << fim.distintas << ',' << segundos
          << ',' << csvTexto(arvore) << '\n';
        if (!f) throw std::runtime_error("erro ao gravar " + resumo);
    }

    log << std::fixed << std::setprecision(1) << cfg.jogo << " " << rotulo << " semente " << p.semente << ": "
        << fim.geracao << " geracoes (" << textoDe(r.motivo) << "), campeao pior " << 100 * ca.fitness.minimo << "% (na evolucao "
        << 100 * r.campeaoNaEvolucao.minimo << "%), " << r.campeao.nos << " nos, " << segundos << " s\n";
    return resumo;
}

}  // namespace

std::string usoExperimento() {
    return "uso: experimento --jogo pig|blackjack [opções]\n"
           "  --semente N        semente do gerador (padrão: a do parametros.json)\n"
           "  --limite N         profundidade_limite_filhos (profundidade_max é reduzida se passar dele)\n"
           "  --partidas N       partidas por adversário na evolução (par)\n"
           "  --confirmacao N    partidas por adversário na reavaliação (par; 0 desliga)\n"
           "  --geracoes N       geracoes_max\n"
           "  --sem-melhora N    geracoes_sem_melhora (use um número alto para rodar sempre até geracoes_max)\n"
           "  --alvo X           winrate_alvo (0 a 1)\n"
           "  --populacao N      tamanho da população\n"
           "  --saida PASTA      onde gravar (padrão: resultados)\n"
           "  --rotulo NOME      nome da pasta da configuração (padrão: limite_<L>_partidas_<P>)\n"
           "  --parametros, --avaliador, --spec ARQUIVO   outros json (padrão: os de dataset/)\n";
}

ConfigExperimento lerArgumentos(const std::vector<std::string>& args) {
    ConfigExperimento c;
    for (std::size_t i = 0; i < args.size(); ++i) {
        const std::string& op = args[i];
        if (i + 1 >= args.size()) throw std::runtime_error("faltou o valor de " + op + "\n" + usoExperimento());
        const std::string& val = args[++i];
        if (op == "--jogo") c.jogo = val;
        else if (op == "--semente") c.semente = lerInteiro(op, val);
        else if (op == "--limite") c.limite = lerInteiro(op, val);
        else if (op == "--partidas") c.partidas = lerInteiro(op, val);
        else if (op == "--confirmacao") c.confirmacao = lerInteiro(op, val);
        else if (op == "--geracoes") c.geracoes = lerInteiro(op, val);
        else if (op == "--sem-melhora") c.semMelhora = lerInteiro(op, val);
        else if (op == "--populacao") c.populacao = lerInteiro(op, val);
        else if (op == "--alvo") c.alvo = lerReal(op, val);
        else if (op == "--saida") c.saida = val;
        else if (op == "--rotulo") c.rotulo = val;
        else if (op == "--parametros") c.parametros = val;
        else if (op == "--avaliador") c.avaliador = val;
        else if (op == "--spec") c.spec = val;
        else throw std::runtime_error("opção desconhecida: " + op + "\n" + usoExperimento());
    }
    if (c.jogo != JOGO_PIG && c.jogo != JOGO_BLACKJACK)
        throw std::runtime_error("--jogo deveria ser \"pig\" ou \"blackjack\"\n" + usoExperimento());
    return c;
}

std::string rodarExperimento(const ConfigExperimento& cfg, std::ostream& log) {
    const std::string avaliador = cfg.avaliador.empty() ? "dataset/avaliador_" + cfg.jogo + ".json" : cfg.avaliador;
    const std::string spec = cfg.spec.empty() ? "dataset/specs_regras_" + cfg.jogo + ".json" : cfg.spec;
    const Opcoes o = LeitorJSON::lerOpcoes(cfg.parametros, avaliador, spec);

    // trocas pedidas na linha de comando, conferidas pelas mesmas validações dos json
    Parametros p = o.parametros;
    CriterioAvaliador c = o.avaliador;
    if (cfg.semente) p.semente = *cfg.semente;
    if (cfg.populacao) p.populacao = *cfg.populacao;
    if (cfg.geracoes) p.geracoes_max = *cfg.geracoes;
    if (cfg.semMelhora) p.parada.geracoes_sem_melhora = *cfg.semMelhora;
    if (cfg.alvo) p.parada.winrate_alvo = *cfg.alvo;
    if (cfg.confirmacao) p.parada.partidas_confirmacao = *cfg.confirmacao;
    if (cfg.partidas) c.partidas_por_arvore = *cfg.partidas;
    if (cfg.limite) {
        p.profundidade_limite_filhos = *cfg.limite;
        p.profundidade_max = std::min(p.profundidade_max, *cfg.limite);  // a população inicial cabe no limite
        p.profundidade_min = std::min(p.profundidade_min, p.profundidade_max);
    }
    validarParametros(p);
    validarCriterio(c);

    if (cfg.jogo == JOGO_PIG) return rodar<Pig>(cfg, o, p, c, log);
    return rodar<BlackjackDados>(cfg, o, p, c, log);
}
