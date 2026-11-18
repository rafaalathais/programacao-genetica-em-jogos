#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_MOTOR_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_MOTOR_H

#include <functional>
#include <map>
#include <string>
#include <vector>

#include "avaliador.h"
#include "estatisticas.h"
#include "gerador.h"
#include "individuo.h"
#include "parametros.h"

enum class MotivoParada {
    ALVO_ATINGIDO,  // o melhor da geração chegou a winrate_alvo no pior winrate (e confirmou, se houver confirmação)
    SEM_MELHORA,    // geracoes_sem_melhora gerações seguidas sem superar o melhor de todos
    GERACOES_MAX    // chegou à geração geracoes_max
};

const char* textoDe(MotivoParada motivo);

struct ResultadoEvolucao {
    std::vector<Individuo> populacao;            // a última geração, já avaliada
    Individuo melhor;                            // o melhor MEDIDO em qualquer geração (cópia; número otimista)
    int geracaoDoMelhor = 0;
    std::vector<EstatisticasGeracao> historico;  // uma entrada por geração, a partir da 0
    MotivoParada motivo = MotivoParada::GERACOES_MAX;

    // Campeão: escolhido no fim entre os candidatos (o `melhor` e os melhores da última
    // geração), pela reavaliação com partidas_confirmacao partidas novas. A `avaliacao`
    // dele é a da reavaliação, sem a sorte da seleção. Sem confirmação (0), é o `melhor`.
    Individuo campeao;
    Fitness campeaoNaEvolucao;   // o que o campeão tinha medido durante a evolução
    int confirmacoes = 0;        // quantas árvores foram reavaliadas (alvo + escolha do campeão)
    int rejeitadas = 0;          // candidatas ao alvo que NÃO confirmaram (eram sorte)
};

// Motor evolutivo: o laço de gerações.
//
//   geração 0: população inicial do Gerador (ramped half-and-half), avaliada.
//   a cada geração nova:
//     1. elite: as `elitismo` melhores árvores passam SEM alteração (cópia exata);
//     2. o resto, um filho por vez, até a população ter exatamente `populacao` indivíduos:
//          pai1 = torneio
//          com prob. cruzamento.probabilidade: pai2 = torneio, filho = cruzar(pai1, pai2)
//          senão:                               filho = cópia do pai1
//          com prob. mutacao.probabilidade:     filho = mutar(filho)
//        (as probabilidades são sorteadas para cada filho; não são cotas exatas);
//     3. a geração nova inteira é avaliada — inclusive a elite (ver abaixo).
//   para quando (testado depois de avaliar cada geração, nesta ordem):
//     - o melhor da geração tem pior winrate >= winrate_alvo;
//     - geracoes_sem_melhora gerações seguidas não superaram o melhor de todos
//       (comparação do Fitness: subir só a média, com o mínimo igual, conta como melhora);
//     - a geração atual é geracoes_max (ou seja, no máximo geracoes_max gerações depois da 0).
//
// Separar as árvores boas das sortudas (partidas_confirmacao > 0):
//   O número medido com partidas_por_arvore tem sorte, e o MAIOR deles numa população é
//   quase sempre inflado (o primeiro colocado é também o mais sortudo). Por isso:
//   - alvo: quando o melhor da geração mede >= winrate_alvo, ele é reavaliado com
//     partidas_confirmacao partidas NOVAS. Só para se o pior winrate reavaliado também
//     for >= winrate_alvo; senão a árvore é anotada como rejeitada e a evolução segue.
//     Uma árvore já rejeitada não é reavaliada de novo (cache pelo texto da árvore).
//   - campeão: no fim, o `melhor` e as CANDIDATOS_FINAIS melhores da última geração
//     são reavaliados, e o campeão é o melhor pela reavaliação.
//   Uma medida nova de UMA árvore não tem o viés de escolher o maior entre muitas, e
//   com 20000 partidas a sorte é de só ±0,35 ponto.
//
// A elite é REAVALIADA a cada geração (escolha nossa, o grupo pode mudar com
// REAVALIAR_ELITE). Com 500 partidas o winrate varia uns ±4,5 pontos só por sorte;
// guardar o fitness antigo deixaria uma árvore "sortuda" presa na elite para sempre.
// A árvore da elite não muda; só o número medido.
//
// Gerador, Avaliador (com o Jogo e os adversários) e o Motor devem usar o MESMO
// Aleatorio: o motor sorteia com gerador.aleatorio().
class Motor {
public:
    static constexpr bool REAVALIAR_ELITE = true;
    // Quantas das melhores da última geração disputam o título de campeão (além do `melhor`).
    static constexpr int CANDIDATOS_FINAIS = 5;

    // Chamado ao fim de cada geração (inclusive a 0), por exemplo para imprimir o progresso.
    using AoFimDaGeracao = std::function<void(const EstatisticasGeracao&, const std::vector<Individuo>&)>;

    Motor(const Parametros& parametros, Gerador& gerador, Avaliador& avaliador);

    ResultadoEvolucao evoluir(const AoFimDaGeracao& aoFimDaGeracao = nullptr);

private:
    Individuo novoIndividuo(Arvore arvore);
    std::vector<Individuo> proximaGeracao(const std::vector<Individuo>& atual, ContadoresOperadores& contadores);
    void avaliarTodos(std::vector<Individuo>& populacao, std::size_t aPartirDe);
    // Reavalia com partidas_confirmacao, guardando o resultado (cada árvore uma vez só).
    const Avaliacao& confirmar(const No& arvore, ResultadoEvolucao& r);
    void escolherCampeao(ResultadoEvolucao& r);

    std::map<std::string, Avaliacao> confirmadas_;  // texto da árvore -> reavaliação

    const Parametros& p_;
    Gerador& gerador_;
    Avaliador& avaliador_;
};

#endif  // PROGRAMACAO_GENETICA_EM_JOGOS_MOTOR_H
