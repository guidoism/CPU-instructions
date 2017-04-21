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

#include "cpu_instructions/util/pdf/pdf_document_utils.h"

#include <algorithm>
#include <numeric>

#include "cpu_instructions/testing/test_util.h"
#include "cpu_instructions/util/proto_util.h"
#include "glog/logging.h"
#include "gmock/gmock.h"
#include "gtest/gtest.h"
#include "src/google/protobuf/text_format.h"

namespace cpu_instructions {
namespace pdf {
namespace {

using cpu_instructions::testing::EqualsProto;

PdfPage GetFakeDocument() {
  return ParseProtoFromStringOrDie<PdfPage>(R"(
    rows {
      blocks {
        text: "0, 0"
      }
      blocks {
        text: "0, 1"
      }
    }
    rows {
      blocks {
        text: "1, 0"
      }
      blocks {
        text: "1, 1"
      }
    }
  )");
}

TEST(PdfDocumentExtractorTest, GetCellTextOrEmpty) {
  // Access regular cells
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 0, 0), "0, 0");
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 0, 1), "0, 1");
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 1, 0), "1, 0");
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 1, 1), "1, 1");

  // -1 for col or row means last one.
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 0, -1), "0, 1");
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), -1, 0), "1, 0");
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), -1, -1), "1, 1");

  // -2 would be the one before last, aka 0 in our case.
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), -2, -2), "0, 0");

  // Access inexistent cell
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 0, 5), "");
  EXPECT_EQ(GetCellTextOrEmpty(GetFakeDocument(), 5, 0), "");
}

TEST(PdfDocumentExtractorTest, MutateCellOrNull) {
  PdfPage page = GetFakeDocument();
  // Access regular cells
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, 0, 0)), "0, 0");
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, 0, 1)), "0, 1");
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, 1, 0)), "1, 0");
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, 1, 1)), "1, 1");

  // -1 for col or row means last one.
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, 0, -1)), "0, 1");
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, -1, 0)), "1, 0");
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, -1, -1)), "1, 1");

  // -2 would be the one before last, aka 0 in our case.
  EXPECT_EQ(*CHECK_NOTNULL(GetMutableCellTextOrNull(&page, -2, -2)), "0, 0");

  // Access inexistent cell
  EXPECT_EQ(GetMutableCellTextOrNull(&page, 0, 5), nullptr);
  EXPECT_EQ(GetMutableCellTextOrNull(&page, 5, 0), nullptr);
}

TEST(PdfDocumentExtractorTest, PatchDocument) {
  PdfPage page = ParseProtoFromStringOrDie<PdfPage>(R"(
    number: 5
    rows {
      blocks {
        text: "0, 0"
      }
      blocks {
        text: "0, 1"
      }
    }
    rows {
      blocks {
        text: "1, 0"
      }
      blocks {
        text: "1, 1"
      }
    }
  )");

  const PdfPagePatch patch = ParseProtoFromStringOrDie<PdfPagePatch>(R"(
    row: 0 col: 1
    expected: "0, 1"
    replacement: "will be replaced"
  )");
  ApplyPatchOrDie(patch, &page);
  EXPECT_EQ(GetCellTextOrEmpty(page, 0, 0), "0, 0");
  EXPECT_EQ(GetCellTextOrEmpty(page, 0, 1), "will be replaced");
  EXPECT_EQ(GetCellTextOrEmpty(page, 1, 0), "1, 0");
  EXPECT_EQ(GetCellTextOrEmpty(page, 1, 1), "1, 1");
}

TEST(PdfDocumentExtractorTest, GetPageBodyRows) {
  const PdfPage page = ParseProtoFromStringOrDie<PdfPage>(R"(
    width: 100
    height: 30
    rows { bounding_box { top: 1.0 bottom: 9.0 } }  # in header
    rows { bounding_box { top: 1.0 bottom: 11.0 } }  # across header boundary
    rows { bounding_box { top: 11.0 bottom: 19.0 } }  # in body
    rows { bounding_box { top: 11.0 bottom: 21.0 } }  # across footer boundary
    rows { bounding_box { top: 21.0 bottom: 29.0 } }  # in footer
    rows { bounding_box { top: 1.0 bottom: 29.0 } }  # across header and footer
  )");
  const auto result = GetPageBodyRows(page, 10.0f);
  EXPECT_EQ(result.size(), 1);
}

TEST(PdfDocumentExtractorTest, TransferPatches) {
  const auto from_pdf = ParseProtoFromStringOrDie<PdfDocument>(R"(
    document_id { title: "doc 1" }
    pages {
      number: 5
      width: 100
      height: 30
      rows {
        blocks {
          row: 0
          col: 0
          text: "incorrect"
        }
        bounding_box { top: 11.0 bottom: 12.0 } # in body
      }
      rows {
        blocks {
          row: 1
          col: 0
          text: "to replace"
        }
        bounding_box { top: 12.0 bottom: 13.0 } # in body
      }
    }
  )");

  const auto patches = ParseProtoFromStringOrDie<PdfDocumentChanges>(R"(
    document_id { title: "doc 1" }
    pages {
      page_number: 5
      patches {
        row: 0
        col: 0
        expected: "incorrect"
        replacement: "correct"
      }
      patches {
        row: 1
        col: 0
        expected: "to replace"
        replacement: "replaced"
      }
    }
  )");

  const auto to_pdf = ParseProtoFromStringOrDie<PdfDocument>(R"(
    document_id { title: "doc 2" }
    pages {
      number: 6
      width: 100
      height: 30
      rows {
        blocks {
          row: 0
          col: 0
          text: "incorrect"
        }
        bounding_box { top: 11.0 bottom: 12.0 } # in body
      }
      rows {
        blocks {
          row: 1
          col: 0
          text: "to replace with typo"
        }
        bounding_box { top: 12.0 bottom: 13.0 } # in body
      }
    }
  )");

  PdfDocumentChanges successful_patches;
  PdfDocumentChanges failed_patches;
  TransferPatches(patches, from_pdf, to_pdf, &successful_patches,
                  &failed_patches);

  constexpr char kExpectedSuccessful[] = R"(
      document_id { title: "doc 2" }
      pages {
        page_number: 6
        patches {
          row: 0
          col: 0
          expected: "incorrect"
          replacement: "correct"
        }
      }
    )";
  EXPECT_THAT(successful_patches, EqualsProto(kExpectedSuccessful));

  constexpr char kExpectedFailed[] = R"(
      document_id { title: "doc 1" }
      pages {
        page_number: 5
        patches {
          row: 1
          col: 0
          expected: "to replace"
          replacement: "replaced"
        }
      }
    )";
  EXPECT_THAT(failed_patches, EqualsProto(kExpectedFailed));
}

}  // namespace
}  // namespace pdf
}  // namespace cpu_instructions
