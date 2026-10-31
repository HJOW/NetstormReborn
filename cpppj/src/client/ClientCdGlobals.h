//00558dd4의 CD 루트 캐시는 Scene/Session/화면 장치와 별개의 Client 장기 물리다.
#pragma once
#include "client/CdPresence.h"
#include <array>

namespace netstorm::client { // State 정리/새 미션에서 캐시를 다시 초기화하지 않는다.
class ClientCdGlobals {
public:
    std::span<char,128> CachedRoot() { return cache_; } // 같은 현재128BYTE이며 검사/MCP는 이 실제 주소를 읽는다.
    std::span<const char,128> CachedRoot() const { return cache_; } // source가 없는 별도 표시 문자열을 만들지 않는다.
    std::uint32_t Check(const CdPresenceHooks& hooks) { std::array<char,128> scratch;return CheckCdPresence(cache_,scratch,hooks); } // 원문의 지역 stack은 열거가 선택될 때만 장치가 작성한다.
private:
    std::array<char,128> cache_{}; // 최초 BSS0이며 해제/화면 전환 때 다른 경로 캐시로 덮지 않는다.
};
}
