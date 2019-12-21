#
# Copyright (2017) Baidu Inc. All rights reserved.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#   http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#

$(LOCAL_MODULE)_TARGET_DIR := $(OUT_DIR)/modules/$(LOCAL_MODULE)

$(LOCAL_MODULE)_OBJ_FILES := $(patsubst $(MODULE_PATH)/%.c, $($(LOCAL_MODULE)_TARGET_DIR)/%.c.o, $(LOCAL_SRC_FILES))

$(LOCAL_MODULE)_CFLAGS := $(foreach o,$(LOCAL_CDEFS),-D$(strip $(o))) $(LOCAL_CFLAGS) $(COM_DEFS)

ifneq ($(strip $(LOCAL_HM_MODULE_NAME)),true)
UPPER_MODULE_NAME = $(shell echo $(LOCAL_MODULE) | tr [a-z\-] [A-Z_])
$(LOCAL_MODULE)_CFLAGS += -DDUER_HM_MODULE_NAME=DUER_HM_$(UPPER_MODULE_NAME)
endif

$(LOCAL_MODULE)_LDFLAGS := $(LOCAL_LDFLAGS)

$(LOCAL_MODULE)_INCLUDES := $(foreach i,$(LOCAL_INCLUDES),-I$(strip $(i)))

ifneq ($(strip $(LOCAL_SRC_FILES)),)
$(LOCAL_MODULE)_TARGET_LIB := $($(LOCAL_MODULE)_TARGET_DIR)/lib$(LOCAL_MODULE).a

$(LOCAL_MODULE)_TARGET_EXE := $($(LOCAL_MODULE)_TARGET_DIR)/$(LOCAL_MODULE)
else

ifeq ($(strip $(LOCAL_PACK_ALL)),true)
$(LOCAL_MODULE)_TARGET_LIB := $($(LOCAL_MODULE)_TARGET_DIR)/lib$(LOCAL_MODULE).a
endif

endif

$(LOCAL_MODULE)_STATIC_LIBRARIES := $(LOCAL_STATIC_LIBRARIES)

$(LOCAL_MODULE)_DEPS = $(foreach m,$($(LOCAL_MODULE)_STATIC_LIBRARIES),$(OUT_DIR)/modules/$(m)/lib$(m).a) $($(LOCAL_MODULE)_OBJ_FILES)
#$(LOCAL_MODULE)_DEPS = $(foreach m,$($(LOCAL_MODULE)_STATIC_LIBRARIES),$($(m)_TARGET_LIB)) $($(LOCAL_MODULE)_OBJ_FILES)

define obtain_prefix
$(shell echo $(1) | sed -e "s#.*/modules/\([^\\/]*\)/.*#\1#" )
endef

define obtain_includes
$(foreach prefix,$(1),$($(prefix)_INCLUDES) $(foreach m,$($(prefix)_STATIC_LIBRARIES),$($(m)_INCLUDES)))
endef

define obtain_cflags_internal
$(foreach prefix,$(1),$($(prefix)_CFLAGS) $(call obtain_includes, $(prefix)))
endef

define obtain_cflags
$(call obtain_cflags_internal, $(call obtain_prefix, $(1)))
endef

define obtain_generated
$(foreach prefix,$(patsubst clean_%,%,$@),$($(prefix)_TARGET_LIB)  $($(prefix)_TARGET_EXE) $($(prefix)_OBJ_FILES))
endef

define obtain_ldflgas
$(foreach ld,$(filter %.a,$(1)),-L$(dir $(ld)) -l$(patsubst lib%.a,%,$(notdir $(ld))))
endef

define obtain_all_objects
$(foreach prefix,$(1),$($(call obtain_prefix,$(prefix))_OBJ_FILES))
endef

ifeq ($(DEBUG),true)
$(info ======= $(LOCAL_MODULE) BEGIN =======)
$(info $(LOCAL_MODULE)_OBJ_FILES)
$(info --------------------------------------)
$(info $($(LOCAL_MODULE)_OBJ_FILES))
$(info ======================================)
$(info $(LOCAL_MODULE)_CFLAGS)
$(info --------------------------------------)
$(info $($(LOCAL_MODULE)_CFLAGS))
$(info ======================================)
$(info $(LOCAL_MODULE)_INCLUDES)
$(info --------------------------------------)
$(info $($(LOCAL_MODULE)_INCLUDES))
$(info ======================================)
$(info $(LOCAL_MODULE)_TARGET_LIB)
$(info --------------------------------------)
$(info $($(LOCAL_MODULE)_TARGET_LIB))
$(info ======================================)
$(info $(LOCAL_MODULE)_TARGET_EXE)
$(info --------------------------------------)
$(info $($(LOCAL_MODULE)_TARGET_EXE))
$(info ======================================)
$(info $(LOCAL_MODULE)_STATIC_LIBRARIES)
$(info --------------------------------------)
$(info $($(LOCAL_MODULE)_STATIC_LIBRARIES))
$(info ======= $(LOCAL_MODULE) END =======)
endif


ifeq ($(strip $(LOCAL_PACK_ALL)),true)
$($(LOCAL_MODULE)_TARGET_LIB): $($(LOCAL_MODULE)_DEPS)
	$(HIDE)echo "---> AR $@"
	$(HIDE)mkdir -p $(dir $@)
	$(HIDE)$(TOOLCHAIN)$(AR) -rc $@ $(filter %.o,$^) $(call obtain_all_objects, $(filter %.a,$^))
else
$($(LOCAL_MODULE)_TARGET_LIB): $($(LOCAL_MODULE)_OBJ_FILES)
	$(HIDE)echo "---> AR $@"
	$(HIDE)$(TOOLCHAIN)$(AR) -rc $@ $^
endif

$($(LOCAL_MODULE)_TARGET_EXE): $($(LOCAL_MODULE)_DEPS)
	$(HIDE)echo "===> LN $@"
	$(HIDE)$(TOOLCHAIN)$(CC) -o $@ $(filter %.o,$^) $(call obtain_ldflgas, $^) $($(call obtain_prefix, $@)_LDFLAGS) $(LDFLAGS)

$($(LOCAL_MODULE)_OBJ_FILES):$($(LOCAL_MODULE)_TARGET_DIR)/%.o:$(MODULE_PATH)/%
	$(HIDE)echo CC $@
	$(HIDE)mkdir -p $(dir $@)
	$(HIDE)$(TOOLCHAIN)$(CC) -o $@ -c $< $(call obtain_cflags, $@) $(CFLAGS)

clean_$(LOCAL_MODULE):
	$(HIDE)rm -f  $(call obtain_generated, $@)

ifneq ($(strip $($(LOCAL_MODULE)_TARGET_EXE)),)
run_$(LOCAL_MODULE): $($(LOCAL_MODULE)_TARGET_EXE)
	$(HIDE)$<
endif
