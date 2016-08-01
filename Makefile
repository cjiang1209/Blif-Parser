EXEC=blif_parser
SRCS=$(wildcard *.cpp)
OBJS=$(SRCS:.cpp=.o)
DIR=/

CXX=g++
CFLAGS=-Wall -I"/home/cjiang/workspace/meddly/include" -std=c++11
LFLAGS=-Wall
LLIBS=-L"/home/cjiang/workspace/meddly/lib" -lmeddly

COPTIMIZE=-O3

.PHONY: release debug profile help clean

# Target
release debug profile: $(EXEC)

# Compile options
release: CFLAGS+=$(COPTIMIZE) -DNDEBUG
debug: CFLAGS+=-O0 -g3
profile: CFLAGS+=$(COPTIMIZE) -pg -DNDEBUG

# Directory
release: DIR=Release/
debug: DIR=Debug/
profile: DIR=Profile/

# Build
%.o: %.cpp
	@echo Compiling: $@
	@mkdir -p $(DIR)
	$(CXX) $(CFLAGS) -c $< -o $(addprefix $(DIR),$@)
	@echo ""

# Link
$(EXEC): $(OBJS)
	@echo Linking $^
	$(CXX) $(LFLAGS) $(addprefix $(DIR),$^) -o $(addprefix $(DIR),$@) $(LLIBS)
	@echo ""

# Help
help:
	@echo "make          -- Compile and link in release mode."
	@echo "make release  -- Compile and link in release mode."
	@echo "make debug    -- Compile and link in debug mode."
	@echo "make profile  -- Compile and link in profile mode."
	@echo "make clean    -- Clean object files."
	@echo "make cleanall -- Clean all files."
	@echo "make help     -- Print help message."

# Clean
clean:
	rm -f Release/*.o Debug/*.o Profile/*.o

cleanall:
	rm -rf Release/ Debug/ Profile/

