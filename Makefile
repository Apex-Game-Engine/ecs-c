#!/bin/make

# Directories
SRCDIR=src
OBJDIR=obj
BINDIR=bin

# Compiler flags
CC=clang-12
CFLAGS=-std=c99 -Wall -Werror -O2 -I$(SRCDIR) -Iinclude -I/usr/include/lua5.3
LDFLAGS=-lm

# Files
SOURCES=$(wildcard $(SRCDIR)/*.c)
OBJECTS=$(patsubst $(SRCDIR)/%.c, $(OBJDIR)/%.o, $(SOURCES))
#TARGETS=$(BINDIR)/hashmap_test
#TARGETS=$(BINDIR)/ecs_test
TARGETS=$(BINDIR)/lua_ecs_test

.PHONY: all clean run

all: $(TARGETS)

# Linking
$(BINDIR)/hashmap_test: $(OBJDIR)/hashmap_test.o $(OBJDIR)/hashmap.o | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BINDIR)/ecs_test: $(OBJDIR)/ecs_test.o $(OBJDIR)/ecs.o $(OBJDIR)/hashmap.o | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS)

$(BINDIR)/lua_ecs_test: $(OBJDIR)/lua_ecs_test.o $(OBJDIR)/ecs.o $(OBJDIR)/hashmap.o | $(BINDIR)
	$(CC) $(CFLAGS) -o $@ $^ $(LDFLAGS) -llua5.3

# Compiling
$(OBJDIR)/%.o: $(SRCDIR)/%.c | $(OBJDIR)
	$(CC) $(CFLAGS) -c $< -o $@

# Create directories
$(BINDIR):
	mkdir -p $@

$(OBJDIR):
	mkdir -p $@

# Clean
clean:
	rm -rf $(OBJDIR) $(BINDIR)

run: $(TARGETS)
	$(TARGETS)

