// 원문 이름의 NUL 경계와 서로 다른 고정 주소를 유지한다.
#include "client/ClientFortGlobals.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { // 실제 Client 소유자 이외에 이름 사본을 만들지 않는다.
namespace { // 고정 이름 버퍼의 같은 읽기 계약을 공유한다.
// NUL 뒤의 미작성 바이트를 읽지 않고 현재 C문자열 view만 반환한다.
std::string_view Name(std::span<const std::uint8_t> bytes) {
    const auto end=std::find(bytes.begin(),bytes.end(),std::uint8_t{});if (end==bytes.end()) throw std::out_of_range("Client Fort 이름의 NUL 물리 범위");
    return {reinterpret_cast<const char*>(bytes.data()),static_cast<std::size_t>(end-bytes.begin())};
}
}
ClientFortGlobals::ClientFortGlobals():selectedFileSpec_(&fileSpec_) { current_[0]=0;pending_[0]=0;catalog_.dirty=1; } // 원문 두 할당은 각 첫BYTE만 쓰며 보호PE00542678의 최초 dirty는1이다.
std::span<std::uint8_t,256> ClientFortGlobals::CurrentBytes() { return current_; } // 원문 setter의 같은 목적 주소다.
std::span<std::uint8_t,256> ClientFortGlobals::PendingBytes() { return pending_; } // 다음 상태가 나중에 읽는 별도 목적 주소다.
std::string_view ClientFortGlobals::Current() const { return Name(current_); } // 같은 현재 물리만 읽는다.
std::string_view ClientFortGlobals::Pending() const { return Name(pending_); } // 실제 선택 예약을 조회한다.
SoloMapFileSpec& ClientFortGlobals::FileSpec() { return fileSpec_; } // 고정 키의 쓰기 대여다.
void ClientFortGlobals::SelectFileSpec() { selectedFileSpec_=&fileSpec_; } // 포인터 선택과 키 내용 변경을 구별한다.
std::string_view ClientFortGlobals::SelectedFileSpec() const { return selectedFileSpec_->Current(); } // 각 경로 작성 때 현재 키를 재조회한다.
o::SoloMapListState& ClientFortGlobals::Catalog() { return catalog_; } // 같은50행과dirty DWORD를 직접 빌린다.
std::uint32_t& ClientFortGlobals::SelectionMode() { return selectionMode_; } // 같은 실제 모드의 참조다.
}
