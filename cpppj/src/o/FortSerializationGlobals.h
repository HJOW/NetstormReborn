// Template 생성자와 객체 저장·로더가 같은 Client 수명으로 공유하는 원본 전역이다.
#pragma once
#include <cstdint>

namespace netstorm::o {
//10.78 판본/깊이/객체저장범위와현재두mask를소유한다. Storage/Session/NoWorld저장은각필드의같은주소를빌린다.
struct FortSerializationGlobals {
    std::uint32_t version{}; // manager생성자는0, 전체Objects저장은2를 쓰며 각 소비자는현재값을읽는다.
    std::uint32_t loadingDepth{}; //005c89b8의 BSS0이며 Session/World 없는 메타 Core 로더가 같은 중첩 깊이를 빌린다.
    std::uint32_t scope{}; //005c89b0의BSS0이며실제Terr1/Chaff2로더와writer가World없이도같은현재DWORD를쓴다.
    std::uint32_t ownerSavedMask{0x5d77cf00}; //00542644의현재flags2저장마스크이며Session교체/반납이PE초기값으로덮지않는다.
    std::uint32_t battleSavedMask{0x40480002}; //0054263c의현재전투저장마스크이며다른FortKeepGenus00542640과구별한다.
};
}
