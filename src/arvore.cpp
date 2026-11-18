#include "arvore.h"

#include <algorithm>
#include <stdexcept>

// ---------------------------------------------------------------- construção

namespace {

Arvore criarNo(TipoNo tipo, int valor) {
    Arvore no = std::make_unique<No>();
    no->tipo = tipo;
    no->valor = valor;
    return no;
}

}  // namespace

Arvore criarSe(Arvore condicao, Arvore entao, Arvore senao) {
    Arvore no = criarNo(TipoNo::SE, 0);
    no->filhos.push_back(std::move(condicao));
    no->filhos.push_back(std::move(entao));
    no->filhos.push_back(std::move(senao));
    return no;
}

Arvore criarComparacao(Comparacao op, Arvore esquerda, Arvore direita) {
    Arvore no = criarNo(TipoNo::COMPARACAO, static_cast<int>(op));
    no->filhos.push_back(std::move(esquerda));
    no->filhos.push_back(std::move(direita));
    return no;
}

Arvore criarVariavel(int indice) { return criarNo(TipoNo::VARIAVEL, indice); }
Arvore criarConstante(int valor) { return criarNo(TipoNo::CONSTANTE, valor); }
Arvore criarAcao(Acao acao) { return criarNo(TipoNo::ACAO, static_cast<int>(acao)); }

// ---------------------------------------------------------------- avaliação

namespace {

int valorOperando(const No& no, const Estado& estado) {
    if (no.tipo == TipoNo::VARIAVEL) return estado.at(static_cast<std::size_t>(no.valor));
    return no.valor;  // CONSTANTE
}

bool avaliarCondicao(const No& comparacao, const Estado& estado) {
    return comparar(static_cast<Comparacao>(comparacao.valor), valorOperando(*comparacao.filhos[0], estado),
                    valorOperando(*comparacao.filhos[1], estado));
}

}  // namespace

Acao avaliar(const No& arvore, const Estado& estado) {
    // Desce pela árvore sem recursão: em cada SE, escolhe o ramo "então" ou "senão"
    // conforme a condição, até chegar numa folha de ação.
    const No* atual = &arvore;
    while (atual->tipo == TipoNo::SE)
        atual = avaliarCondicao(*atual->filhos[0], estado) ? atual->filhos[1].get() : atual->filhos[2].get();
    if (atual->tipo != TipoNo::ACAO) throw std::logic_error("avaliar: árvore inválida (esperava uma ação)");
    return static_cast<Acao>(atual->valor);
}

Acao avaliarMarcando(const No& arvore, const Estado& estado, std::unordered_set<const No*>& visitados) {
    const No* atual = &arvore;
    while (atual->tipo == TipoNo::SE) {
        visitados.insert(atual);
        atual = avaliarCondicao(*atual->filhos[0], estado) ? atual->filhos[1].get() : atual->filhos[2].get();
    }
    if (atual->tipo != TipoNo::ACAO) throw std::logic_error("avaliarMarcando: árvore inválida (esperava uma ação)");
    visitados.insert(atual);
    return static_cast<Acao>(atual->valor);
}

int nosEfetivos(const No& arvore, const std::unordered_set<const No*>& visitados) {
    if (visitados.count(&arvore) == 0) return 0;  // nenhuma decisão chegou aqui
    if (arvore.tipo != TipoNo::SE) return 1;      // folha de ação alcançada
    // o SE e a sua comparação inteira contam; os ramos, só se foram visitados
    return 1 + tamanho(*arvore.filhos[0]) + nosEfetivos(*arvore.filhos[1], visitados) +
           nosEfetivos(*arvore.filhos[2], visitados);
}

// ---------------------------------------------------------------- medidas e cópia

Arvore copiar(const No& arvore) {
    Arvore copia = criarNo(arvore.tipo, arvore.valor);
    for (const Arvore& filho : arvore.filhos) copia->filhos.push_back(copiar(*filho));
    return copia;
}

int profundidade(const No& arvore) {
    int maior = -1;  // folha: 1 + (-1) = 0
    for (const Arvore& filho : arvore.filhos) maior = std::max(maior, profundidade(*filho));
    return 1 + maior;
}

int tamanho(const No& arvore) {
    int total = 1;
    for (const Arvore& filho : arvore.filhos) total += tamanho(*filho);
    return total;
}

std::string paraTexto(const No& arvore, const Vocabulario& vocab) {
    switch (arvore.tipo) {
        case TipoNo::SE:
            return "SE(" + paraTexto(*arvore.filhos[0], vocab) + ", " + paraTexto(*arvore.filhos[1], vocab) + ", " +
                   paraTexto(*arvore.filhos[2], vocab) + ")";
        case TipoNo::COMPARACAO:
            return paraTexto(*arvore.filhos[0], vocab) + " " + textoDe(static_cast<Comparacao>(arvore.valor)) + " " +
                   paraTexto(*arvore.filhos[1], vocab);
        case TipoNo::VARIAVEL:
            return vocab.variaveis.at(static_cast<std::size_t>(arvore.valor)).nome;
        case TipoNo::CONSTANTE:
            return std::to_string(arvore.valor);
        case TipoNo::ACAO:
            return textoDe(static_cast<Acao>(arvore.valor));
    }
    return "?";
}

// ---------------------------------------------------------------- validação

namespace {

const char* nomeTipo(TipoNo t) {
    switch (t) {
        case TipoNo::SE: return "SE";
        case TipoNo::COMPARACAO: return "COMPARACAO";
        case TipoNo::VARIAVEL: return "VARIAVEL";
        case TipoNo::CONSTANTE: return "CONSTANTE";
        case TipoNo::ACAO: return "ACAO";
    }
    return "?";
}

// Confere quantidade de filhos e que nenhum é nulo.
std::string conferirFilhos(const No& no, std::size_t esperado) {
    if (no.filhos.size() != esperado)
        return std::string(nomeTipo(no.tipo)) + " com " + std::to_string(no.filhos.size()) + " filhos (esperado " +
               std::to_string(esperado) + ")";
    for (const Arvore& filho : no.filhos)
        if (!filho) return std::string(nomeTipo(no.tipo)) + " com filho nulo";
    return "";
}

bool indiceValido(int i, const Vocabulario& vocab) {
    return i >= 0 && static_cast<std::size_t>(i) < vocab.variaveis.size();
}

std::string validarCondicao(const No& no, const Vocabulario& vocab) {
    if (no.tipo != TipoNo::COMPARACAO)
        return std::string("lugar de condição com ") + nomeTipo(no.tipo) + " (esperado COMPARACAO)";
    if (std::string e = conferirFilhos(no, 2); !e.empty()) return e;
    if (no.valor != static_cast<int>(Comparacao::MAIOR_IGUAL) && no.valor != static_cast<int>(Comparacao::MAIOR))
        return "comparação com operador inválido " + std::to_string(no.valor);
    const Comparacao op = static_cast<Comparacao>(no.valor);
    if (std::find(vocab.comparacoes.begin(), vocab.comparacoes.end(), op) == vocab.comparacoes.end())
        return "comparação \"" + textoDe(op) + "\" fora do vocabulário";

    const No& esq = *no.filhos[0];
    const No& dir = *no.filhos[1];
    if (esq.tipo != TipoNo::VARIAVEL) return std::string("lado esquerdo da comparação é ") + nomeTipo(esq.tipo);
    if (std::string e = conferirFilhos(esq, 0); !e.empty()) return e;
    if (!indiceValido(esq.valor, vocab)) return "índice de variável inválido " + std::to_string(esq.valor);
    if (std::string e = conferirFilhos(dir, 0); !e.empty()) return e;

    if (dir.tipo == TipoNo::VARIAVEL) {
        if (!indiceValido(dir.valor, vocab)) return "índice de variável inválido " + std::to_string(dir.valor);
        if (!vocab.compativeis(esq.valor, dir.valor))
            return "variáveis incompatíveis: " + vocab.variaveis[static_cast<std::size_t>(esq.valor)].nome + " e " +
                   vocab.variaveis[static_cast<std::size_t>(dir.valor)].nome;
        return "";
    }
    if (dir.tipo == TipoNo::CONSTANTE) {
        const Variavel& v = vocab.variaveis[static_cast<std::size_t>(esq.valor)];
        const Faixa f = faixaConstante(v, op);
        if (dir.valor < f.min || dir.valor > f.max)
            return "constante " + std::to_string(dir.valor) + " fora da faixa [" + std::to_string(f.min) + ", " +
                   std::to_string(f.max) + "] de " + v.nome + " " + textoDe(op);
        return "";
    }
    return std::string("lado direito da comparação é ") + nomeTipo(dir.tipo);
}

std::string validarAcao(const No& no, const Vocabulario& vocab) {
    if (no.tipo == TipoNo::ACAO) {
        if (std::string e = conferirFilhos(no, 0); !e.empty()) return e;
        if (no.valor != static_cast<int>(Acao::CONTINUAR) && no.valor != static_cast<int>(Acao::PARAR))
            return "ação inválida " + std::to_string(no.valor);
        const Acao a = static_cast<Acao>(no.valor);
        if (std::find(vocab.acoes.begin(), vocab.acoes.end(), a) == vocab.acoes.end())
            return "ação \"" + textoDe(a) + "\" fora do vocabulário";
        return "";
    }
    if (no.tipo == TipoNo::SE) {
        if (std::string e = conferirFilhos(no, 3); !e.empty()) return e;
        if (std::string e = validarCondicao(*no.filhos[0], vocab); !e.empty()) return e;
        if (std::string e = validarAcao(*no.filhos[1], vocab); !e.empty()) return e;
        return validarAcao(*no.filhos[2], vocab);
    }
    return std::string("lugar de ação com ") + nomeTipo(no.tipo) + " (esperado SE ou ACAO)";
}

}  // namespace

std::string validar(const No& arvore, const Vocabulario& vocab) { return validarAcao(arvore, vocab); }
