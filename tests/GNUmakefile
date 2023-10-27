
override CFILES := $(shell find . -type f -name '*.c')
override CPPFILES := $(shell find . -type f -name '*.cpp')
override ASFILES := $(shell find . -type f -name '*.S')
override NASMFILES := $(shell find . -type f -name '*.asm')

override CFILES += \
	../lib/ctype.c ../lib/convert.c ../lib/adt/string_view.c \
	../lib/adt/range.c ../lib/util.c ../lib/format.c ../lib/parse_printf.c \
	../lib/adt/avltree.c ../lib/adt/string.c ../lib/time.c \
	../lib/parse_strftime.c ../lib/adt/mutable_buffer.c \
	../lib/adt/growable_buffer.c ../lib/string.c ../lib/align.c \
	../lib/strftime.c ../lib/adt/bitmap.c ../lib/math.c ../lib/bits.c \
	../lib/memory.c

override OBJ := $(foreach obj, $(CFILES:./%=%), obj/$(basename $(subst ../,,$(obj))).o) \
				$(foreach obj, $(CPPFILES:./%=%), obj/$(basename $(obj)).cpp.o) \
				$(foreach obj, $(ASFILES:./%=%), obj/$(basename $(obj)).S.o) \
				$(foreach obj, $(NASMFILES:./%=%), obj/$(basename $(obj)).asm.o)

define DEFAULT_VAR =
	ifeq ($(origin $1),default)
		override $(1) := $(2)
	endif
	ifeq ($(origin $1),undefined)
		override $(1) := $(2)
	endif
endef

override DEFAULT_CXX := cxx
$(eval $(call DEFAULT_VAR,CXX,$(DEFAULT_CXX)))

# Same thing for "ld" (the linker).
override DEFAULT_LD := ld
$(eval $(call DEFAULT_VAR,LD,$(DEFAULT_LD)))

override DEFAULT_CFLAGS :=
$(eval $(call DEFAULT_VAR,CFLAGS,$(DEFAULT_CFLAGS)))

override CFLAGS += -g3 -DBUILD_TEST -I../ -fno-stack-protector

all: test
test: $(CFILES)
	$(CC) -std=gnu17 $(CFLAGS) $(LDFLAGS) $(CFILES) -o $@

clean:
	rm -rf $(OBJ)
	rm test
