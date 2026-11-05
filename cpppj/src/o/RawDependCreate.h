// 감시 Form 두 개의 현재 생성/링크/가상 부착 순서만 원문 부모로 실행한다.
#pragma once
#include "o/SidPool.h"
#include <functional>
#include <string_view>
namespace netstorm::o { // 객체 토큰과 원문 SID 변환을 구분해 실제 풀과 독립 관측을 함께 연결한다.
struct DependCreateHooks {
    std::function<bool()> server; // 두 부모 모두 client 번호가 아닐 때만 현재 서버 권한을 읽는다.
    std::function<std::uint32_t()> formType; // 각 Create 직전의 전체 현재 DWORD를 따로 읽는다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t)> create; // 현재 공통 Factory가 반환한 객체 토큰이다.
    std::function<std::uint32_t(std::uint32_t)> sid; // 같은 물리 기준으로 반환 객체의 원문 SID를 계산한다.
    std::function<void(std::uint32_t,std::size_t,std::uint16_t)> word; // payload의 지정 WORD만 기록하고 앞선 쓰기를 취소하지 않는다.
    std::function<std::uint32_t(std::uint32_t)> vtable; // 각 원문 위치에서 선택한 현재 VT 토큰을 반환한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> attach; // 저장한 VT+74와 전체 부모 DWORD/flags를 호출한다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 원문 server 진단이 정상 복귀하면 flags0으로 계속한다.
};
struct DependCreateObjects { // 원문 함수는 void이며 이 반환은 호스트의 생성 객체 추적용 부가 결과다.
    std::uint32_t watched{},watcher{}; // 두 번째/첫 번째 Create가 반환한 객체를 원래 방향으로 보관한다.
};
DependCreateObjects CreateDependForms(OriginalEdition edition,std::uint32_t watched,std::uint32_t watcher,const DependCreateHooks& hooks); // PATCH004b1510/CD004af940의 부모다.
}
