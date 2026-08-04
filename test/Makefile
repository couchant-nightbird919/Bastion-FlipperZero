# Compiles the real grading engine for the host and checks every score, band
# boundary and string. The grade is the whole product, so it is verified on
# every push rather than eyeballed on a screenshot. See host_grade_test.c.
#
#   make -C test

CC     ?= cc
CFLAGS := -std=gnu11 -Wall -Wextra -Werror -O1 -g
CFLAGS += -fsanitize=address,undefined -fno-omit-frame-pointer
CFLAGS += -I ..

SRC := host_grade_test.c ../helpers/lf_grade.c
BIN := host_grade_test

.PHONY: all run clean
all: run

$(BIN): $(SRC) ../helpers/lf_grade.h
	$(CC) $(CFLAGS) $(SRC) -o $@

run: $(BIN)
	./$(BIN)

clean:
	rm -rf $(BIN) $(BIN).dSYM
