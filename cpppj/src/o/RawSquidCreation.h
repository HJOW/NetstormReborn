// 원문 Create의 호출 순서만 보관하고 슬롯/TYPE/가상 장치는 같은 실제 공장에 맡긴다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>

namespace netstorm::o { // 두 판본의 raw 생성 부모와 호스트 장치 경계를 연결한다.
struct RawSquidCreationHooks { // 반환 객체는 장치가 해석하는32비트 토큰이며 할당 SID와 다를 수 있다.
    std::function<std::uint32_t()> fakeSurfaceType; // PATCH 진입 시 현재005412e0을 읽는다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 진단이 정상 복귀하면 다음 명령부터 계속한다.
    std::function<std::uint32_t(std::uint32_t)> allocate; // 같은 풀의 실제 할당을 실행한다.
    std::function<std::uint32_t(std::uint32_t)> constructor; // 할당/진단 뒤 현재 전체DWORD TYPE의 생성자를 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> construct; // 주소0은base, 나머지는cdecl의 실제 반환 객체를 돌려준다.
    std::function<std::uint32_t(std::uint32_t)> vtable; // 생성자가 반환한 객체의 현재VT 기록을 읽는다.
    std::function<void(std::uint32_t,std::uint8_t)> storeType; // 반환 객체에 원문 인자의 하위BYTE만 쓴다.
    std::function<void(std::uint32_t,std::uint32_t)> postCreate; // 저장한VT의+28을 TYPE 쓰기 뒤 선택하고 실행한다.
};
std::uint32_t CreateRawSquid(OriginalEdition edition,std::uint32_t type,std::uint32_t flags,const RawSquidCreationHooks& hooks); //004af530/CD004ab390의 부모 순서와 signed SID 진단을 실행한다.
}
