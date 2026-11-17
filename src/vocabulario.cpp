#include "vocabulario.h"

#include <algorithm>
#include <climits>
#include <stdexcept>

#include "nomes.h"

// Funções auxiliares visíveis só neste arquivo (namespace anônimo).
namespace {

[[noreturn]] void erro(const std::string& mensagem) {
    throw std::runtime_error("spec: " + mensagem);
}

// Pega objeto[chave], com erro claro se não for objeto ou se a chave faltar.
const nlohmann::json& campo(const nlohmann::json& objeto, const char* chave, const std::string& onde) {
    if (!objeto.is_object()) erro(onde + " deveria ser um objeto json, mas é " + objeto.dump());
    auto it = objeto.find(chave);
    if (it == objeto.end()) erro(onde + ": falta a chave \"" + chave + "\"");
    return *it;
}

// Lê um inteiro sem conversão silenciosa: 1.5, "1" e true são recusados.
int lerInteiro(const nlohmann::json& valor, const std::string& onde) {
    if (!valor.is_number_integer()) erro(onde + " deveria ser um número inteiro, mas é " + valor.dump());
    if (valor.is_number_unsigned()) {
        if (valor.get<unsigned long long>() > static_cast<unsigned long long>(INT_MAX))
            erro(onde + " é grande demais: " + valor.dump());
    } else {
        const long long v = valor.get<long long>();
        if (v < INT_MIN || v > INT_MAX) erro(onde + " está fora da faixa de int: " + valor.dump());
    }
    return valor.get<int>();
}

std::string lerTexto(const nlohmann::json& valor, const std::string& onde) {
    if (!valor.is_string()) erro(onde + " deveria ser um texto, mas é " + valor.dump());
    return valor.get<std::string>();
}

const nlohmann::json& lerLista(const nlohmann::json& spec, const char* chave) {
    // `onde` é uma variável, não o literal "spec" direto: o g++ 13+ acusa
    // (falsamente) "referência pendurada" quando uma função que devolve
    // referência recebe um std::string temporário.
    const std::string onde = "spec";
    const nlohmann::json& lista = campo(spec, chave, onde);
    if (!lista.is_array() || lista.empty())
        erro(std::string("\"") + chave + "\" deveria ser uma lista não vazia, mas é " + lista.dump());
    return lista;
}

std::string posicao(const char* lista, std::size_t i) {
    return std::string(lista) + "[" + std::to_string(i) + "]";
}

template <typename T>
bool contem(const std::vector<T>& v, const T& x) {
    return std::find(v.begin(), v.end(), x) != v.end();
}

}  // namespace

int Vocabulario::indiceDe(const std::string& nome) const {
    for (std::size_t i = 0; i < variaveis.size(); ++i)
        if (variaveis[i].nome == nome) return static_cast<int>(i);
    throw std::runtime_error("vocabulário: variável desconhecida \"" + nome + "\"");
}

bool Vocabulario::compativeis(int i, int j) const {
    const auto& a = variaveis.at(static_cast<std::size_t>(i));
    const auto& b = variaveis.at(static_cast<std::size_t>(j));
    return i != j && a.min == b.min && a.max == b.max;
}

Faixa faixaConstante(const Variavel& v, Comparacao op) {
    if (op == Comparacao::MAIOR_IGUAL) return {v.min + 1, v.max};
    return {v.min, v.max - 1};
}

Vocabulario lerVocabulario(const nlohmann::json& spec) {
    if (!spec.is_object()) erro("o arquivo deveria conter um objeto json");
    Vocabulario vocab;

    // variaveis: [{"nome": ..., "min": ..., "max": ...}, ...]
    const nlohmann::json& variaveis = lerLista(spec, nomes::VARIAVEIS);
    for (std::size_t i = 0; i < variaveis.size(); ++i) {
        const std::string onde = posicao(nomes::VARIAVEIS, i);
        const nlohmann::json& item = variaveis[i];
        Variavel v;
        v.nome = lerTexto(campo(item, nomes::NOME, onde), onde + "." + nomes::NOME);
        v.min = lerInteiro(campo(item, nomes::MIN, onde), onde + "." + nomes::MIN);
        v.max = lerInteiro(campo(item, nomes::MAX, onde), onde + "." + nomes::MAX);
        if (v.nome.empty()) erro(onde + ": nome vazio");
        // min == max seria uma "variável" que nunca muda: inútil e quebra faixaConstante.
        if (v.min >= v.max) erro(onde + " (" + v.nome + "): min deveria ser menor que max");
        for (const Variavel& outra : vocab.variaveis)
            if (outra.nome == v.nome) erro("variável repetida: \"" + v.nome + "\"");
        vocab.variaveis.push_back(v);
    }

    // acoes: ["CONTINUAR", "PARAR"]
    const nlohmann::json& acoes = lerLista(spec, nomes::ACOES);
    for (std::size_t i = 0; i < acoes.size(); ++i) {
        const std::string onde = posicao(nomes::ACOES, i);
        const std::string texto = lerTexto(acoes[i], onde);
        const std::optional<Acao> acao = acaoDeTexto(texto);
        if (!acao) erro(onde + ": ação desconhecida \"" + texto + "\" (esperado \"" + nomes::CONTINUAR +
                        "\" ou \"" + nomes::PARAR + "\")");
        if (contem(vocab.acoes, *acao)) erro("ação repetida: \"" + texto + "\"");
        vocab.acoes.push_back(*acao);
    }
    if (!contem(vocab.acoes, Acao::CONTINUAR) || !contem(vocab.acoes, Acao::PARAR))
        erro(std::string("\"") + nomes::ACOES + "\" deveria ter " + nomes::CONTINUAR + " e " + nomes::PARAR);

    // comparacoes: [">=", ">"]
    const nlohmann::json& comparacoes = lerLista(spec, nomes::COMPARACOES);
    for (std::size_t i = 0; i < comparacoes.size(); ++i) {
        const std::string onde = posicao(nomes::COMPARACOES, i);
        const std::string texto = lerTexto(comparacoes[i], onde);
        const std::optional<Comparacao> op = comparacaoDeTexto(texto);
        if (!op) erro(onde + ": comparação desconhecida \"" + texto + "\" (esperado \"" + nomes::MAIOR_IGUAL +
                      "\" ou \"" + nomes::MAIOR + "\")");
        if (contem(vocab.comparacoes, *op)) erro("comparação repetida: \"" + texto + "\"");
        vocab.comparacoes.push_back(*op);
    }

    return vocab;
}
