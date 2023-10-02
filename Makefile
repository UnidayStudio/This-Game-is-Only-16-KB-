# Makefile Created by Guilherme Teres (@UnidayStudio)
# It's a bit of overkill for a single file program with no external dependencies,
# but I'l practicing my Makefile skills, so I'm considering it a win. :)

# Compiler and linker options
CC := cl
LD := link

# Intermediate files directory
INTERMEDIATE_DIR := Bin/Intermediates/
OUTPUT_DIR  := Bin/

OUTPUT_FILE := $(OUTPUT_DIR)/Sokoban_LD54.exe

GAME_SRC_DIR     := Source
GAME_INCLUDE_DIR := -I$(GAME_SRC_DIR)

# Compiler flags
#CFLAGS := /std:c++17 /O2 /EHsc /MD $(GAME_INCLUDE_DIR)
CFLAGS := /std:c++17 /EHsc /MD $(GAME_INCLUDE_DIR)
CFLAGS += /O1 /Os /nologo /W3 /Gw /GS-

# Linker flags
LDFLAGS := /NODEFAULTLIB /SUBSYSTEM:WINDOWS /INCREMENTAL:NO 
LDFLAGS += /nologo /FILEALIGN:16 /ALIGN:128

LIBS := Kernel32.lib user32.lib gdi32.lib #shell32.lib gdi32.lib Advapi32.lib ole32.lib oleaut32.lib

# Source files:				
GAME_SRCS :=  $(wildcard $(GAME_SRC_DIR)/*.cpp)

# Obj files:
GAME_OBJS := $(patsubst $(GAME_SRC_DIR)/%.cpp,$(INTERMEDIATE_DIR)/%.obj,$(GAME_SRCS))

# Makefile rules
all: $(OUTPUT_FILE) 

$(OUTPUT_FILE): $(GAME_OBJS)
	$(LD) /OUT:$(OUTPUT_FILE) $(LDFLAGS) $(GAME_OBJS) $(LIBS)
	
$(INTERMEDIATE_DIR)/%.obj: $(GAME_SRC_DIR)/%.cpp
	@if not exist $(dir $@) mkdir $(subst /,\,$(dir $@))
	$(CC) $(CFLAGS) /c $< /Fo:$@
	
# Rule to create intermediate directory:
$(INTERMEDIATE_DIR): 
	mkdir -p $(INTERMEDIATE_DIR)

clean:
	del /Q $(subst /,\,$(INTERMEDIATE_DIR))
	del /Q $(subst /,\,$(OUTPUT_FILE))

.PHONY: all clean
