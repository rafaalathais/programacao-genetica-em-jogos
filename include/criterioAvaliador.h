//
// Created by leon on 16/11/2026.
//

#ifndef PROGRAMACAO_GENETICA_EM_JOGOS_CRITERIOAVALIACAO_H
#define PROGRAMACAO_GENETICA_EM_JOGOS_CRITERIOAVALIACAO_H
#include <optional>
#include "nlohmann/json.hpp"

namespace nlohmann {
    template <typename T>
    struct adl_serializer<std::optional<T>> {
        static void to_json(json& j, const std::optional<T>& opt) {
            if (opt.has_value()) j = *opt;
            else j = nullptr;
        }
        static void from_json(const json& j, std::optional<T>& opt) {
            if (j.is_null()) opt = std::nullopt;
            else opt = j.get<T>();
        }
    };
}

struct Adversario {
    std::string tipo;
    std::optional<std::string> variavel;
    std::optional<int> valor;
    std::optional<int> limiar_se_primeiro;
};

NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE_WITH_DEFAULT(Adversario, tipo, variavel, valor, limiar_se_primeiro)

struct CriterioAvaliador {
    int partidas_por_arvore;
    bool alternar_quem_comeca;
    std::vector<Adversario> adversarios;
    std::string fitness;
};
NLOHMANN_DEFINE_TYPE_NON_INTRUSIVE(CriterioAvaliador, partidas_por_arvore, alternar_quem_comeca, adversarios, fitness)
#endif //PROGRAMACAO_GENETICA_EM_JOGOS_CRITERIOAVALIACAO_H
