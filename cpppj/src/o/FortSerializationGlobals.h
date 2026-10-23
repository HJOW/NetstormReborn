// Template 생성자와 객체 저장·로더가 같은 Client 수명으로 공유하는 원본 전역이다.
#pragma once
#include <cstdint>

namespace netstorm::o {
// 10.78의005c89c0은 실제DWORD한개다. Storage/Session/목록이 값 사본 대신 같은 객체를 빌린다.
struct FortSerializationGlobals {
    std::uint32_t version{}; // manager생성자는0, 전체Objects저장은2를 쓰며 각 소비자는현재값을읽는다.
};
}
