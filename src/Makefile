TARGET   = jerkcity
CXXFLAGS = -g -O3 --std=c++1y
LDFLAGS  = `pkg-config --libs opencv` -lboost_program_options -lboost_filesystem -lboost_system

CXX=clang++
OBJDIR=_obj

SOURCES = $(wildcard *.cc) $(wildcard */*.cc) # note: only goes one deep. TODO: find copy of this Makefile that went infinitely deep
OBJECTS = $(addprefix $(OBJDIR)/,$(SOURCES:.cc=.o))
DEPS    = $(OBJECTS:.o=.d)

all: $(TARGET)

-include $(DEPS)

.PHONY: clean all

clean:
	rm -rf $(OBJDIR)

$(OBJDIR)/%.o: %.cc
	@mkdir -p $(OBJDIR)/`dirname $<`
	@echo Compiling $<
	@$(CXX) -c $(CXXFLAGS) -MMD -MP -o $@ $<

$(TARGET): $(OBJECTS)
	@echo Linking $@
	@$(CXX) -o $@ $(OBJECTS) $(LDFLAGS)
