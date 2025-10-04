/* Copyright (C) 2023-2024 Michal Kosciesza <michal@mkiol.net>
 *
 * This Source Code Form is subject to the terms of the Mozilla Public
 * License, v. 2.0. If a copy of the MPL was not distributed with this
 * file, You can obtain one at http://mozilla.org/MPL/2.0/.
 */

#include "cpu_tools.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>

TEST_CASE("cpu_tools", "[parse_cpuinfo]") {
    SECTION("parse_amd") {
        std::string cpuinfo_data = R"(processor	: 0
vendor_id	: AuthenticAMD
cpu family	: 23
model		: 113
model name	: AMD Ryzen
stepping	: 0
flags		: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx mmxext fxsr_opt pdpe1gb rdtscp lm constant_tsc rep_good nopl nonstop_tsc cpuid extd_apicid aperfmperf rapl pni pclmulqdq monitor ssse3 fma cx16 sse4_1 sse4_2 movbe popcnt aes xsave avx f16c rdrand lahf_lm cmp_legacy svm extapic cr8_legacy abm sse4a misalignsse 3dnowprefetch osvw ibs skinit wdt tce topoext perfctr_core perfctr_nb bpext perfctr_llc mwaitx cpb cat_l3 cdp_l3 hw_pstate ssbd mba ibpb stibp vmmcall fsgsbase bmi1 avx2 smep bmi2 cqm rdt_a rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local clzero irperf xsaveerptr rdpru wbnoinvd arat npt lbrv svm_lock nrip_save tsc_scale vmcb_clean flushbyasid decodeassists pausefilter pfthreshold avic v_vmsave_vmload vgif v_spec_ctrl umip rdpid overflow_recov succor smca sev sev_es
bogomips	: 7189.52

processor	: 1
vendor_id	: AuthenticAMD
cpu family	: 23
model		: 113
model name	: AMD Ryzen
stepping	: 0
flags		: fpu vme de pse tsc msr pae mce cx8 apic sep mtrr pge mca cmov pat pse36 clflush mmx fxsr sse sse2 ht syscall nx mmxext fxsr_opt pdpe1gb rdtscp lm constant_tsc rep_good nopl nonstop_tsc cpuid extd_apicid aperfmperf rapl pni pclmulqdq monitor ssse3 fma cx16 sse4_1 sse4_2 movbe popcnt aes xsave avx f16c rdrand lahf_lm cmp_legacy svm extapic cr8_legacy abm sse4a misalignsse 3dnowprefetch osvw ibs skinit wdt tce topoext perfctr_core perfctr_nb bpext perfctr_llc mwaitx cpb cat_l3 cdp_l3 hw_pstate ssbd mba ibpb stibp vmmcall fsgsbase bmi1 avx2 smep bmi2 cqm rdt_a rdseed adx smap clflushopt clwb sha_ni xsaveopt xsavec xgetbv1 cqm_llc cqm_occup_llc cqm_mbm_total cqm_mbm_local clzero irperf xsaveerptr rdpru wbnoinvd arat npt lbrv svm_lock nrip_save tsc_scale vmcb_clean flushbyasid decodeassists pausefilter pfthreshold avic v_vmsave_vmload vgif v_spec_ctrl umip rdpid overflow_recov succor smca sev sev_es
bogomips	: 7189.52)";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u, cpu_tools::feature_flags_t::avx |
                    cpu_tools::feature_flags_t::avx2 |
                    cpu_tools::feature_flags_t::fma |
                    cpu_tools::feature_flags_t::f16c |
                    cpu_tools::feature_flags_t::sse4_1 |
                    cpu_tools::feature_flags_t::bmi2};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }

    SECTION("parse_pinebook") {
        std::string cpuinfo_data = R"(processor	: 0
BogoMIPS	: 48.00
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid
CPU implementer	: 0x41
CPU architecture: 8
CPU variant	: 0x0
CPU part	: 0xd03
CPU revision	: 4

processor	: 1
BogoMIPS	: 48.00
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid
CPU implementer	: 0x41
CPU architecture: 8
CPU variant	: 0x0
CPU part	: 0xd03
CPU revision	: 4)";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u, cpu_tools::feature_flags_t::asimd};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }

    SECTION("parse_xperia10") {
        std::string cpuinfo_data =
            R"(Processor	: AArch64 Processor rev 4 (aarch64)
processor	: 0
BogoMIPS	: 38.40
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid
CPU implementer	: 0x51
CPU architecture: 8
CPU variant	: 0xa
CPU part	: 0x801
CPU revision	: 4

processor	: 1
BogoMIPS	: 38.40
Features	: fp asimd evtstrm aes pmull sha1 sha2 crc32 cpuid
CPU implementer	: 0x51
CPU architecture: 8
CPU variant	: 0xa
CPU part	: 0x801
CPU revision	: 4)";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u, cpu_tools::feature_flags_t::asimd};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }

    SECTION("parse_xperia13") {
        std::string cpuinfo_data =
            R"(Processor       : AArch64 Processor rev 14 (aarch64)
processor       : 0
BogoMIPS        : 38.40
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm lrcpc dcpop asimddp
CPU implementer : 0x51
CPU architecture: 8
CPU variant     : 0xd
CPU part        : 0x805
CPU revision    : 14

processor       : 1
BogoMIPS        : 38.40
Features        : fp asimd evtstrm aes pmull sha1 sha2 crc32 atomics fphp asimdhp cpuid asimdrdm lrcpc dcpop asimddp
CPU implementer : 0x51
CPU architecture: 8
CPU variant     : 0xd
CPU part        : 0x805
CPU revision    : 14
)";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u, cpu_tools::feature_flags_t::asimd};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }

    SECTION("parse_jollac") {
        std::string cpuinfo_data =
            R"(processor	: 0
model name	: ARMv7 Processor rev 5 (v7l)
BogoMIPS	: 38.40
Features	: swp half thumb fastmult vfp edsp neon vfpv3 tls vfpv4 idiva idivt vfpd32 evtstrm 
CPU implementer	: 0x41
CPU architecture: 7
CPU variant	: 0x0
CPU part	: 0xc07
CPU revision	: 5

processor	: 1
model name	: ARMv7 Processor rev 5 (v7l)
BogoMIPS	: 38.40
Features	: swp half thumb fastmult vfp edsp neon vfpv3 tls vfpv4 idiva idivt vfpd32 evtstrm 
CPU implementer	: 0x41
CPU architecture: 7
CPU variant	: 0x0
CPU part	: 0xc07
CPU revision	: 5)";
        std::istringstream is{cpuinfo_data};

        auto cpuinfo = cpu_tools::parse_cpuinfo(is);
        cpu_tools::cpuinfo_t expected_cpuinfo = {
            2u, cpu_tools::feature_flags_t::none};

        REQUIRE(cpuinfo == expected_cpuinfo);
    }
}
