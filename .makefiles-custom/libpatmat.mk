# Static library build. The c skill syntax-checks and runs cppcheck.
# It does not compile or link.

override CFLAGS += -std=c11 -Wall -Wextra -Wpedantic -Werror -Iinclude
override CPPCHECK_FLAGS += -Iinclude --std=c11

PATMAT_BUILD := build
PATMAT_LIB := $(PATMAT_BUILD)/libpatmat.a
PATMAT_OBJ := $(PATMAT_BUILD)/patmat.o
PATMAT_TEST := $(PATMAT_BUILD)/test_patmat

.PHONY: lib test check clean

lib: $(PATMAT_LIB)

$(PATMAT_OBJ): src/patmat.c include/patmat.h
	mkdir -p $(PATMAT_BUILD)
	$(CC) $(CFLAGS) -c -o $@ src/patmat.c

$(PATMAT_LIB): $(PATMAT_OBJ)
	$(AR) rcs $@ $(PATMAT_OBJ)

$(PATMAT_TEST): tests/test_patmat.c include/patmat.h $(PATMAT_LIB)
	$(CC) $(CFLAGS) -o $@ tests/test_patmat.c $(PATMAT_LIB) $(LDFLAGS)

test: $(PATMAT_TEST)
	./$(PATMAT_TEST)

check: $(if $(filter c,$(SKILLS)),c-check) test

clean:
	rm -rf $(PATMAT_BUILD)
