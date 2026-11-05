// 공통 Form 부착의 현재 TYPE/가상 setter와 진단 후 부분 쓰기를 연결한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 원문 thiscall의 같은 물리 슬롯을 사용하며 클래스별 +74 접두와 구별한다.
struct FormAttachHooks { // 선택된 자식만 요구하며 콜백 후 현재 바이트를 원문 위치에서 재조회한다.
    std::function<std::uint32_t(std::uint8_t)> genus; // 현재 raw TYPE의 flags2를 읽는다.
    std::function<void(Sid,std::uint32_t,Sid)> setParent; // 현재 VT+8의 실제 부모 setter다.
    std::function<void(Sid,std::uint32_t,Sid)> setPrevious; // 현재 VT+c의 실제 이전 setter다.
    std::function<void(Sid,std::uint32_t)> pop; // 직접 공통 Pop 자식이며 flags|100을 전달한다.
    std::function<std::uint32_t()> contentType; // Pop 뒤 현재 전체 contentFormType DWORD다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 식/파일/행의 정상 복귀도 허용한다.
};
void AttachCommonForm(SidPool& pool,Sid form,Sid parent,std::uint32_t flags,const FormAttachHooks& hooks); // PATCH004ac800/CD004ac2e0만 실행한다. 알 수 없는 실제 자식은 선택 위치에서 실패한다.
}
