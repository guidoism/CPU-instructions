// Copyright 2016 Google Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//    http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

#include "cpu_instructions/x86/cleanup_instruction_set_alternatives.h"

#include <cstdint>
#include <unordered_map>
#include <vector>
#include "strings/string.h"

#include "cpu_instructions/base/cleanup_instruction_set.h"
#include "cpu_instructions/proto/instructions.pb.h"
#include "glog/logging.h"
#include "strings/str_cat.h"
#include "util/gtl/map_util.h"
#include "util/task/canonical_errors.h"
#include "util/task/status.h"

namespace cpu_instructions {
namespace x86 {
namespace {

using ::cpu_instructions::util::InvalidArgumentError;
using ::cpu_instructions::util::Status;

// Information about an operand that need to be modified when adding an
// alternative. There is one instance of this struct for each alternative.
struct OperandAlternative {
  // The new name of the operand.
  const char* operand_name;

  // The new addressing mode of the operand.
  InstructionOperand::AddressingMode addressing_mode;

  // The new value of the operand.
  uint32_t value_size;
};
using OperandAlternativeMap =
    std::unordered_map<string, std::vector<OperandAlternative>>;

// Returns the list of operand alternatives indexed by the name of the operand.
const OperandAlternativeMap& GetOperandAlternativesByName() {
  static const OperandAlternativeMap* const kAlternatives = []() {
    constexpr InstructionOperand::AddressingMode DIRECT_ADDRESSING =
        InstructionOperand::DIRECT_ADDRESSING;
    constexpr InstructionOperand::AddressingMode INDIRECT_ADDRESSING =
        InstructionOperand::INDIRECT_ADDRESSING;
    return new OperandAlternativeMap({
        {"mm/m32",
         {{"mm1", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"mm/m64",
         {{"mm1", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"mm2/m64",
         {{"mm2", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"r/m8",
         {{"r8", DIRECT_ADDRESSING, 8}, {"m8", INDIRECT_ADDRESSING, 8}}},
        {"r/m16",
         {{"r16", DIRECT_ADDRESSING, 16}, {"m16", INDIRECT_ADDRESSING, 16}}},
        {"r/m32",
         {{"r32", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"r/m64",
         {{"r64", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"r32/m8",
         {{"r32", DIRECT_ADDRESSING, 32}, {"m8", INDIRECT_ADDRESSING, 8}}},
        {"r32/m16",
         {{"r32", DIRECT_ADDRESSING, 32}, {"m16", INDIRECT_ADDRESSING, 16}}},
        {"r64/m16",
         {{"r64", DIRECT_ADDRESSING, 64}, {"m16", INDIRECT_ADDRESSING, 16}}},
        {"reg/m8",
         {{"r32", DIRECT_ADDRESSING, 32}, {"m8", INDIRECT_ADDRESSING, 8}}},
        {"reg/m16",
         {{"r32", DIRECT_ADDRESSING, 32}, {"m16", INDIRECT_ADDRESSING, 16}}},
        {"reg/m32",
         {{"r32", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"xmm2/m8",
         {{"xmm2", DIRECT_ADDRESSING, 8}, {"m8", INDIRECT_ADDRESSING, 8}}},
        {"xmm2/m16",
         {{"xmm2", DIRECT_ADDRESSING, 16}, {"m16", INDIRECT_ADDRESSING, 8}}},
        {"xmm/m32",
         {{"xmm2", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"xmm1/m32",
         {{"xmm1", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"xmm2/m32",
         {{"xmm2", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"xmm3/m32",
         {{"xmm3", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"xmm/m64",
         {{"xmm2", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"xmm1/m64",
         {{"xmm1", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"xmm1/m128",
         {{"xmm1", DIRECT_ADDRESSING, 128},
          {"m128", INDIRECT_ADDRESSING, 128}}},
        {"xmm2/m64",
         {{"xmm2", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"xmm3/m64",
         {{"xmm3", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
        {"xmm/m128",
         {{"xmm2", DIRECT_ADDRESSING, 128},
          {"m128", INDIRECT_ADDRESSING, 128}}},
        {"xmm2/m128",
         {{"xmm2", DIRECT_ADDRESSING, 128},
          {"m128", INDIRECT_ADDRESSING, 128}}},
        {"xmm3/m128",
         {{"xmm3", DIRECT_ADDRESSING, 128},
          {"m128", INDIRECT_ADDRESSING, 128}}},
        {"xmm2/m256",
         {{"xmm2", DIRECT_ADDRESSING, 256},
          {"m256", INDIRECT_ADDRESSING, 256}}},
        {"xmm3/m256",
         {{"xmm3", DIRECT_ADDRESSING, 256},
          {"m256", INDIRECT_ADDRESSING, 256}}},
        {"ymm2/m256",
         {{"ymm2", DIRECT_ADDRESSING, 256},
          {"m256", INDIRECT_ADDRESSING, 256}}},
        {"ymm3/m256",
         {{"ymm3", DIRECT_ADDRESSING, 256},
          {"m256", INDIRECT_ADDRESSING, 256}}},
        {"xmm3/m128/m32bcst",
         {
             {"xmm3", DIRECT_ADDRESSING, 128},
             {"m128", INDIRECT_ADDRESSING, 128},
             // TODO(user): Re-enable broadcasted arguments when we have a
             // way to represent them in the proto.
             // {"m64bcst", INDIRECT_ADDRESSING, 128},
         }},
        {"xmm3/m128/m64bcst",
         {
             {"xmm3", DIRECT_ADDRESSING, 128},
             {"m128", INDIRECT_ADDRESSING, 128},
             // TODO(user): Re-enable broadcasted arguments when we have a
             // way to represent them in the proto.
             // {"m64bcst", INDIRECT_ADDRESSING, 128},
         }},
        {"bnd1/m128",
         {{"bnd1", DIRECT_ADDRESSING, 128},
          {"m128", INDIRECT_ADDRESSING, 128}}},
        {"bnd2/m128",
         {{"bnd2", DIRECT_ADDRESSING, 128},
          {"m128", INDIRECT_ADDRESSING, 128}}},
        {"k2/m8",
         {{"k2", DIRECT_ADDRESSING, 8}, {"m8", INDIRECT_ADDRESSING, 8}}},
        {"k2/m16",
         {{"k2", DIRECT_ADDRESSING, 16}, {"m16", INDIRECT_ADDRESSING, 16}}},
        {"k2/m32",
         {{"k2", DIRECT_ADDRESSING, 32}, {"m32", INDIRECT_ADDRESSING, 32}}},
        {"k2/m64",
         {{"k2", DIRECT_ADDRESSING, 64}, {"m64", INDIRECT_ADDRESSING, 64}}},
    });
  }();
  return *kAlternatives;
}

}  // namespace

Status AddAlternatives(InstructionSetProto* instruction_set) {
  CHECK(instruction_set != nullptr);
  Status status = Status::OK;
  const OperandAlternativeMap& alternatives_by_name =
      GetOperandAlternativesByName();
  std::vector<InstructionProto> new_instructions;
  for (InstructionProto& instruction :
       *instruction_set->mutable_instructions()) {
    InstructionFormat* const vendor_syntax =
        instruction.mutable_vendor_syntax();
    for (int operand_index = 0; operand_index < vendor_syntax->operands_size();
         ++operand_index) {
      InstructionOperand* const operand =
          vendor_syntax->mutable_operands(operand_index);
      const std::vector<OperandAlternative>* const alternatives =
          FindOrNull(alternatives_by_name, operand->name());
      if (alternatives == nullptr) continue;

      // The only encoding that allows alternatives is modrm.rm. An operand with
      // alternatives anywhere else means that there is an error in the data.
      if (operand->encoding() != InstructionOperand::MODRM_RM_ENCODING) {
        return InvalidArgumentError(
            StrCat("Instruction does not use modrm.rm encoding:\n",
                   instruction.DebugString()));
      }
      // The altenatives are always "register" vs "memory", because that is the
      // only kind of alternatives that can be expressed through operand
      // encoding.
      if (operand->addressing_mode() !=
          InstructionOperand::ANY_ADDRESSING_WITH_FLEXIBLE_REGISTERS) {
        return InvalidArgumentError(StrCat(
            "The addressing mode does not allow splitting: ",
            InstructionOperand::AddressingMode_Name(operand->addressing_mode()),
            "\n", instruction.DebugString()));
      }
      // Note that we start iterating at 1, and that we will be reusing
      // the existing instruction for the first alternative.
      for (int i = 1; i < alternatives->size(); ++i) {
        const OperandAlternative& alternative = (*alternatives)[i];
        new_instructions.push_back(instruction);
        InstructionOperand* const new_instruction_operand =
            new_instructions.back().mutable_vendor_syntax()->mutable_operands(
                operand_index);
        new_instruction_operand->set_name(alternative.operand_name);
        new_instruction_operand->set_addressing_mode(
            alternative.addressing_mode);
        new_instruction_operand->set_value_size_bits(alternative.value_size);
      }
      // Now overwrite the current instruction's operand.
      const OperandAlternative& first_alternative = alternatives->front();
      operand->set_name(first_alternative.operand_name);
      operand->set_addressing_mode(first_alternative.addressing_mode);
      operand->set_value_size_bits(first_alternative.value_size);
    }
  }
  for (InstructionProto& new_instruction : new_instructions) {
    instruction_set->add_instructions()->Swap(&new_instruction);
  }
  return status;
}
REGISTER_INSTRUCTION_SET_TRANSFORM(AddAlternatives, 6000);

}  // namespace x86
}  // namespace cpu_instructions
