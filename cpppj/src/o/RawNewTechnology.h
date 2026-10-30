// 현재 기술부모의 직접 조회/추가와 snapshot·이름/그림·안내의 원문 순서를 공유한다.
#pragma once
#include "o/RiftType.h"
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>

namespace netstorm::o { // Help Learn과 로컬 획득이 같은 게임 본문을 사용한다.
// 게임 저장소와 현재 Client의 문자열/안내 장치는 선택된 위치에서만 요구한다.
struct NewTechnologyHooks {
    std::function<std::uint32_t()> technologyParent; //0048fc60의 현재 로컬 기술 부모이며 사용하지 않는 첫 조회도 실행한다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> find; //004acac0(parent,4,type,0)의 반환은 SID가 아닌 일치한 종류WORD다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> create; //004afa70(parent,type,0,4,10,0)이며 현재 부모를 다시 읽은 뒤 호출한다.
    std::function<void()> snapshot; //004bfac0의 두 독립 Battle/Fort 저장 선택이다. 기술이 이미 있어도 생략하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> type; //0049a840의 현재 TYPE 조회이며 반환 토큰을 이름 getter에 그대로 전달한다.
    std::function<std::string(std::uint32_t)> name; //0049af80의 현재 언어 이름이다. 반환 C 문자열의 첫NUL까지만 설정에 전달한다.
    std::function<void(std::string_view,std::string_view)> configure; //004414d0의 실제 두 설정 쓰기이며 첫 쓰기가 끝난 뒤 그림을 만든다.
    std::function<std::string(std::string_view,std::int32_t)> format; //004a89c0의256BYTE 지역 그림 서식과 signed %d 인자다.
    std::function<void(std::string_view)> tell; //004cf960("NewTech")의 실제 안내이며 마지막에만 호출한다.
};
// 10.78의00492a30 전체 순서다. 타입 범위/현재 부모의 실제 물리는 각 게임 child가 판정한다.
void AcquireNewTechnology(OriginalEdition edition,std::uint32_t type,const NewTechnologyHooks& hooks);
}
