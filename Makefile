# Compila e roda os testes de cada peça.
#   make testes       -> compila tudo com avisos e sanitizers e roda todos os testes
#   make experimento  -> compila o executável de experimentos, otimizado (-O2, sem sanitizers)
#   make limpar       -> apaga a pasta build/ e o executável experimento
#
# Flags: -Wall -Wextra (avisos), -fsanitize=address,undefined (acusa acesso fora
# da memória, vazamento e comportamento indefinido em tempo de execução) e
# -D_GLIBCXX_ASSERTIONS (o std::vector confere índices em operator[]).

CXX      ?= g++
CXXFLAGS ?= -std=c++17 -Wall -Wextra -Wpedantic -g -O1 -fno-omit-frame-pointer \
            -fsanitize=address,undefined -D_GLIBCXX_ASSERTIONS
CPPFLAGS += -Iinclude
BUILD    ?= build

# Todos os .cpp de src/ menos a main (que é local e ignorada pelo git).
FONTES  := $(filter-out src/main.cpp,$(wildcard src/*.cpp))
OBJETOS := $(patsubst src/%.cpp,$(BUILD)/src/%.o,$(FONTES))
TESTES  := $(patsubst testes/%.cpp,$(BUILD)/testes/%,$(wildcard testes/teste_*.cpp))

# O experimento é compilado à parte, com -O2 e sem sanitizers: fica ~10x mais rápido.
RELEASE ?= -std=c++17 -O2 -Wall -Wextra -Wpedantic

.PHONY: testes limpar

testes: $(TESTES)
	@for t in $(TESTES); do echo "== $$t"; "$$t" || exit 1; done

$(BUILD)/src/%.o: src/%.cpp
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) $(CXXFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/testes/%: testes/%.cpp $(OBJETOS)
	@mkdir -p $(dir $@)
	$(CXX) $(CPPFLAGS) -Itestes $(CXXFLAGS) -MMD -MP $< $(OBJETOS) -o $@

experimento: apps/experimento.cpp $(FONTES) $(wildcard include/*.h)
	$(CXX) $(CPPFLAGS) $(RELEASE) apps/experimento.cpp $(FONTES) -o $@

limpar:
	rm -rf $(BUILD) experimento

-include $(OBJETOS:.o=.d) $(TESTES:=.d)
