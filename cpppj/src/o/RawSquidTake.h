// 원문 Take 부모의 진단/현재 가상 해제/생성자 반환 객체와 저장 순서를 보관한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 실제 슬롯/생성자/가상 몸체는 같은 공장이 제공한다.
struct RawSquidTakeHooks { // 객체는 장치가 해석하는32비트 토큰으로 원래 SID와 다를 수 있다.
    std::function<std::uint32_t()> capacity; // 진입 시 현재 numSquids를 읽는다.
    std::function<std::uint32_t(std::uint32_t)> slot; // 현재 풀 기준에서 인자의 실제 객체를 얻는다.
    std::function<std::uint8_t(std::uint32_t)> state,type; // 원문 위치마다 같은 객체의 현재 상태/TYPE BYTE를 읽는다.
    std::function<std::uint32_t(std::uint32_t)> vtable; // 가상 호출 전에 선택하는 현재 VT 기록이다.
    std::function<void(std::uint32_t,std::uint32_t)> detail; // 서버/클라이언트 TYPE 이름의 원문 상세를 복사한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 진단 정상 복귀 뒤 계속한다.
    std::function<void(std::uint32_t,std::uint32_t)> unpop; // 선택한 객체/VT의+48을 인자0으로 실행한다.
    std::function<void(std::uint32_t,std::uint8_t)> storeState,storeType; // 상태와 TYPE BYTE만 쓰며 다른 payload는 보존한다.
    std::function<std::uint32_t(std::uint32_t)> constructor; // 해제/상태 변경 뒤 현재 TYPE 생성자를 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> construct; // 주소0은 base이며 cdecl 반환 객체를 그대로 사용한다.
    std::function<void(std::uint32_t,std::uint32_t)> postTake; // 저장한 실제 VT의+2c를 실행한다.
};
std::uint32_t TakeRawSquid(OriginalEdition edition,std::uint32_t type,std::uint32_t sid,const RawSquidTakeHooks& hooks); //004af610/CD004ab440이며 할당/자유 목록 변경은 하지 않는다.
}
