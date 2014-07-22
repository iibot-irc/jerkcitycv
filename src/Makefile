TARGET   = jerkcity
CXXFLAGS = -g -O3 --std=c++1y
LDFLAGS  = `pkg-config --libs opencv` -lboost_program_options -lboost_filesystem -lboost_system

CXX=clang++
OBJDIR=_obj

SOURCES = $(wildcard *.cc) $(wildcard */*.cc)
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCES:.cc=.o))

.PHONY: clean all

all: $(TARGET)

clean:
	rm -rf $(OBJDIR)

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(OBJDIR)/`dirname $<`
	@echo Compiling $<
	@$(CXX) -o $@ -c $(CXXFLAGS) $<

$(TARGET): $(OBJECTS)
	@echo Linking $@
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
