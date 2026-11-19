#include "estatisticas.h"

#include <algorithm>
#include <fstream>
#include <iomanip>
#include <set>
#include <stdexcept>

#include "selecao.h"

EstatisticasGeracao calcularEstatisticas(int geracao, const std::vector<Individuo>& populacao,
                                         const Vocabulario& vocab) {
    EstatisticasGeracao e;
    e.geracao = geracao;
    e.individuos = static_cast<int>(populacao.size());
    if (populacao.empty()) return e;

    std::vector<Fitness> fit;
    std::set<std::string> textos;
    for (const Individuo& i : populacao) {
        fit.push_back(i.avaliacao.fitness);
        e.mediaMinimo += i.avaliacao.fitness.minimo;
        e.mediaMedia += i.avaliacao.fitness.media;
        e.nosMedio += i.nos;
        e.profundidadeMedia += i.profundidade;
        e.nosMax = std::max(e.nosMax, i.nos);
        e.profundidadeMax = std::max(e.profundidadeMax, i.profundidade);
        textos.insert(paraTexto(*i.arvore, vocab));
    }
    const double n = static_cast<double>(populacao.size());
    e.mediaMinimo /= n;
    e.mediaMedia /= n;
    e.nosMedio /= n;
    e.profundidadeMedia /= n;
    e.distintas = static_cast<int>(textos.size());

    const Individuo& m = populacao[melhores(fit, 1)[0]];
    e.melhor = m.avaliacao.fitness;
    for (const Placar& p : m.avaliacao.placares) e.winratesMelhor.push_back(p.winrate());
    e.nosMelhor = m.nos;
    e.profundidadeMelhor = m.profundidade;
    return e;
}

void escreverCsv(const std::string& caminho, const std::vector<EstatisticasGeracao>& historico,
                 const std::vector<std::string>& adversarios) {
    std::ofstream f(caminho);
    if (!f) throw std::runtime_error("não foi possível criar " + caminho);
    f << "geracao,individuos,melhor_minimo,melhor_media";
    for (const std::string& a : adversarios) f << ",melhor_w_" << a;
    f << ",melhor_nos,melhor_profundidade,media_minimo,media_media,nos_medio,nos_max,"
         "profundidade_media,profundidade_max,distintas,cruzamentos,mutacoes,rejeitadas,segundos\n";
    f << std::fixed << std::setprecision(6);
    for (const EstatisticasGeracao& e : historico) {
        f << e.geracao << ',' << e.individuos << ',' << e.melhor.minimo << ',' << e.melhor.media;
        for (std::size_t i = 0; i < adversarios.size(); ++i)
            f << ',' << (i < e.winratesMelhor.size() ? e.winratesMelhor[i] : 0.0);
        f << ',' << e.nosMelhor << ',' << e.profundidadeMelhor << ',' << e.mediaMinimo << ',' << e.mediaMedia << ','
          << e.nosMedio << ',' << e.nosMax << ',' << e.profundidadeMedia << ',' << e.profundidadeMax << ','
          << e.distintas << ',' << e.cruzamentos << ',' << e.mutacoes << ',' << e.rejeitadas << ','
          << e.segundos << '\n';
    }
    if (!f) throw std::runtime_error("erro ao gravar " + caminho);
}
