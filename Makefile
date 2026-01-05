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
APPVERSION_N=6
APPVERSION_P=1
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
#            Swap features             #
########################################
ENABLE_SWAP = 1

ifneq ($(NOCONSENT),)
DEFINES   += NO_CONSENT
endif

include $(BOLOS_SDK)/Makefile.standard_app
