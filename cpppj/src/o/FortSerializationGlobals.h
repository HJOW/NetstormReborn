// Template 생성자와 객체 저장·로더가 같은 Client 수명으로 공유하는 원본 전역이다.
#pragma once
#include <cstdint>

namespace netstorm::o {
//10.78 판본005c89c0/중첩깊이005c89b8의 두 DWORD를 소유한다. Storage/Session/목록은 각 필드의 같은 주소를 빌린다.
struct FortSerializationGlobals {
    std::uint32_t version{}; // manager생성자는0, 전체Objects저장은2를 쓰며 각 소비자는현재값을읽는다.
    std::uint32_t loadingDepth{}; //005c89b8의 BSS0이며 Session/World 없는 메타 Core 로더가 같은 중첩 깊이를 빌린다.
};
}
