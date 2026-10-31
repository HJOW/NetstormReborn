//00558dd4의 CD 루트 캐시는 Scene/Session/화면 장치와 별개의 Client 장기 물리다.
#pragma once
#include "client/CdPresence.h"
#include <array>

namespace netstorm::client { // State 정리/새 미션에서 캐시를 다시 초기화하지 않는다.
class ClientCdGlobals {
public:
    std::span<char,128> CachedRoot() { return cache_; } // 같은 현재128BYTE를 실제 소비자가 빌린다. 조회로 장치를 실행하지 않는다.
    std::span<const char,128> CachedRoot() const { return cache_; } // source가 없는 별도 표시 문자열을 만들지 않는다.
    std::uint32_t Check(const CdPresenceHooks& hooks) { std::array<char,128> scratch;return CheckCdPresence(cache_,scratch,hooks); } // 원문의 지역 stack은 열거가 선택될 때만 장치가 작성한다.
    void InitializeStartup(const CdPresenceHooks& hooks) { startupPresent_=0;startupPresent_=Check(hooks); } //004392e7..004392f9의0054db6c0 쓰기→실제440c60→현재 결과 저장이다. 실패/예외 앞선0도 보존한다.
    const std::uint32_t& StartupPresent() const { return startupPresent_; } //0054db6c는 State7의005c85b8/현재 캐시와 별도이며 재조회/정리로 덮지 않는다.
private:
    std::array<char,128> cache_{}; // 최초 BSS0이며 해제/화면 전환 때 다른 경로 캐시로 덮지 않는다.
    std::uint32_t startupPresent_{}; // 최초 BSS0 뒤 실제 시작 위치만 기록하는 CD 결과 DWORD다.
};
}
