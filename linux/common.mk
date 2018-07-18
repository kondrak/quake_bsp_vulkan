INCLUDES = -I$(VULKAN_SDK)/x86_64/include -I../contrib -I../src
CFLAGS := $(OPTFLAGS) $(shell pkg-config --cflags sdl2)
CXXFLAGS = $(CFLAGS) -std=c++14
LDFLAGS = -L$(VULKAN_SDK)/x86_64/lib -lvulkan $(shell pkg-config --libs sdl2)

include sources.mk
BUILDDIR = $(BUILD)/build

TMPOBJS = $(SOURCES:.cpp=.o)
OBJS = $(TMPOBJS:.c=.o)
OBJS := $(patsubst %,$(BUILDDIR)/%,$(OBJS))

all: $(TARGET)

clean:
	rm -rf $(BUILD)
	rm -f $(TARGET)

$(TARGET): $(OBJS)
	$(CXX) $(OBJS) $(INCLUDES) $(CXXFLAGS) $(LDFLAGS) -o $(TARGET)

$(BUILDDIR)/%.o: %.cpp
	@mkdir -p $(@D)
	$(CXX) -c $(INCLUDES) $(CXXFLAGS) $(DEFINES) $< -o $@

$(BUILDDIR)/%.o: %.c
	@mkdir -p $(@D)
	$(CC) -c $(INCLUDES) $(CFLAGS) $(DEFINES) $< -o $@

$(BUILDDIR)/%.d: %.cpp
	@echo Resolving dependencies of $<
	@mkdir -p $(@D)
	@$(CXX) -MM $(INCLUDES) $(CXXFLAGS) $(DEFINES) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(BUILDDIR)/$(<:.cpp=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

$(BUILDDIR)/%.d: %.c
	@echo Resolving dependencies of $<
	@mkdir -p $(@D)
	@$(CC) -MM $(INCLUDES) $(CFLAGS) $(DEFINES) $< > $@.$$$$; \
	sed 's,.*\.o[ :]*,$(BUILDDIR)/$(<:.c=.o) $@ : ,g' < $@.$$$$ > $@; \
	rm -f $@.$$$$

ifneq "$(MAKECMDGOALS)" "clean"
-include $(OBJS:.o=.d)
endif

.PHONY: all clean
