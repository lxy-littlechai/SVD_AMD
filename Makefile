#environment
ROOTFS=${EDGE_COMMON_SW_PATH}/rootfs.ext4
IMAGE=${EDGE_COMMON_SW_PATH}/Image
SYSROOT = ${SYSROOT_PATH}/sysroots/cortexa72-cortexa53-xilinx-linux
SDKTARGETSYSROOT = ${SYSROOT}
BASE_PLATFORM ?= ${PLATFORM_REPO_PATHS}/xilinx_vck190_base_202320_1/xilinx_vck190_base_202320_1.xpfm

# Makefile input options
TARGET := hw_emu

XO_DIR=./300$(TARGET).xo_dir
XSA_DIR=./300$(TARGET).xsa_dir
PACKAGE_DIR = ./300$(TARGET).xclbin_dir

# File names and locations
KERNEL_SRC = ./pl/TopFunc.cpp
KERNEL_XO := ./$(XO_DIR)/TopFunc.xo

XSA := ./$(XSA_DIR)/svd.xsa
HOST_SRC = ./ps/host.cpp
HOST = host.exe
XCLBIN = ./$(PACKAGE_DIR)/svd.xclbin
GRAPH := aie/aie_graph.cpp
GRAPH_O := libadf.a
CONFIG_FILE := conn.cfg


# Command-line options
VPP := v++
AIECC := v++ -c --mode aie
AIESIM := aiesimulator
X86SIM := x86simulator

HW_EMU_CMD := ./launch_hw_emu.sh -g -add-env AIE_COMPILER_WORKDIR=../Work 
#HW_EMU_CMD := ./launch_hw_emu.sh -add-env AIE_COMPILER_WORKDIR=../Work -aie-sim-options ../aiesimulator_output/aiesim_options.txt

# compile aie
AIE_INCLUDE_FLAGS := --include "$(XILINX_VITIS)/aietools/include" --include "./aie" --include "./aie/src" --include "./" --aie.xlopt=0
#AIE_STRATEGY := --aie.Xmapper=DisableFloorplanning
AIE_FLAGS := $(AIE_INCLUDE_FLAGS) $(AIE_STRATEGY) --platform $(BASE_PLATFORM) --work_dir ./Work


ifeq ($(TARGET),sw_emu)
	AIE_FLAGS += --target x86sim
else
	AIE_FLAGS += --target hw
endif 

# kernel build config
VPP_XO_FLAGS += -c --platform $(BASE_PLATFORM)
VPP_XO_FLAGS += --hls.jobs 8 --freqhz=300000000:TopFunc --hls.clock 300000000:TopFunc
VPP_XO_FLAGS += -I$(CUR_DIR)/pl 

	
VPP_LINK_FLAGS := -l -t $(TARGET) --platform $(BASE_PLATFORM) $(KERNEL_XO) $(GRAPH_O) --save-temps -g --config $(CONFIG_FILE) -o $(XSA)
VPP_LINK_FLAGS += --profile.exec all:all:all --freqhz=300000000:TopFunc.ap_clk
VPP_FLAGS := $(VPP_LINK_FLAGS)

CXX := $(XILINX_VITIS)/gnu/aarch64/lin/aarch64-linux/bin/aarch64-linux-gnu-g++

GCC_FLAGS := -Wall -c \
	     	 -std=c++20 \
			 -Wno-int-to-pointer-cast \
			 --sysroot=$(SYSROOT) \ 


GCC_INCLUDES := -I$(SYSROOT)/usr/include/xrt \
				-I./  \
				-I${XILINX_VITIS}/aietools/include \
				-I${XILINX_VITIS}/include \
				-I./aie/src \

GCC_LIB := -lxaiengine -ladf_api_xrt -lxrt_core -lxrt_coreutil \
		   -L$(SYSROOT)/usr/lib \
		   --sysroot=$(SYSROOT) \
		   -L${XILINX_VITIS}/aietools/lib/aarch64.o 

LDCLFLAGS := $(GCC_LIB)

.ONESHELL:
.PHONY: clean all kernels aie sim xsa host package run_emu

###
# Guarding Checks. Do not modify.
###
check_defined = \
	$(strip $(foreach 1,$1, \
		$(call __check_defined,$1,$(strip $(value 2)))))

__check_defined = \
	$(if $(value $1),, \
		$(error Undefined $1$(if $2, ($2))))

guard-PLATFORM_REPO_PATHS:
	$(call check_defined, PLATFORM_REPO_PATHS, Set your where you downloaded xilinx_vck190_base_202320_1)

guard-ROOTFS:
	$(call check_defined, ROOTFS, Set to: xilinx-versal-common-v2023.2/rootfs.ext4)

guard-IMAGE:
	$(call check_defined, IMAGE, Set to: xilinx-versal-common-v2023.2/Image)

guard-CXX:
	$(call check_defined, CXX, Run: xilinx-versal-common-v2023.2/environment-setup-aarch64-xilinx-linux)

guard-SDKTARGETSYSROOT:
	$(call check_defined, SDKTARGETSYSROOT, Run: xilinx-versal-common-v2023.2/environment-setup-aarch64-xilinx-linux)

###

all: kernels aie xsa host package
sd_card: all

######################################################
# This step compiles the HLS C kernels and creates the *.xo's 
# which is used as the output and from the *.cpp files.
# Note : hw_emu and hw targets use the Unified CLI command to 
# compile HLS kernels

kernels: guard-PLATFORM_REPO_PATHS $(KERNEL_XO) $(KERNEL_XO_1)
$(KERNEL_XO): $(KERNEL_SRC)
	mkdir -p $(XO_DIR)
	$(VPP) $(VPP_XO_FLAGS) -k TopFunc $< -o $@ | tee $(XO_DIR)/TopFunc.log
	


aie: $(GRAPH_O)

#AIE or X86 Simulation
sim: $(GRAPH_O)
     
ifeq ($(TARGET),sw_emu)
	$(X86SIM) --pkg-dir=./Work
else
	$(AIESIM) --profile --dump-vcd=svd --pkg-dir=./Work
# $(AIESIM) --profile --pkg-dir=./Work --online -wdb
  
endif 

#AIE or X86 compilation
$(GRAPH_O): $(GRAPH)
	$(AIECC) $(AIE_FLAGS) $(GRAPH)
#####################################################

########################################################
# Once the kernels and graph are generated, you can build
# the hardware part of the design. This creates an xsa
# that will be used to run the design on the platform.
xsa: guard-PLATFORM_REPO_PATHS $(XSA)
$(XSA): 
	mkdir -p $(XSA_DIR)
	$(VPP) $(VPP_LINK_FLAGS) || (echo "task: [xsa] failed error code: $$?"; exit 1)
	@echo "COMPLETE: .xsa created."
########################################################

############################################################################################################################
# For sw emulation, hw emulation and hardware, compile the PS code and generate the host.exe. This is needed for creating the sd_card.
host: guard-CXX guard-SDKTARGETSYSROOT  
	$(CXX) $(GCC_FLAGS) $(GCC_INCLUDES) -o host.o $(HOST_SRC)
	$(CXX) *.o $(GCC_LIB) -std=c++20 -o $(HOST)
	@echo "COMPLETE: Host application created."
############################################################################################################################

##################################################################################################
# Depending on the TARGET, it'll either generate the PDI for sw_emu,hw_emu or hw.

package: guard-PLATFORM_REPO_PATHS guard-IMAGE guard-ROOTFS $(XCLBIN)
$(XCLBIN):
	mkdir -p $(PACKAGE_DIR)
	v++ -p -t ${TARGET} \
		-f ${BASE_PLATFORM} \
		--package.out_dir=$(PACKAGE_DIR) \
		--package.rootfs=${ROOTFS} \
		--package.image_format=ext4 \
		--package.boot_mode=sd \
		--package.kernel_image=${IMAGE} \
		--package.defer_aie_run \
		--package.sd_file $(HOST) $(XSA) $(GRAPH_O) -o $(XCLBIN) \
		--package.sd_dir ./data 
	@echo "COMPLETE: emulation package created."

###################################################################################################

#Build the design and then run sw/hw emulation 
run: all run_emu

###########################################################################
run_emu: 
# If the target is for HW_EMU, launch the emulator
ifeq (${TARGET},hw_emu)
	cd ./$(PACKAGE_DIR)
	$(HW_EMU_CMD)
else
	@echo "Hardware build, no emulation executed."
endif


###########################################################################

clear_aie:
	rm -rf Work *.a
clear_pl:
	rm -rf $(XO_DIR)
clear_ps: 
	rm -rf *.o *.exe
clear_xsa:
	rm -rf $(XSA_DIR)
clear_package: 
	rm -rf $(PACKAGE_DIR)
clear_all: clear_aie clear_pl clear_ps clear_xsa clear_package
	rm -rf _x v++* .Xil *.db *.log *.wcfg *.wdb pl_sample_counts *.csv .AIE_* ISS_* aiesimulator_* function_*
	 