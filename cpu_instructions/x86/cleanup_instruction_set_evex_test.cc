#include "cpu_instructions/x86/cleanup_instruction_set_evex.h"

#include "cpu_instructions/base/cleanup_instruction_set_test_utils.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"

namespace cpu_instructions {
namespace x86 {
namespace {

TEST(AddEvexBInterpretationTest, LegacyAndVexEncoding) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        description: "Add with carry r/m8 to byte register."
        vendor_syntax {
          mnemonic: "ADC"
          operands { addressing_mode: DIRECT_ADDRESSING
                     encoding: MODRM_REG_ENCODING
                     value_size_bits: 8
                     name: "r8" }
          operands { addressing_mode: ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS
                     encoding: MODRM_RM_ENCODING
                     value_size_bits: 8
                     name: "r/m8" }}
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "RM"
        raw_encoding_specification: "12 /r"
        x86_encoding_specification {
          legacy_prefixes {} opcode: 0x12 modrm_usage: FULL_MODRM }}
      instructions {
        vendor_syntax { mnemonic: 'VFMSUB231PS' operands { name: 'xmm0' }
                        operands { name: 'xmm1' } operands { name: 'm128' }}
        feature_name: 'FMA' encoding_scheme: 'A'
        raw_encoding_specification: 'VEX.DDS.128.66.0F38.W0 BA /r'
        x86_encoding_specification {
          opcode: 997562
          modrm_usage: FULL_MODRM
          vex_prefix {
            prefix_type: VEX_PREFIX
            vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
            vector_size: VEX_VECTOR_SIZE_128_BIT
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F38
            vex_w_usage: VEX_W_IS_ZERO }}})";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, Broadcast) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VADDPD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "xmm1" tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "xmm2" }
          operands { encoding: MODRM_RM_ENCODING name: "xmm3/m128/m64bcst"
                     usage: USAGE_READ }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "FV"
        raw_encoding_specification: "EVEX.NDS.128.66.0F.W1 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_128_BIT
            vex_w_usage: VEX_W_IS_ONE }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  constexpr char kExpectedInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VADDPD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "xmm1" tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "xmm2" }
          operands { encoding: MODRM_RM_ENCODING name: "xmm3/m128/m64bcst"
                     usage: USAGE_READ }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "FV"
        raw_encoding_specification: "EVEX.NDS.128.66.0F.W1 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_128_BIT
            vex_w_usage: VEX_W_IS_ONE
            evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, RoundingControl) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VADDSS"
          operands { encoding: MODRM_REG_ENCODING name: "xmm1"
                     tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "xmm2" }
          operands { encoding: MODRM_RM_ENCODING name: "xmm3/m32"
                     tags { name: "er" } usage: USAGE_READ }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1S"
        raw_encoding_specification: "EVEX.NDS.LIG.F3.0F.W0 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_REPE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_IS_IGNORED
            vex_w_usage: VEX_W_IS_ZERO }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58
        }})";
  constexpr char kExpectedInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VADDSS"
          operands { encoding: MODRM_REG_ENCODING name: "xmm1"
                     tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "xmm2" }
          operands { encoding: MODRM_RM_ENCODING name: "xmm3/m32"
                     tags { name: "er" } usage: USAGE_READ }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1S"
        raw_encoding_specification: "EVEX.NDS.LIG.F3.0F.W0 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_REPE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_IS_IGNORED
            vex_w_usage: VEX_W_IS_ZERO
            evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, SuppressAllExceptions) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VCMPSD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "k1" tags { name: "k2" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "xmm2" }
          operands { encoding: MODRM_RM_ENCODING
                     name: "xmm3/m64" tags { name: "sae" }
                     usage: USAGE_READ }
          operands { encoding: IMMEDIATE_VALUE_ENCODING
                     name: "imm8" usage: USAGE_READ }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1S"
        raw_encoding_specification: "EVEX.NDS.LIG.F2.0F.W1 C2 /r ib"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_REPNE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_IS_IGNORED
            vex_w_usage: VEX_W_IS_ONE }
          modrm_usage: FULL_MODRM
          opcode: 0x0fc2
          immediate_value_bytes: 1 }})";
  constexpr char kExpectedInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VCMPSD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "k1" tags { name: "k2" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "xmm2" }
          operands { encoding: MODRM_RM_ENCODING
                     name: "xmm3/m64" tags { name: "sae" }
                     usage: USAGE_READ }
          operands { encoding: IMMEDIATE_VALUE_ENCODING
                     name: "imm8" usage: USAGE_READ }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1S"
        raw_encoding_specification: "EVEX.NDS.LIG.F2.0F.W1 C2 /r ib"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_REPNE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_IS_IGNORED
            vex_w_usage: VEX_W_IS_ONE
            evex_b_interpretations: EVEX_B_ENABLES_SUPPRESS_ALL_EXCEPTIONS }
          modrm_usage: FULL_MODRM
          opcode: 0x0fc2
          immediate_value_bytes: 1 }})";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexBInterpretationTest, Combined) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VADDPD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "zmm1" tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "zmm2" }
          operands { encoding: MODRM_RM_ENCODING usage: USAGE_READ
                     name: "zmm3/m512/m64bcst" tags { name: "er" }}}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "FV"
        raw_encoding_specification: "EVEX.NDS.512.66.0F.W1 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_512_BIT
            vex_w_usage: VEX_W_IS_ONE }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  constexpr char kExpectedInstructionSetProto[] = R"(
      instructions {
        vendor_syntax {
          mnemonic: "VADDPD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "zmm1" tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "zmm2" }
          operands { encoding: MODRM_RM_ENCODING usage: USAGE_READ
                     name: "zmm3/m512/m64bcst" tags { name: "er" }}}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "FV"
        raw_encoding_specification: "EVEX.NDS.512.66.0F.W1 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_512_BIT
            vex_w_usage: VEX_W_IS_ONE
            evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
            evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  TestTransform(AddEvexBInterpretation, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

TEST(AddEvexOpmaskUsageTest, Combined) {
  constexpr char kInstructionSetProto[] = R"(
      instructions {
        llvm_mnemonic: "VCVTSD2SIrr"
        vendor_syntax {
          mnemonic: "VCVTSD2SI"
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 32
                     encoding: MODRM_REG_ENCODING usage: USAGE_WRITE
                     name: "r32" }
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 64
                     encoding: MODRM_RM_ENCODING usage: USAGE_READ
                     name: "xmm1" tags { name: "er" }}}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1F"
        binary_encoding_size_bytes: 4
        raw_encoding_specification: "EVEX.LIG.F2.0F.W0 2D /r"
        x86_encoding_specification {
          opcode: 0x0f2d
          modrm_usage: FULL_MODRM
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_REPNE
            map_select: MAP_SELECT_0F
            vex_w_usage: VEX_W_IS_ZERO
            evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL }}}
      instructions {
        llvm_mnemonic: "VGATHERDPDYrm"
        vendor_syntax {
          mnemonic: "VGATHERDPD"
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 256
                     encoding: MODRM_REG_ENCODING usage: USAGE_READ_WRITE
                     name: "ymm1" }
          operands { addressing_mode: INDIRECT_ADDRESSING usage: USAGE_READ
                     encoding: VSIB_ENCODING name: "vm32x" }
          operands { addressing_mode: DIRECT_ADDRESSING encoding: VEX_V_ENCODING
                     value_size_bits: 256 usage: USAGE_READ_WRITE
                     name: "ymm2" }}
        feature_name: "AVX2"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "RMV"
        binary_encoding_size_bytes: 6
        raw_encoding_specification: "VEX.DDS.256.66.0F38.W1 92 /r /vsib"
        x86_encoding_specification {
          opcode: 0x0f38
          modrm_usage: FULL_MODRM
          vex_prefix { prefix_type: VEX_PREFIX
                       vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
                       vector_size: VEX_VECTOR_SIZE_256_BIT
                       mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                       map_select: MAP_SELECT_0F38
                       vex_w_usage: VEX_W_IS_ONE vsib_usage: VSIB_USED }}}
      instructions {
        vendor_syntax {
          mnemonic: "VGATHERDPD"
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 128
                     encoding: MODRM_REG_ENCODING usage: USAGE_WRITE
                     name: "xmm1" tags { name: "k1" }}
          operands { addressing_mode: INDIRECT_ADDRESSING usage: USAGE_READ
                     encoding: VSIB_ENCODING name: "vm32x" }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1S"
        raw_encoding_specification: "EVEX.128.66.0F38.W1 92 /vsib"
        x86_encoding_specification {
          opcode: 997522
          modrm_usage: FULL_MODRM
          vex_prefix { prefix_type: EVEX_PREFIX
                       vector_size: VEX_VECTOR_SIZE_128_BIT
                       mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                       map_select: MAP_SELECT_0F38
                       vex_w_usage: VEX_W_IS_ONE
                       vsib_usage: VSIB_USED }}}
      instructions {
        vendor_syntax {
          mnemonic: "VADDPD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "zmm1" tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "zmm2" }
          operands { encoding: MODRM_RM_ENCODING usage: USAGE_READ
                     name: "zmm3/m512/m64bcst" tags { name: "er" }}}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "FV"
        raw_encoding_specification: "EVEX.NDS.512.66.0F.W1 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_512_BIT
            vex_w_usage: VEX_W_IS_ONE
            evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
            evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  constexpr char kExpectedInstructionSetProto[] = R"(
      instructions {
        llvm_mnemonic: "VCVTSD2SIrr"
        vendor_syntax {
          mnemonic: "VCVTSD2SI"
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 32
                     encoding: MODRM_REG_ENCODING usage: USAGE_WRITE
                     name: "r32" }
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 64
                     encoding: MODRM_RM_ENCODING usage: USAGE_READ
                     name: "xmm1" tags { name: "er" }}}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1F"
        binary_encoding_size_bytes: 4
        raw_encoding_specification: "EVEX.LIG.F2.0F.W0 2D /r"
        x86_encoding_specification {
          opcode: 0x0f2d
          modrm_usage: FULL_MODRM
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_REPNE
            map_select: MAP_SELECT_0F
            vex_w_usage: VEX_W_IS_ZERO
            evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL }}}
      instructions {
        llvm_mnemonic: "VGATHERDPDYrm"
        vendor_syntax {
          mnemonic: "VGATHERDPD"
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 256
                     encoding: MODRM_REG_ENCODING usage: USAGE_READ_WRITE
                     name: "ymm1" }
          operands { addressing_mode: INDIRECT_ADDRESSING usage: USAGE_READ
                     encoding: VSIB_ENCODING name: "vm32x" }
          operands { addressing_mode: DIRECT_ADDRESSING encoding: VEX_V_ENCODING
                     value_size_bits: 256 usage: USAGE_READ_WRITE
                     name: "ymm2" }}
        feature_name: "AVX2"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "RMV"
        binary_encoding_size_bytes: 6
        raw_encoding_specification: "VEX.DDS.256.66.0F38.W1 92 /r /vsib"
        x86_encoding_specification {
          opcode: 0x0f38
          modrm_usage: FULL_MODRM
          vex_prefix { prefix_type: VEX_PREFIX
                       vex_operand_usage: VEX_OPERAND_IS_SECOND_SOURCE_REGISTER
                       vector_size: VEX_VECTOR_SIZE_256_BIT
                       mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                       map_select: MAP_SELECT_0F38
                       vex_w_usage: VEX_W_IS_ONE vsib_usage: VSIB_USED }}}
      instructions {
        vendor_syntax {
          mnemonic: "VGATHERDPD"
          operands { addressing_mode: DIRECT_ADDRESSING value_size_bits: 128
                     encoding: MODRM_REG_ENCODING usage: USAGE_WRITE
                     name: "xmm1" tags { name: "k1" }}
          operands { addressing_mode: INDIRECT_ADDRESSING usage: USAGE_READ
                     encoding: VSIB_ENCODING name: "vm32x" }}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "T1S"
        raw_encoding_specification: "EVEX.128.66.0F38.W1 92 /vsib"
        x86_encoding_specification {
          opcode: 997522
          modrm_usage: FULL_MODRM
          vex_prefix { prefix_type: EVEX_PREFIX
                       vector_size: VEX_VECTOR_SIZE_128_BIT
                       mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
                       map_select: MAP_SELECT_0F38
                       vex_w_usage: VEX_W_IS_ONE
                       vsib_usage: VSIB_USED
                       opmask_usage: EVEX_OPMASK_IS_REQUIRED
                       masking_operation: EVEX_MASKING_MERGING_ONLY }}}
      instructions {
        vendor_syntax {
          mnemonic: "VADDPD"
          operands { encoding: MODRM_REG_ENCODING
                     name: "zmm1" tags { name: "k1" } tags { name: "z" }
                     usage: USAGE_WRITE }
          operands { encoding: VEX_V_ENCODING name: "zmm2" }
          operands { encoding: MODRM_RM_ENCODING usage: USAGE_READ
                     name: "zmm3/m512/m64bcst" tags { name: "er" }}}
        feature_name: "AVX512F"
        available_in_64_bit: true
        legacy_instruction: true
        encoding_scheme: "FV"
        raw_encoding_specification: "EVEX.NDS.512.66.0F.W1 58 /r"
        x86_encoding_specification {
          vex_prefix {
            prefix_type: EVEX_PREFIX
            mandatory_prefix: MANDATORY_PREFIX_OPERAND_SIZE_OVERRIDE
            map_select: MAP_SELECT_0F
            vector_size: VEX_VECTOR_SIZE_512_BIT
            vex_w_usage: VEX_W_IS_ONE
            evex_b_interpretations: EVEX_B_ENABLES_64_BIT_BROADCAST
            evex_b_interpretations: EVEX_B_ENABLES_STATIC_ROUNDING_CONTROL
            opmask_usage: EVEX_OPMASK_IS_OPTIONAL
            masking_operation: EVEX_MASKING_MERGING_AND_ZEROING }
          modrm_usage: FULL_MODRM
          opcode: 0x0f58 }})";
  TestTransform(AddEvexOpmaskUsage, kInstructionSetProto,
                kExpectedInstructionSetProto);
}

}  // namespace
}  // namespace x86
}  // namespace cpu_instructions
