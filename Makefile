# The executable name is suffix depending on the target
OUT = leanXstream
HOST_SUFFIX = _host
TARGET_SUFFIX = _target
TARGETSIM_SUFFIX = _sim_target

# Disable make's built-in rules
MAKEFLAGS += -r

# this includes the framework configuration
-include .config

# decide whether we are building or dooing something other like cleaning or configuring
ifeq ($(filter $(MAKECMDGOALS), clean distclean config), )
  # check whether a .config file has been found
  ifeq ($(filter .config, $(MAKEFILE_LIST)), )
    $(error "Cannot make the target '$(MAKECMDGOALS)' without configuring the application. Please run make config to do this.")
  endif
endif

# Host-Compiler executables and flags
HOST_CC = gcc 
HOST_CFLAGS = $(HOST_FEATURES) -Wall -pedantic -std=gnu99 -DOSC_HOST -g
HOST_LDFLAGS = -lm

# Cross-Compiler executables and flags
TARGET_CC = bfin-uclinux-gcc 
TARGET_CFLAGS = -Wall -pedantic -std=gnu99 -O2 -DOSC_TARGET
TARGETDBG_CFLAGS = -Wall -pedantic -std=gnu99 -ggdb3 -DOSC_TARGET
TARGETSIM_CFLAGS = -Wall -pedantic -O2 -DOSC_TARGET -DOSC_SIM
TARGET_LDFLAGS = -Wl,-elf2flt="-s 1048576" -lbfdsp

# Source files of the application
SOURCES = leanXmain.c leanXtools.c leanXip.c

# Default target
all : $(OUT)

$(OUT) : target host writebmps 

# this target ensures that the application has beeb built prior to deployment
$(OUT)_% :
	@ echo "Please use make {target,targetdbg,targetsim} to build the application first"; exit 1

# Compiles the executable
target: $(SOURCES) inc/*.h lib/libosc_target.a
	@echo "Compiling for target.."
	$(TARGET_CC) $(SOURCES) lib/libosc_target.a $(TARGET_CFLAGS) \
	$(TARGET_LDFLAGS) -o $(OUT)$(TARGET_SUFFIX)
	@echo "Target executable done."
	
targetdbg: $(SOURCES) inc/*.h lib/libosc_target.a
	@echo "Compiling for target.."
	$(TARGET_CC) $(SOURCES) lib/libosc_target.a $(TARGETDBG_CFLAGS) \
	$(TARGET_LDFLAGS) -o $(OUT)$(TARGET_SUFFIX)
	@echo "Target executable done."
	
targetsim: $(SOURCES) inc/*.h lib/libosc_target_sim.a
	@echo "Compiling for target.."
	$(TARGET_CC) $(SOURCES) lib/libosc_target_sim.a $(TARGETSIM_CFLAGS) \
	$(TARGET_LDFLAGS) -o $(OUT)$(TARGETSIM_SUFFIX)
	@echo "Target executable done."
	
host: $(SOURCES) inc/*.h lib/libosc_host.a
	@echo "Compiling for host.."
	$(HOST_CC) $(SOURCES) lib/libosc_host.a $(HOST_CFLAGS) \
	$(HOST_LDFLAGS) -o $(OUT)$(HOST_SUFFIX)
	@echo "Host executable done."

writebmps: writebmps.c inc/*.h lib/libosc_host.a
	@echo "Compiling writebmps for host.."
	$(HOST_CC) writebmps.c lib/libosc_host.a $(HOST_CFLAGS) \
	$(HOST_LDFLAGS) -o writebmps
	@echo "writebmps."

# Target to explicitly start the configuration process
.PHONY : config
config :
	@ ./configure
	@ $(MAKE) --no-print-directory get

# Set symlinks to the framework
.PHONY : get
get :
	@ rm -rf inc lib
	@ ln -s $(CONFIG_FRAMEWORK)/staging/inc ./inc
	@ ln -s $(CONFIG_FRAMEWORK)/staging/lib ./lib
	@ echo "Configured Oscar framework."

# Cleanup
.PHONY : clean
clean :	
	rm -f $(OUT)$(HOST_SUFFIX) $(OUT)$(TARGET_SUFFIX) $(OUT)$(TARGETSIM_SUFFIX) writebmps
	rm -f *.o *.gdb
	rm -f test.dat
	rm -f movie*.bmp
	@ echo "Directory cleaned"

# Cleans everything not intended for source distribution
.PHONY : distclean
distclean : clean
	rm -f .config
	rm -rf inc lib
