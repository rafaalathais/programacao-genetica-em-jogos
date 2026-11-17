#include "leitorObjeto.h"

#include <climits>
#include <stdexcept>

LeitorObjeto::LeitorObjeto(const nlohmann::json& objeto, std::string onde) : objeto_(objeto), onde_(std::move(onde)) {
    if (!objeto_.is_object()) erro("deveria ser um objeto json, mas é " + objeto_.dump());
}

void LeitorObjeto::erro(const std::string& mensagem) const { throw std::runtime_error(onde_ + ": " + mensagem); }

const nlohmann::json& LeitorObjeto::campo(const char* chave) {
    auto it = objeto_.find(chave);
    if (it == objeto_.end()) erro(std::string("falta a chave \"") + chave + "\"");
    lidas_.insert(chave);
    return *it;
}

int LeitorObjeto::inteiro(const char* chave) {
    const nlohmann::json& v = campo(chave);
    // is_number_integer é falso para 1.5 e para true (no nlohmann, bool não é número).
    if (!v.is_number_integer()) erro(std::string(chave) + " deveria ser um número inteiro, mas é " + v.dump());
    if (v.is_number_unsigned() ? v.get<unsigned long long>() > static_cast<unsigned long long>(INT_MAX)
                               : (v.get<long long>() < INT_MIN || v.get<long long>() > INT_MAX))
        erro(std::string(chave) + " está fora da faixa de int: " + v.dump());
    return v.get<int>();
}

bool LeitorObjeto::logico(const char* chave) {
    const nlohmann::json& v = campo(chave);
    if (!v.is_boolean()) erro(std::string(chave) + " deveria ser true ou false, mas é " + v.dump());
    return v.get<bool>();
}

std::string LeitorObjeto::texto(const char* chave) {
    const nlohmann::json& v = campo(chave);
    if (!v.is_string()) erro(std::string(chave) + " deveria ser um texto, mas é " + v.dump());
    return v.get<std::string>();
}

LeitorObjeto LeitorObjeto::objeto(const char* chave) { return LeitorObjeto(campo(chave), onde_ + "." + chave); }

void LeitorObjeto::semChavesDesconhecidas() const {
    std::string sobrando;
    for (auto it = objeto_.begin(); it != objeto_.end(); ++it)
        if (lidas_.count(it.key()) == 0) sobrando += (sobrando.empty() ? "\"" : ", \"") + it.key() + "\"";
    if (!sobrando.empty()) erro("chave desconhecida: " + sobrando);
}
