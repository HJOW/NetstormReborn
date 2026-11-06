// PATCH 정렬 Content 부착의 링크·현재 가상 호출·늦은 Pop/부모 통지를 제공한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 원문 물리 슬롯과 가상 표 토큰을 빌리는 정렬 부모다.
struct ContentAttachHooks { // 선택한 장치를 원문 소비 위치에서만 요구한다.
    std::function<std::uint32_t()> contentType; // 반복마다 현재00541088 전체 DWORD를 읽는다.
    std::function<Sid(Sid,std::uint32_t)> previous; // 선택해 저장한 VT+4 getter다.
    std::function<void(Sid,std::uint32_t,Sid)> setPrevious; // 현재 또는 저장한 VT+c WORD setter다.
    std::function<void(Sid,std::uint32_t,Sid)> setParent; // 순서 BYTE 뒤 현재 VT+8이다.
    std::function<void(Sid,std::uint32_t)> pop; // 링크/부모/contained 뒤 실제 공통 Pop이다.
    std::function<void(Sid,Sid,std::uint32_t)> notice; // 저장한 부모의 이유1 통지다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단 복귀 뒤 저장값/현재값을 구별한다.
};
bool AttachSortedContent(SidPool& pool,Sid form,Sid parent,std::uint32_t flags,const ContentAttachHooks& hooks); //004aec10의 통지까지 실행한다. dead/free 반환은 false이며 부분 실패는 rollback하지 않는다.
}
