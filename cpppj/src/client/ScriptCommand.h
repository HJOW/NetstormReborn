// HTML이 예약한 명령과 Config 명령을 원본 토큰/입력 큐에 연결한다.
#pragma once
#include "client/InputEvent.h"
#include "o/ConfigInterface.h"
#include <array>
#include <functional>

namespace netstorm::client {
// 실제 원본 명령 표의 두 항목이다. 문자열 이름 조회는 별도로 공급한다.
inline constexpr std::uint32_t kScriptConfig=0x0200006d,kScriptWarning=0x02000102; // PATCH 원본 명령 표의 Config/Warning 코드다.
// 원본 80칸 문자열 고리의 한 항목이다. opaque 토큰은 호스트 포인터 폭과 무관하다.
struct ScriptStringSlot {
    std::string text; // 이 칸에 마지막으로 복사한 원본 바이트 문자열이다.
    std::uint32_t token{}; // 같은 칸의 이전 수명과 구별하는 비영 토큰이다.
};
// 004c7910의 고리 수명을 보존하며 원본처럼 같은 문자열도 매번 새 항목을 소비한다.
class ScriptStringRing {
public:
    // 다음 칸의 이전 문자열을 해제한 뒤 새 문자열을 복사하고 비영 opaque 토큰을 돌려준다.
    std::uint32_t Store(std::string_view text);
    // 현재80칸 안의 실제 토큰만 해석한다. 해제된 문자열 참조는 명시 오류다.
    std::string_view Resolve(std::uint32_t token) const;
    // 독립 원본 대조에서 현재 고리 위치를 확인한다.
    std::size_t Index() const;
private:
    std::array<ScriptStringSlot,80> slots_; // 원본80칸의 현재 수명이다.
    std::size_t index_{}; // BSS의 최초0에서 첫 Store 때1이 된다.
    std::uint32_t serial_{100}; // 호스트 토큰은 원본 Begin의 signed 주소 관문을 만족하는101..INT_MAX다. 실제 주소 값은 복제하지 않는다.
};
// 외부 명령 표/진단만 공급하고 토큰 해석·수명·큐 순서는 실제 모듈이 처리한다.
struct ScriptCommandHooks {
    std::function<std::uint32_t(std::string_view)> lookup; // 원본004d52c0의 현재 명령 표 조회다. 미등록은0xffffffff다.
    std::function<void(std::string_view)> unknown; // lookup0 또는-1에서 실행하는 원본 진단 경계다.
};
// 004c9090. 숫자로 시작하는 인자만 atol이며 그 외는 문자열 고리다. -1도 인자 처리를 마친 뒤 거부한다.
void QueueScriptCommand(std::string_view source,InputQueue& queue,ScriptStringRing& strings,const ScriptCommandHooks& hooks);
// 00441580. 원본 '='·개행 토큰으로 한 설정만 바꿔 onChanged를 호출하고 항상0을 반환한다.
std::int32_t ApplyScriptConfig(std::string_view source,o::ConfigInterface& config,const std::function<void()>& noKeyword={});
// 같은 원본 parser의 Set 경계다. 실제 Config.Set 공급과 독립 원본의 전체 키/값 대조에 사용한다.
std::int32_t ApplyScriptConfig(std::string_view source,const std::function<bool(std::string_view,std::string_view)>& set,const std::function<void()>& noKeyword={});
}
