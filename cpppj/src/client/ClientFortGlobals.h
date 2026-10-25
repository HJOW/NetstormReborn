// 현재/pending Fort 이름과 목록/설정 dirty의 실제 Client 수명을 한 곳에 보관한다.
#pragma once
#include "client/SoloMapCatalog.h"
#include <array>
#include <span>

namespace netstorm::client { // 메뉴와 짧은 Session이 같은 이름/키/목록 물리를 빌린다.
// 원문 두 이름 버퍼와 FileSpec 포인터/목록/모드의 단일 소유자다.
class ClientFortGlobals {
public:
    ClientFortGlobals(); // 원문 두256BYTE 할당의 첫BYTE만 빈문자열로 초기화한다.
    ClientFortGlobals(const ClientFortGlobals&)=delete;ClientFortGlobals& operator=(const ClientFortGlobals&)=delete; // 고정 이름/목록/키 주소를 복제하지 않는다.
    std::span<std::uint8_t,256> CurrentBytes(); // 0055b490이 가리키는 실제 쓰기 버퍼다.
    std::span<std::uint8_t,256> PendingBytes(); // 0055b494의 별개256BYTE이며 current에 복사하지 않는다.
    std::string_view Current() const; // 현재 NUL 전까지 읽으며 미종료 버퍼를 빈문자열로 만들지 않는다.
    std::string_view Pending() const; // 같은 pending 물리를 빌려 별도 문자열 동기화를 피한다.
    SoloMapFileSpec& FileSpec(); // 005327ac의 같은9BYTE 키다. SelectSolo/SelectFort의 주소는 변하지 않는다.
    void SelectFileSpec(); // 005327e0을 같은 고정 키의 주소로 지정하며 키 내용은 보존한다.
    std::string_view SelectedFileSpec() const; // 현재 선택 포인터의 키를 PathSpec 소비 위치에서 읽는다.
    o::SoloMapListState& Catalog(); // 목록과 이름/설정이 같은00542678 dirty를 사용한다.
    std::uint32_t& SelectionMode(); // 005cae84의 현재 ReadFort/AskEditFort 모드다.
private:
    std::array<std::uint8_t,256> current_,pending_; // NUL 뒤 미작성 바이트를 임의 초기화하지 않는 두 실제 할당이다.
    SoloMapFileSpec fileSpec_;const SoloMapFileSpec* selectedFileSpec_; // 원문 기본 포인터도 처음부터 같은 고정 키를 가리킨다.
    o::SoloMapListState catalog_;std::uint32_t selectionMode_{}; // 고정50행 캐시/dirty와 최초BSS0 모드다.
};
}
