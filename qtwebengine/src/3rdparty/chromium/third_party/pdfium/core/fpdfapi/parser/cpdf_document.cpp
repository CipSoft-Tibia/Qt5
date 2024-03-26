// Copyright 2014 The PDFium Authors
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

// Original code copyright 2014 Foxit Software Inc. http://www.foxitsoftware.com

#include "core/fpdfapi/parser/cpdf_document.h"

#include <utility>

#include "core/fpdfapi/parser/cpdf_array.h"
#include "core/fpdfapi/parser/cpdf_dictionary.h"
#include "core/fpdfapi/parser/cpdf_linearized_header.h"
#include "core/fpdfapi/parser/cpdf_name.h"
#include "core/fpdfapi/parser/cpdf_number.h"
#include "core/fpdfapi/parser/cpdf_parser.h"
#include "core/fpdfapi/parser/cpdf_read_validator.h"
#include "core/fpdfapi/parser/cpdf_reference.h"
#include "core/fpdfapi/parser/cpdf_stream.h"
#include "core/fpdfapi/parser/cpdf_stream_acc.h"
#include "core/fpdfapi/parser/fpdf_parser_utility.h"
#include "core/fxcodec/jbig2/JBig2_DocumentContext.h"
#include "core/fxcrt/fx_codepage.h"
#include "core/fxcrt/scoped_set_insertion.h"
#include "core/fxcrt/stl_util.h"
#include "third_party/abseil-cpp/absl/types/optional.h"
#include "third_party/base/check.h"
#include "third_party/base/containers/contains.h"

namespace {

const int kMaxPageLevel = 1024;

// Returns a value in the range [0, `CPDF_Document::kPageMaxNum`), or nullopt on
// error.
absl::optional<int> CountPages(
    RetainPtr<CPDF_Dictionary> pPages,
    std::set<RetainPtr<CPDF_Dictionary>>* visited_pages) {
  int count = pPages->GetIntegerFor("Count");
  if (count > 0 && count < CPDF_Document::kPageMaxNum)
    return count;
  RetainPtr<CPDF_Array> pKidList = pPages->GetMutableArrayFor("Kids");
  if (!pKidList)
    return 0;
  count = 0;
  for (size_t i = 0; i < pKidList->size(); i++) {
    RetainPtr<CPDF_Dictionary> pKid = pKidList->GetMutableDictAt(i);
    if (!pKid || pdfium::Contains(*visited_pages, pKid))
      continue;
    if (pKid->KeyExist("Kids")) {
      // Use |visited_pages| to help detect circular references of pages.
      ScopedSetInsertion<RetainPtr<CPDF_Dictionary>> local_add(visited_pages,
                                                               pKid);
      absl::optional<int> local_count =
          CountPages(std::move(pKid), visited_pages);
      if (!local_count.has_value()) {
        return absl::nullopt;  // Propagate error.
      }
      count += local_count.value();
    } else {
      // This page is a leaf node.
      count++;
    }
    if (count >= CPDF_Document::kPageMaxNum) {
      return absl::nullopt;  // Error: too many pages.
    }
  }
  pPages->SetNewFor<CPDF_Number>("Count", count);
  return count;
}

int FindPageIndex(const CPDF_Dictionary* pNode,
                  uint32_t* skip_count,
                  uint32_t objnum,
                  int* index,
                  int level) {
  if (!pNode->KeyExist("Kids")) {
    if (objnum == pNode->GetObjNum())
      return *index;

    if (*skip_count != 0)
      (*skip_count)--;

    (*index)++;
    return -1;
  }

  RetainPtr<const CPDF_Array> pKidList = pNode->GetArrayFor("Kids");
  if (!pKidList)
    return -1;

  if (level >= kMaxPageLevel)
    return -1;

  size_t count = pNode->GetIntegerFor("Count");
  if (count <= *skip_count) {
    (*skip_count) -= count;
    (*index) += count;
    return -1;
  }

  if (count && count == pKidList->size()) {
    for (size_t i = 0; i < count; i++) {
      RetainPtr<const CPDF_Reference> pKid =
          ToReference(pKidList->GetObjectAt(i));
      if (pKid && pKid->GetRefObjNum() == objnum)
        return static_cast<int>(*index + i);
    }
  }

  for (size_t i = 0; i < pKidList->size(); i++) {
    RetainPtr<const CPDF_Dictionary> pKid = pKidList->GetDictAt(i);
    if (!pKid || pKid == pNode)
      continue;

    int found_index =
        FindPageIndex(pKid.Get(), skip_count, objnum, index, level + 1);
    if (found_index >= 0)
      return found_index;
  }
  return -1;
}

}  // namespace

CPDF_Document::CPDF_Document(std::unique_ptr<RenderDataIface> pRenderData,
                             std::unique_ptr<PageDataIface> pPageData)
    : m_pDocRender(std::move(pRenderData)),
      m_pDocPage(std::move(pPageData)),
      m_StockFontClearer(m_pDocPage.get()) {
  m_pDocRender->SetDocument(this);
  m_pDocPage->SetDocument(this);
}

CPDF_Document::~CPDF_Document() {
  // Be absolutely certain that |m_pExtension| is null before destroying
  // the extension, to avoid re-entering it while being destroyed. clang
  // seems to already do this for us, but the C++ standards seem to
  // indicate the opposite.
  m_pExtension.reset();
}

// static
bool CPDF_Document::IsValidPageObject(const CPDF_Object* obj) {
  // See ISO 32000-1:2008 spec, table 30.
  return ValidateDictType(ToDictionary(obj), "Page");
}

RetainPtr<CPDF_Object> CPDF_Document::ParseIndirectObject(uint32_t objnum) {
  return m_pParser ? m_pParser->ParseIndirectObject(objnum) : nullptr;
}

bool CPDF_Document::TryInit() {
  SetLastObjNum(m_pParser->GetLastObjNum());

  RetainPtr<CPDF_Object> pRootObj =
      GetOrParseIndirectObject(m_pParser->GetRootObjNum());
  if (pRootObj)
    m_pRootDict = pRootObj->GetMutableDict();

  LoadPages();
  return GetRoot() && GetPageCount() > 0;
}

CPDF_Parser::Error CPDF_Document::LoadDoc(
    RetainPtr<IFX_SeekableReadStream> pFileAccess,
    const ByteString& password) {
  if (!m_pParser)
    SetParser(std::make_unique<CPDF_Parser>(this));

  return HandleLoadResult(
      m_pParser->StartParse(std::move(pFileAccess), password));
}

CPDF_Parser::Error CPDF_Document::LoadLinearizedDoc(
    RetainPtr<CPDF_ReadValidator> validator,
    const ByteString& password) {
  if (!m_pParser)
    SetParser(std::make_unique<CPDF_Parser>(this));

  return HandleLoadResult(
      m_pParser->StartLinearizedParse(std::move(validator), password));
}

void CPDF_Document::LoadPages() {
  const CPDF_LinearizedHeader* linearized_header =
      m_pParser->GetLinearizedHeader();
  if (!linearized_header) {
    m_PageList.resize(RetrievePageCount());
    return;
  }

  uint32_t objnum = linearized_header->GetFirstPageObjNum();
  if (!IsValidPageObject(GetOrParseIndirectObject(objnum).Get())) {
    m_PageList.resize(RetrievePageCount());
    return;
  }

  uint32_t first_page_num = linearized_header->GetFirstPageNo();
  uint32_t page_count = linearized_header->GetPageCount();
  DCHECK(first_page_num < page_count);
  m_PageList.resize(page_count);
  m_PageList[first_page_num] = objnum;
}

RetainPtr<CPDF_Dictionary> CPDF_Document::TraversePDFPages(int iPage,
                                                           int* nPagesToGo,
                                                           size_t level) {
  if (*nPagesToGo < 0 || m_bReachedMaxPageLevel)
    return nullptr;

  RetainPtr<CPDF_Dictionary> pPages = m_pTreeTraversal[level].first;
  RetainPtr<CPDF_Array> pKidList = pPages->GetMutableArrayFor("Kids");
  if (!pKidList) {
    m_pTreeTraversal.pop_back();
    if (*nPagesToGo != 1)
      return nullptr;
    m_PageList[iPage] = pPages->GetObjNum();
    return pPages;
  }
  if (level >= kMaxPageLevel) {
    m_pTreeTraversal.pop_back();
    m_bReachedMaxPageLevel = true;
    return nullptr;
  }
  RetainPtr<CPDF_Dictionary> page;
  for (size_t i = m_pTreeTraversal[level].second; i < pKidList->size(); i++) {
    if (*nPagesToGo == 0)
      break;
    pKidList->ConvertToIndirectObjectAt(i, this);
    RetainPtr<CPDF_Dictionary> pKid = pKidList->GetMutableDictAt(i);
    if (!pKid) {
      (*nPagesToGo)--;
      m_pTreeTraversal[level].second++;
      continue;
    }
    if (pKid == pPages) {
      m_pTreeTraversal[level].second++;
      continue;
    }
    if (!pKid->KeyExist("Kids")) {
      m_PageList[iPage - (*nPagesToGo) + 1] = pKid->GetObjNum();
      (*nPagesToGo)--;
      m_pTreeTraversal[level].second++;
      if (*nPagesToGo == 0) {
        page = std::move(pKid);
        break;
      }
    } else {
      // If the vector has size level+1, the child is not in yet
      if (m_pTreeTraversal.size() == level + 1)
        m_pTreeTraversal.emplace_back(std::move(pKid), 0);
      // Now m_pTreeTraversal[level+1] should exist and be equal to pKid.
      RetainPtr<CPDF_Dictionary> pPageKid =
          TraversePDFPages(iPage, nPagesToGo, level + 1);
      // Check if child was completely processed, i.e. it popped itself out
      if (m_pTreeTraversal.size() == level + 1)
        m_pTreeTraversal[level].second++;
      // If child did not finish, no pages to go, or max level reached, end
      if (m_pTreeTraversal.size() != level + 1 || *nPagesToGo == 0 ||
          m_bReachedMaxPageLevel) {
        page = std::move(pPageKid);
        break;
      }
    }
  }
  if (m_pTreeTraversal[level].second == pKidList->size())
    m_pTreeTraversal.pop_back();
  return page;
}

void CPDF_Document::ResetTraversal() {
  m_iNextPageToTraverse = 0;
  m_bReachedMaxPageLevel = false;
  m_pTreeTraversal.clear();
}

void CPDF_Document::SetParser(std::unique_ptr<CPDF_Parser> pParser) {
  DCHECK(!m_pParser);
  m_pParser = std::move(pParser);
}

CPDF_Parser::Error CPDF_Document::HandleLoadResult(CPDF_Parser::Error error) {
  if (error == CPDF_Parser::SUCCESS)
    m_bHasValidCrossReferenceTable = !m_pParser->xref_table_rebuilt();
  return error;
}

RetainPtr<const CPDF_Dictionary> CPDF_Document::GetPagesDict() const {
  const CPDF_Dictionary* pRoot = GetRoot();
  return pRoot ? pRoot->GetDictFor("Pages") : nullptr;
}

RetainPtr<CPDF_Dictionary> CPDF_Document::GetMutablePagesDict() {
  return pdfium::WrapRetain(
      const_cast<CPDF_Dictionary*>(this->GetPagesDict().Get()));
}

bool CPDF_Document::IsPageLoaded(int iPage) const {
  return !!m_PageList[iPage];
}

RetainPtr<const CPDF_Dictionary> CPDF_Document::GetPageDictionary(int iPage) {
  if (!fxcrt::IndexInBounds(m_PageList, iPage))
    return nullptr;

  const uint32_t objnum = m_PageList[iPage];
  if (objnum) {
    RetainPtr<CPDF_Dictionary> result =
        ToDictionary(GetOrParseIndirectObject(objnum));
    if (result)
      return result;
  }

  RetainPtr<CPDF_Dictionary> pPages = GetMutablePagesDict();
  if (!pPages)
    return nullptr;

  if (m_pTreeTraversal.empty()) {
    ResetTraversal();
    m_pTreeTraversal.emplace_back(std::move(pPages), 0);
  }
  int nPagesToGo = iPage - m_iNextPageToTraverse + 1;
  RetainPtr<CPDF_Dictionary> pPage = TraversePDFPages(iPage, &nPagesToGo, 0);
  m_iNextPageToTraverse = iPage + 1;
  return pPage;
}

RetainPtr<CPDF_Dictionary> CPDF_Document::GetMutablePageDictionary(int iPage) {
  return pdfium::WrapRetain(
      const_cast<CPDF_Dictionary*>(GetPageDictionary(iPage).Get()));
}

void CPDF_Document::SetPageObjNum(int iPage, uint32_t objNum) {
  m_PageList[iPage] = objNum;
}

JBig2_DocumentContext* CPDF_Document::GetOrCreateCodecContext() {
  if (!m_pCodecContext)
    m_pCodecContext = std::make_unique<JBig2_DocumentContext>();
  return m_pCodecContext.get();
}

RetainPtr<CPDF_Stream> CPDF_Document::CreateModifiedAPStream() {
  auto stream = NewIndirect<CPDF_Stream>();
  m_ModifiedAPStreamIDs.insert(stream->GetObjNum());
  return stream;
}

bool CPDF_Document::IsModifiedAPStream(const CPDF_Stream* stream) const {
  return stream && pdfium::Contains(m_ModifiedAPStreamIDs, stream->GetObjNum());
}

int CPDF_Document::GetPageIndex(uint32_t objnum) {
  uint32_t skip_count = 0;
  bool bSkipped = false;
  for (uint32_t i = 0; i < m_PageList.size(); ++i) {
    if (m_PageList[i] == objnum)
      return i;

    if (!bSkipped && m_PageList[i] == 0) {
      skip_count = i;
      bSkipped = true;
    }
  }
  RetainPtr<const CPDF_Dictionary> pPages = GetPagesDict();
  if (!pPages)
    return -1;

  int start_index = 0;
  int found_index = FindPageIndex(pPages, &skip_count, objnum, &start_index, 0);

  // Corrupt page tree may yield out-of-range results.
  if (!fxcrt::IndexInBounds(m_PageList, found_index))
    return -1;

  // Only update |m_PageList| when |objnum| points to a /Page object.
  if (IsValidPageObject(GetOrParseIndirectObject(objnum).Get()))
    m_PageList[found_index] = objnum;
  return found_index;
}

int CPDF_Document::GetPageCount() const {
  return fxcrt::CollectionSize<int>(m_PageList);
}

int CPDF_Document::RetrievePageCount() {
  RetainPtr<CPDF_Dictionary> pPages = GetMutablePagesDict();
  if (!pPages)
    return 0;

  if (!pPages->KeyExist("Kids"))
    return 1;

  std::set<RetainPtr<CPDF_Dictionary>> visited_pages = {pPages};
  return CountPages(std::move(pPages), &visited_pages).value_or(0);
}

uint32_t CPDF_Document::GetUserPermissions() const {
  if (m_pParser)
    return m_pParser->GetPermissions();

  return m_pExtension ? m_pExtension->GetUserPermissions() : 0;
}

RetainPtr<CPDF_StreamAcc> CPDF_Document::GetFontFileStreamAcc(
    RetainPtr<const CPDF_Stream> pFontStream) {
  return m_pDocPage->GetFontFileStreamAcc(std::move(pFontStream));
}

void CPDF_Document::MaybePurgeFontFileStreamAcc(
    RetainPtr<CPDF_StreamAcc>&& pStreamAcc) {
  if (m_pDocPage)
    m_pDocPage->MaybePurgeFontFileStreamAcc(std::move(pStreamAcc));
}

void CPDF_Document::MaybePurgeImage(uint32_t objnum) {
  if (m_pDocPage)
    m_pDocPage->MaybePurgeImage(objnum);
}

void CPDF_Document::CreateNewDoc() {
  DCHECK(!m_pRootDict);
  DCHECK(!m_pInfoDict);
  m_pRootDict = NewIndirect<CPDF_Dictionary>();
  m_pRootDict->SetNewFor<CPDF_Name>("Type", "Catalog");

  auto pPages = NewIndirect<CPDF_Dictionary>();
  pPages->SetNewFor<CPDF_Name>("Type", "Pages");
  pPages->SetNewFor<CPDF_Number>("Count", 0);
  pPages->SetNewFor<CPDF_Array>("Kids");
  m_pRootDict->SetNewFor<CPDF_Reference>("Pages", this, pPages->GetObjNum());
  m_pInfoDict = NewIndirect<CPDF_Dictionary>();
}

RetainPtr<CPDF_Dictionary> CPDF_Document::CreateNewPage(int iPage) {
  auto pDict = NewIndirect<CPDF_Dictionary>();
  pDict->SetNewFor<CPDF_Name>("Type", "Page");
  uint32_t dwObjNum = pDict->GetObjNum();
  if (!InsertNewPage(iPage, pDict)) {
    DeleteIndirectObject(dwObjNum);
    return nullptr;
  }
  return pDict;
}

bool CPDF_Document::InsertDeletePDFPage(
    RetainPtr<CPDF_Dictionary> pPages,
    int nPagesToGo,
    RetainPtr<CPDF_Dictionary> pPageDict,
    bool bInsert,
    std::set<RetainPtr<CPDF_Dictionary>>* pVisited) {
  RetainPtr<CPDF_Array> pKidList = pPages->GetMutableArrayFor("Kids");
  if (!pKidList)
    return false;

  for (size_t i = 0; i < pKidList->size(); i++) {
    RetainPtr<CPDF_Dictionary> pKid = pKidList->GetMutableDictAt(i);
    if (pKid->GetNameFor("Type") == "Page") {
      if (nPagesToGo != 0) {
        nPagesToGo--;
        continue;
      }
      if (bInsert) {
        pKidList->InsertNewAt<CPDF_Reference>(i, this, pPageDict->GetObjNum());
        pPageDict->SetNewFor<CPDF_Reference>("Parent", this,
                                             pPages->GetObjNum());
      } else {
        pKidList->RemoveAt(i);
      }
      pPages->SetNewFor<CPDF_Number>(
          "Count", pPages->GetIntegerFor("Count") + (bInsert ? 1 : -1));
      ResetTraversal();
      break;
    }
    int nPages = pKid->GetIntegerFor("Count");
    if (nPagesToGo >= nPages) {
      nPagesToGo -= nPages;
      continue;
    }
    if (pdfium::Contains(*pVisited, pKid))
      return false;

    ScopedSetInsertion<RetainPtr<CPDF_Dictionary>> insertion(pVisited, pKid);
    if (!InsertDeletePDFPage(std::move(pKid), nPagesToGo, pPageDict, bInsert,
                             pVisited)) {
      return false;
    }
    pPages->SetNewFor<CPDF_Number>(
        "Count", pPages->GetIntegerFor("Count") + (bInsert ? 1 : -1));
    break;
  }
  return true;
}

bool CPDF_Document::InsertNewPage(int iPage,
                                  RetainPtr<CPDF_Dictionary> pPageDict) {
  RetainPtr<CPDF_Dictionary> pRoot = GetMutableRoot();
  if (!pRoot)
    return false;

  RetainPtr<CPDF_Dictionary> pPages = pRoot->GetMutableDictFor("Pages");
  if (!pPages)
    return false;

  int nPages = GetPageCount();
  if (iPage < 0 || iPage > nPages)
    return false;

  if (iPage == nPages) {
    RetainPtr<CPDF_Array> pPagesList = pPages->GetOrCreateArrayFor("Kids");
    pPagesList->AppendNew<CPDF_Reference>(this, pPageDict->GetObjNum());
    pPages->SetNewFor<CPDF_Number>("Count", nPages + 1);
    pPageDict->SetNewFor<CPDF_Reference>("Parent", this, pPages->GetObjNum());
    ResetTraversal();
  } else {
    std::set<RetainPtr<CPDF_Dictionary>> stack = {pPages};
    if (!InsertDeletePDFPage(std::move(pPages), iPage, pPageDict, true, &stack))
      return false;
  }
  m_PageList.insert(m_PageList.begin() + iPage, pPageDict->GetObjNum());
  return true;
}

RetainPtr<CPDF_Dictionary> CPDF_Document::GetInfo() {
  if (m_pInfoDict)
    return m_pInfoDict;

  if (!m_pParser)
    return nullptr;

  uint32_t info_obj_num = m_pParser->GetInfoObjNum();
  if (info_obj_num == 0)
    return nullptr;

  auto ref = pdfium::MakeRetain<CPDF_Reference>(this, info_obj_num);
  m_pInfoDict = ToDictionary(ref->GetMutableDirect());
  return m_pInfoDict;
}

RetainPtr<const CPDF_Array> CPDF_Document::GetFileIdentifier() const {
  return m_pParser ? m_pParser->GetIDArray() : nullptr;
}

void CPDF_Document::DeletePage(int iPage) {
  RetainPtr<CPDF_Dictionary> pPages = GetMutablePagesDict();
  if (!pPages)
    return;

  int nPages = pPages->GetIntegerFor("Count");
  if (iPage < 0 || iPage >= nPages)
    return;

  std::set<RetainPtr<CPDF_Dictionary>> stack = {pPages};
  if (!InsertDeletePDFPage(std::move(pPages), iPage, nullptr, false, &stack))
    return;

  m_PageList.erase(m_PageList.begin() + iPage);
}

void CPDF_Document::SetRootForTesting(RetainPtr<CPDF_Dictionary> root) {
  m_pRootDict = std::move(root);
}

void CPDF_Document::ResizePageListForTesting(size_t size) {
  m_PageList.resize(size);
}

CPDF_Document::StockFontClearer::StockFontClearer(
    CPDF_Document::PageDataIface* pPageData)
    : m_pPageData(pPageData) {}

CPDF_Document::StockFontClearer::~StockFontClearer() {
  m_pPageData->ClearStockFont();
}

CPDF_Document::PageDataIface::PageDataIface() = default;

CPDF_Document::PageDataIface::~PageDataIface() = default;

CPDF_Document::RenderDataIface::RenderDataIface() = default;

CPDF_Document::RenderDataIface::~RenderDataIface() = default;
