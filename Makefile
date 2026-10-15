#*******************************************************************************
#   Ledger App
#   (c) 2017 Ledger
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#*******************************************************************************

ifeq ($(BOLOS_SDK),)
$(error Environment variable BOLOS_SDK is not set)
endif

include $(BOLOS_SDK)/Makefile.target

APPNAME = "Celo"

APPVERSION_M=1
APPVERSION_N=8
APPVERSION_P=4
APPVERSION=$(APPVERSION_M).$(APPVERSION_N).$(APPVERSION_P)

# Application source files
APP_SOURCE_PATH += src src_common

# Application allowed derivation curves
CURVE_APP_LOAD_PARAMS = secp256k1

# Application allowed derivation paths
PATH_APP_LOAD_PARAMS = "44'/52752'" "44'/60'"

# Setting to allow building variant applications
# - <VARIANT_PARAM> is the name of the parameter which should be set
#   to specify the variant that should be build.
# - <VARIANT_VALUES> a list of variant that can be build using this app code.
#   * It must at least contains one value.
#   * Values can be the app ticker or anything else but should be unique.
VARIANT_PARAM = COIN
VARIANT_VALUES = celo

ICON_STAX = icons/stax_app_celo.gif
ICON_FLEX = icons/flex_app_celo.gif
ICON_NANOX = icons/nanox_app_celo.gif
ICON_NANOSP = icons/nanox_app_celo.gif
ICON_APEX_P = icons/apex_p_app_celo.png

########################################
# Application communication interfaces #
########################################
ENABLE_BLUETOOTH = 1

########################################
#         NBGL custom features         #
########################################
ENABLE_NBGL_QRCODE = 1

########################################
#          Compiler hardening          #
########################################
# The SDK defaults this to 0, which drops -fstack-protector-strong and the
# randomized canary entry point.
ENABLE_STACK_PROTECTOR = 1

########################################
#            Swap features             #
########################################
ENABLE_SWAP = 1

include $(BOLOS_SDK)/Makefile.standard_app

########################################
#           App size report            #
########################################
# Both budgets are tight on Nano devices, so report them at the end of every build.
.PHONY: app-size-report
app-size-report: $(BIN_TARGETS) $(DBG_TARGETS)
	@echo ""
	@echo "Finished Concordium Ledger app ($(TARGET_NAME)) → $(BIN_DIR)/app.elf"
	@$(GCCPATH)arm-none-eabi-size $(BIN_DIR)/app.elf | \
	  awk 'NR==2 { printf "  flash  %6d B\n", $$1 }'
	@$(GCCPATH)arm-none-eabi-nm $(BIN_DIR)/app.elf 2>/dev/null | \
	  python3 -c "import sys; s={p[2]:int(p[0],16) for l in sys.stdin for p in [l.split()] if len(p)==3}; bss=s.get('_bss'); ebss=s.get('_ebss'); stk=s.get('_stack'); estk=s.get('_estack'); print(f'  SRAM   {(ebss-bss)+(estk-stk):6d} B total: {ebss-bss} B BSS variables, {estk-stk} B stack headroom') if all(v is not None for v in [bss,ebss,stk,estk]) else print('  SRAM   (nm symbols _bss/_ebss/_stack/_estack not found; SDK linker script may have changed)')"

default: app-size-report