.PHONY: all 

T_CC_FLAGS       ?= ${T_CC_FLAGS_OPT} -Wall -Isrc -I../see/src
T_CC_OPT_FLAGS   ?= -O0
T_CC_DEBUG_FLAGS ?= -g

SRCFILES:= $(shell find src '(' '!' -regex '.*/_.*' ')' -and '(' -iname "*.cpp" ')' | sed -e 's!^\./!!g') ../see/src/cpp_binding/script.cpp \

include ${T_BASE}/utl/template.mk

all: ${T_OBJ}/waff

-include ${DEPFILES}

${T_OBJ}/waff: ${OBJFILES} ${T_OBJ}/see.a
	@echo LD $@
	${V}${CXX} $^ -o $@ ${T_CC_ALL_FLAGS}
