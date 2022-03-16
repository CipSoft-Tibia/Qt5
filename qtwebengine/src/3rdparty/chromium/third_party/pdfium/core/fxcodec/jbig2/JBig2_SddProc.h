// Copyright 2015 PDFium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#ifndef CORE_FXCODEC_JBIG2_JBIG2_SDDPROC_H_
#define CORE_FXCODEC_JBIG2_JBIG2_SDDPROC_H_

#include <memory>
#include <vector>

#include "core/fxcodec/jbig2/JBig2_ArithDecoder.h"
#include "core/fxcrt/fx_system.h"

class CJBig2_BitStream;
class CJBig2_HuffmanTable;
class CJBig2_Image;
class CJBig2_SymbolDict;

class CJBig2_SDDProc {
 public:
  std::unique_ptr<CJBig2_SymbolDict> DecodeArith(
      CJBig2_ArithDecoder* pArithDecoder,
      std::vector<JBig2ArithCtx>* gbContext,
      std::vector<JBig2ArithCtx>* grContext);

  std::unique_ptr<CJBig2_SymbolDict> DecodeHuffman(
      CJBig2_BitStream* pStream,
      std::vector<JBig2ArithCtx>* gbContext,
      std::vector<JBig2ArithCtx>* grContext);

  bool SDHUFF;
  bool SDREFAGG;
  uint32_t SDNUMINSYMS;
  CJBig2_Image** SDINSYMS;
  uint32_t SDNUMNEWSYMS;
  uint32_t SDNUMEXSYMS;
  const CJBig2_HuffmanTable* SDHUFFDH;
  const CJBig2_HuffmanTable* SDHUFFDW;
  const CJBig2_HuffmanTable* SDHUFFBMSIZE;
  const CJBig2_HuffmanTable* SDHUFFAGGINST;
  uint8_t SDTEMPLATE;
  int8_t SDAT[8];
  bool SDRTEMPLATE;
  int8_t SDRAT[4];
};

#endif  // CORE_FXCODEC_JBIG2_JBIG2_SDDPROC_H_
