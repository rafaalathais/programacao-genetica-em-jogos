// Testes do módulo de experimento (o que o executável ./experimento usa).
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <sstream>

#include "experimento.h"
#include "teste.h"

namespace fs = std::filesystem;

static std::vector<std::string> linhas(const std::string& caminho) {
    std::ifstream f(caminho);
    std::vector<std::string> v;
    for (std::string l; std::getline(f, l);) v.push_back(l);
    return v;
}
static std::vector<std::string> campos(const std::string& linha) {  // sem tratar aspas: só para as colunas iniciais
    std::vector<std::string> v;
    std::stringstream s(linha);
    for (std::string c; std::getline(s, c, ',');) v.push_back(c);
    return v;
}
static std::string semUltimaColuna(const std::string& l) { return l.substr(0, l.rfind(',')); }

int main() {
    // ---- argumentos ----
    {
        const ConfigExperimento c = lerArgumentos({"--jogo", "pig", "--semente", "7", "--limite", "8", "--partidas", "2000",
                                                   "--confirmacao", "0", "--alvo", "0.52", "--saida", "x", "--rotulo", "r"});
        VERIFICAR(c.jogo == "pig" && *c.semente == 7 && c.limites == std::vector<int>{8} && *c.partidas == 2000);
        VERIFICAR(*c.confirmacao == 0 && *c.alvo == 0.52 && c.saida == "x" && c.rotulo == "r");
        VERIFICAR(!c.geracoes && !c.populacao && c.avaliador.empty());
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "xadrez"}));
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--semente", "3"}));                  // sem --jogo
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--semente"}));      // valor faltando
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--semente", "3x"})); // número inválido
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--sementes", "3"})); // opção desconhecida
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--alvo", "alto"}));
        VERIFICAR(lerArgumentos({"--jogo", "pig"}).limites.empty());  // sem --limite: fica o do json
    }

    // ---- --limite em forma de lista ----
    {
        VERIFICAR(lerArgumentos({"--jogo", "pig", "--limite", "3,5,7,10"}).limites == (std::vector<int>{3, 5, 7, 10}));
        VERIFICAR(lerArgumentos({"--jogo", "pig", "--limite", "10,3"}).limites == (std::vector<int>{10, 3}));  // a ordem é mantida
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--limite", "3,,5"}));   // pedaço vazio
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--limite", "3,5,"}));   // vírgula sobrando
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--limite", "3,x"}));    // não é número
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--limite", "5,3,5"}));  // repetido
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--limite", "3 5"}));    // espaço não separa
        VERIFICAR_LANCA(std::runtime_error, lerArgumentos({"--jogo", "pig", "--limite", "3,5", "--rotulo", "r"}));
        VERIFICAR(lerArgumentos({"--jogo", "pig", "--limite", "3", "--rotulo", "r"}).rotulo == "r");  // um limite: pode
    }

    const fs::path pasta = fs::temp_directory_path() / "pg_teste_experimento";
    fs::remove_all(pasta);
    auto config = [&](const std::string& jogo, int semente) {
        ConfigExperimento c = lerArgumentos({"--jogo", jogo, "--semente", std::to_string(semente), "--populacao", "12",
                                             "--geracoes", "2", "--partidas", "20", "--confirmacao", "200", "--limite",
                                             "4", "--saida", pasta.string()});
        return c;
    };

    // ---- uma rodada de cada jogo: arquivos e conteúdo ----
    for (const std::string jogo : {"pig", "blackjack"}) {
        std::ostringstream log;
        const std::vector<std::string> resumos = rodarExperimento(config(jogo, 3), log);
        VERIFICAR(resumos.size() == 1);
        const std::string resumo = resumos.at(0);
        const fs::path dir = pasta / jogo / "limite_4_partidas_20";
        VERIFICAR(resumo == (dir / "semente_3.resumo.csv").string());
        VERIFICAR(fs::exists(dir / "semente_3.csv") && fs::exists(dir / "semente_3_campeao.txt"));
        VERIFICAR(log.str().find(jogo + " limite_4_partidas_20 semente 3:") == 0);

        const auto r = linhas(resumo);
        VERIFICAR(r.size() == 2);
        const auto cab = campos(r[0]), val = campos(r[1]);
        VERIFICAR(cab.size() == 23 && cab[0] == "jogo" && cab.back() == "campeao_arvore");
        VERIFICAR(val[0] == jogo && val[2] == "3" && val[3] == "12" && val[4] == "4" && val[5] == "20" && val[6] == "200");
        const double minimo = std::stod(val[10]), nos = std::stod(val[14]), efet = std::stod(val[15]);
        VERIFICAR(minimo >= 0 && minimo <= 1);
        VERIFICAR(efet >= 1 && efet <= nos);                // nós efetivos nunca passam do tamanho
        VERIFICAR(std::stoi(val[16]) <= 4);                 // o campeão respeita o limite de profundidade
        VERIFICAR(r[1].find("SE(") != std::string::npos || r[1].find("PARAR") != std::string::npos);

        const auto h = linhas((dir / "semente_3.csv").string());
        VERIFICAR(h.size() == 4);  // cabeçalho + gerações 0, 1, 2
        const auto campeao = linhas((dir / "semente_3_campeao.txt").string());
        VERIFICAR(campeao.size() >= 5 && campeao[3].find("reavaliado com 200 partidas") == 0);
    }

    // ---- mesma semente, mesmo resultado (tirando o tempo); outra semente, outro ----
    {
        std::ostringstream log;
        const std::string a = linhas(rodarExperimento(config("blackjack", 3), log).at(0))[1];
        ConfigExperimento c = config("blackjack", 3);
        c.saida = (pasta / "repetida").string();
        const std::string b = linhas(rodarExperimento(c, log).at(0))[1];
        // a coluna "segundos" é a penúltima (antes da árvore); compara todo o resto
        auto semTempo = [](const std::string& l) {
            const std::size_t fimArvore = l.rfind(",\"");
            const std::size_t inicioTempo = l.rfind(',', fimArvore - 1);
            return l.substr(0, inicioTempo) + l.substr(fimArvore);
        };
        VERIFICAR(semTempo(a) == semTempo(b));
        const auto ha = linhas((pasta / "blackjack/limite_4_partidas_20/semente_3.csv").string());
        const auto hb = linhas((pasta / "repetida/blackjack/limite_4_partidas_20/semente_3.csv").string());
        bool historicoIgual = ha.size() == hb.size();
        for (std::size_t i = 0; historicoIgual && i < ha.size(); ++i) historicoIgual = semUltimaColuna(ha[i]) == semUltimaColuna(hb[i]);
        VERIFICAR(historicoIgual);
        const std::string z = linhas(rodarExperimento(config("blackjack", 4), log).at(0))[1];
        VERIFICAR(semTempo(z) != semTempo(a));
    }

    // ---- trocas inválidas passam pelas mesmas validações dos json ----
    {
        std::ostringstream log;
        ConfigExperimento c = config("pig", 1);
        c.partidas = 21;  // ímpar
        VERIFICAR_LANCA(std::runtime_error, rodarExperimento(c, log));
        c = config("pig", 1);
        c.limites = {1};  // menor que a profundidade mínima de uma árvore
        VERIFICAR_LANCA(std::runtime_error, rodarExperimento(c, log));
        c = config("pig", 1);
        c.confirmacao = 3;
        VERIFICAR_LANCA(std::runtime_error, rodarExperimento(c, log));
        c = config("pig", 1);
        c.spec = "dataset/nao_existe.json";
        VERIFICAR_LANCA(std::runtime_error, rodarExperimento(c, log));
    }

    // ---- lista de limites: uma rodada por limite, cada uma igual à rodada avulsa ----
    {
        std::ostringstream log;
        ConfigExperimento c = config("blackjack", 5);
        c.saida = (pasta / "lista").string();
        c.limites = {3, 5};
        const std::vector<std::string> resumos = rodarExperimento(c, log);
        VERIFICAR(resumos.size() == 2);
        VERIFICAR(resumos.at(0) == (pasta / "lista/blackjack/limite_3_partidas_20/semente_5.resumo.csv").string());
        VERIFICAR(resumos.at(1) == (pasta / "lista/blackjack/limite_5_partidas_20/semente_5.resumo.csv").string());
        VERIFICAR(campos(linhas(resumos.at(0))[1])[4] == "3" && campos(linhas(resumos.at(1))[1])[4] == "5");
        VERIFICAR(linhas((pasta / "lista/blackjack/limite_3_partidas_20/semente_5.csv").string()).size() == 4);
        const std::string l = log.str();
        VERIFICAR(l.find("limite_3_partidas_20") < l.find("limite_5_partidas_20"));  // na ordem pedida
        // o limite 5 dentro da lista dá o mesmo histórico que rodado sozinho
        ConfigExperimento s = config("blackjack", 5);
        s.saida = (pasta / "sozinho").string();
        s.limites = {5};
        rodarExperimento(s, log);
        const auto ha = linhas((pasta / "lista/blackjack/limite_5_partidas_20/semente_5.csv").string());
        const auto hb = linhas((pasta / "sozinho/blackjack/limite_5_partidas_20/semente_5.csv").string());
        bool igual = ha.size() == hb.size();
        for (std::size_t i = 0; igual && i < ha.size(); ++i) igual = semUltimaColuna(ha[i]) == semUltimaColuna(hb[i]);
        VERIFICAR(igual);
        // um limite inválido no fim da lista é recusado antes de rodar qualquer um
        ConfigExperimento ruim = config("blackjack", 5);
        ruim.saida = (pasta / "ruim").string();
        ruim.limites = {3, 1};
        VERIFICAR_LANCA(std::runtime_error, rodarExperimento(ruim, log));
        VERIFICAR(!fs::exists(pasta / "ruim"));
        // --rotulo com vários limites também é recusado se a struct for montada à mão
        ConfigExperimento rot = config("blackjack", 5);
        rot.limites = {3, 5};
        rot.rotulo = "r";
        VERIFICAR_LANCA(std::runtime_error, rodarExperimento(rot, log));
    }

    fs::remove_all(pasta);
    return teste::fim("experimento");
}
