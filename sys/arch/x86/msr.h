#ifndef X86_MSR_H
#define X86_MSR_H

/*
 * Intel® 64 and IA-32 Architectures Software Developer’s Manual 
 * Volume 4: Model-Specific Registers
 */
enum x86_msr
{
	IA32_P5_MC_ADDR              = 0x0,
	IA32_P5_MC_TYPE              = 0x1,
	IA32_MONITOR_FILTER_SIZE     = 0x6,
	IA32_TIME_STAMP_COUNTER      = 0x10,
	IA32_PLATFORM_ID             = 0x17,
	IA32_APIC_BASE               = 0x1B,
	IA32_FEATURE_CONTROL         = 0x3A,
	IA32_TSC_ADJUST              = 0x3B,
	IA32_SPEC_CTRL               = 0x48,
	IA32_PRED_CMD                = 0x49,
	IA32_BIOS_UPDT_TRIG          = 0x79,
	IA32_BIOS_SIGN_ID            = 0x8B,
	IA32_SGXLEPUBKEYHASH0        = 0x8C,
	IA32_SGXLEPUBKEYHASH1        = 0x8D,
	IA32_SGXLEPUBKEYHASH2        = 0x8E,
	IA32_SGXLEPUBKEYHASH3        = 0x8F,
	IA32_SMM_MONITOR_CTL         = 0x9B,
	IA32_SMBASE                  = 0x9E,
	IA32_PMC0                    = 0xC1,
	IA32_PMC1                    = 0xC2,
	IA32_PMC2                    = 0xC3,
	IA32_PMC3                    = 0xC4,
	IA32_PMC4                    = 0xC5,
	IA32_PMC5                    = 0xC6,
	IA32_PMC6                    = 0xC7,
	IA32_PMC7                    = 0xC8,
	IA32_CORE_CAPABILITIES       = 0xCF,
	IA32_UMWAIT_CONTROL          = 0xE1,
	IA32_MPERF                   = 0xE7,
	IA32_APERF                   = 0xE8,
	IA32_MTRRCAP                 = 0xFE,
	IA32_ARCH_CAPABILITIES       = 0x10A,
	IA32_FLUSH_CMD               = 0x10B,
	IA32_SYSENTER_CS             = 0x174,
	IA32_SYSENTER_ESP            = 0x175,
	IA32_SYSENTER_EIP            = 0x176,
	IA32_MCG_CAP                 = 0x179,
	IA32_MCG_STATUS              = 0x17A,
	IA32_MCG_CTL                 = 0x17B,
	IA32_PERFEVTSEL0             = 0x186,
	IA32_PERFEVTSEL1             = 0x187,
	IA32_PERFEVTSEL2             = 0x188,
	IA32_PERFEVTSEL3             = 0x189,
	IA32_PERF_STATUS             = 0x198,
	IA32_PERF_CTL                = 0x199,
	IA32_CLOCK_MODULATION        = 0x19A,
	IA32_THERM_INTERRUPT         = 0x19B,
	IA32_THERM_STATUS            = 0x19C,
	IA32_MISC_ENABLE             = 0x1A0,
	IA32_ENERGY_PERF_BIAS        = 0x1B0,
	IA32_PACKAGE_THERM_STATUS    = 0x1B1,
	IA32_PACKAGE_THERM_INTERRUPT = 0x1B2,
	IA32_DEBUGCTL                = 0x1D9,
	IA32_SMRR_PHYSBASE           = 0x1F2,
	IA32_SMRR_PHYSMASK           = 0x1F3,
	IA32_PLATFORM_DCA_CAP        = 0x1F8,
	IA32_CPU_DCA_CAP             = 0x1F9,
	IA32_DCA_0_CAP               = 0x1FA,
	IA32_MTRR_PHYSBASE0          = 0x200,
	IA32_MTRR_PHYSMASK0          = 0x201,
	IA32_MTRR_PHYSBASE1          = 0x202,
	IA32_MTRR_PHYSMASK1          = 0x203,
	IA32_MTRR_PHYSBASE2          = 0x204,
	IA32_MTRR_PHYSMASK2          = 0x205,
	IA32_MTRR_PHYSBASE3          = 0x206,
	IA32_MTRR_PHYSMASK3          = 0x207,
	IA32_MTRR_PHYSBASE4          = 0x208,
	IA32_MTRR_PHYSMASK4          = 0x209,
	IA32_MTRR_PHYSBASE5          = 0x20A,
	IA32_MTRR_PHYSMASK5          = 0x20B,
	IA32_MTRR_PHYSBASE6          = 0x20C,
	IA32_MTRR_PHYSMASK6          = 0x20D,
	IA32_MTRR_PHYSBASE7          = 0x20E,
	IA32_MTRR_PHYSMASK7          = 0x20F,
	IA32_MTRR_PHYSBASE8          = 0x210,
	IA32_MTRR_PHYSMASK8          = 0x211,
	IA32_MTRR_PHYSBASE9          = 0x212,
	IA32_MTRR_PHYSMASK9          = 0x213,
	/* XXX */
};

#endif