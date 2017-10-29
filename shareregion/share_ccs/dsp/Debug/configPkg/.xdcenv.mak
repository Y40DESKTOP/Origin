#
_XDCBUILDCOUNT = 
ifneq (,$(findstring path,$(_USEXDCENV_)))
override XDCPATH = Z:/OMAPL138_StarterWare_1_10_04_01/include/hw;Z:/OMAPL138_StarterWare_1_10_04_01/include;Z:/ti/syslink_2_21_01_05/packages;Z:/ti/ipc_1_25_03_15/packages;Z:/share/project/shareR/shared;Z:/ti/ccsv5/ccs_base;C:/ti/bios_6_35_04_50/packages
override XDCROOT = C:/ti/xdctools_3_25_03_72
override XDCBUILDCFG = ./config.bld
endif
ifneq (,$(findstring args,$(_USEXDCENV_)))
override XDCARGS = 
override XDCTARGETS = 
endif
#
ifeq (0,1)
PKGPATH = Z:/OMAPL138_StarterWare_1_10_04_01/include/hw;Z:/OMAPL138_StarterWare_1_10_04_01/include;Z:/ti/syslink_2_21_01_05/packages;Z:/ti/ipc_1_25_03_15/packages;Z:/share/project/shareR/shared;Z:/ti/ccsv5/ccs_base;C:/ti/bios_6_35_04_50/packages;C:/ti/xdctools_3_25_03_72/packages;..
HOSTOS = Windows
endif
