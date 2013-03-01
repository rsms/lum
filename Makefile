# ifeq ($(CC),gcc)
# override DEBUG = 1 # Always build tests in debug mode
include common.make
include deps/llvm/config.make
SRCDIR := src

# Sources
lib_sources :=\
	$(SRCDIR)/lum.cc \
	$(SRCDIR)/str.cc \
	$(SRCDIR)/sym.cc \
	$(SRCDIR)/cell.cc \
	$(SRCDIR)/bif.cc \
	$(SRCDIR)/fn.cc \
	$(SRCDIR)/namespace.cc \
	$(SRCDIR)/print.cc \

headers_pub_common := $(patsubst $(SRCDIR)/%,%,$(wildcard $(SRCDIR)/common/*.h))

headers_pub :=\
	lum.h \
  common.h $(headers_pub_common) \
  hash.h str.h sym.h sym-defs.h bif.h bif-defs.h fn.h cell.h env.h var.h \
  namespace.h eval.h print.h read.h map.h \

main_sources := $(SRCDIR)/main.cc

test_sources := $(wildcard $(SRCDIR)/*.test.cc)

# --- conf ---------------------------------------------------------------------

project_id = lum
all: $(project_id)

# Source files to Object files
object_dir = $(OBJECT_DIR)-$(project_id)
lib_objects := $(lib_sources:%.c=%.o)
lib_objects := $(lib_objects:%.cc=%.o)
lib_objects := $(lib_objects:%.mm=%.o)
lib_objects := $(call SrcToObjects,$(object_dir),$(lib_objects))
main_objects = $(call SrcToObjects,$(object_dir),${main_sources:.cc=.o})
test_objects = $(call SrcToObjects,$(object_dir),$(test_sources:.cc=.o))
test_programs = $(sort $(patsubst %.cc,$(TESTS_BUILD_PREFIX)/%.test,$(test_sources)))
test_program_dirs = $(call FileDirs,$(test_programs))
object_dirs = $(call FileDirs,$(lib_objects)) $(call FileDirs,$(main_objects))

# For LLVM IR and Assembly output
asmout_dir = $(DEBUG_BUILD_PREFIX)/$(project_id)-asm
asmout_ll = ${lib_objects:.o=.ll}
asmout_ll_dirs = $(call FileDirs,$(asmout_ll))
asmout_s = ${lib_objects:.o=.s}
asmout_s_dirs = $(call FileDirs,$(asmout_s))

# Public headers
headers_pub_dir = $(INCLUDE_BUILD_PREFIX)/$(project_id)
headers_pub_export = $(call PubHeaderNames,$(headers_pub_dir),$(headers_pub))
headers_pub_export_dirs = $(call FileDirs,$(headers_pub_export))

# Library and program
static_library = $(LIB_BUILD_PREFIX)/lib$(project_id).a
main_program = $(BIN_BUILD_PREFIX)/$(project_id)

# Boost prefix
boost_prefix = $(DEPS_ROOT)/boost

# Compiler and linker flags
c_flags    := $(CFLAGS) -MMD -fobjc-arc
cxx_flags  := $(CXXFLAGS) -MMD -fobjc-arc
ld_flags   := $(LDFLAGS)
xxld_flags := $(XXLDFLAGS)

# Dependency: boost
cxx_flags  += -I$(boost_prefix) -DBOOST_EXCEPTION_DISABLE=1

# Dependency libllvm
#c_flags    += $(libllvm_cflags)
#cxx_flags  += $(libllvm_cxxflags)
#ld_flags   += $(libllvm_ldflags)

# --- targets ---------------------------------------------------------------------

clean:
	rm -rf $(object_dir)
	rm -rf $(headers_pub_export)
	rm -rf $(asmout_dir)
	rm -f $(main_program)
	rm -f $(static_library)

common_pre:
	@mkdir -p $(object_dirs)
	@mkdir -p $(headers_pub_export_dirs)

# Create program
$(project_id): common_pre lib$(project_id) $(main_program)
$(main_program): $(main_objects) $(static_library)
	@mkdir -p $(dir $(main_program))
	$(LD) $(ld_flags) $(xxld_flags) -l$(project_id) -o $@ $^

# Symlink boost
$(INCLUDE_BUILD_PREFIX)/boost:
	rm -rf $@
	ln -fs $(boost_prefix)/boost $@

# Create library archive
lib$(project_id): common_pre $(headers_pub_export) \
                  $(INCLUDE_BUILD_PREFIX)/boost \
                  $(static_library)
$(static_library): $(lib_objects)
	@mkdir -p $(dir $(static_library))
	$(AR) -rcL $@ $^

# Build and run tests
test_pre:
	@mkdir -p $(test_program_dirs)
test: c_flags += -DLUM_TEST_SUIT_RUNNING=1
test: lib$(project_id) test_pre $(test_programs)
$(TESTS_BUILD_PREFIX)/%.test: $(object_dir)/%.o
	@$(LD) $(ld_flags) -l$(project_id) -o $@ $^
	@printf "Running test: %s ... " $(patsubst %.test,%.cc,$(@F))
	@$@ >/dev/null
	@echo PASS
	@rm -f $@ # $^

# test/str

-include ${test_objects:.o=.d}

# Generate LLVM IS code (.ll files)
llvm_ir_pre: common_pre
	@mkdir -p $(asmout_ll_dirs)
llvm_ir: llvm_ir_pre $(asmout_ll)

# Generate target assembly code (.s files)
asm_pre: common_pre
	@mkdir -p $(asmout_s_dirs)
asm: asm_pre $(asmout_s)

# C++ source -> object
$(object_dir)/%.o: %.cc
	$(CXXC) $(c_flags) $(cxx_flags) -c -o $@ $<

# Objective-C++ source -> object
$(object_dir)/%.o: %.mm
	$(CXXC) $(c_flags) $(cxx_flags) -c -o $@ $<

# C source -> object
$(object_dir)/%.o: %.c
	$(CC) $(c_flags) -c -o $@ $<

# Objective-C source -> object
$(object_dir)/%.o: %.m
	$(CC) $(c_flags) -c -o $@ $<

# C source -> LLVM IR
$(asmout_dir)/%.ll: %.c
	clang $(CFLAGS) -S -emit-llvm  -DS_CODEGEN_ASM=1 -DS_CODEGEN_LLVMIR=1 -o $@ $<

# C source -> target assembly
$(asmout_dir)/%.s: %.c
	$(CC) $(CFLAGS) -S -DS_CODEGEN_ASM=1 -o $@ $<

# Copy headers into $headers_pub_dir
$(headers_pub_dir)/%.h: $(SRCDIR)/%.h
	@cp $^ $@

# header dependencies
-include ${lib_objects:.o=.d}
-include ${main_objects:.o=.d}

.PHONY: all clean common_pre $(project_id) lib$(project_id) test_pre test \
	      llvm_ir_pre asm_pre
