// State 정리의 고정 Zone 객체004bfd60을 정확6BYTE 쓰기로 복원한다.
#pragma once
#include <cstdint>
#include <span>

namespace netstorm::o { //Zone은 State144BYTE 블록이나 인접8BYTE 전체와 별개다.
//현재 실제 객체의 DWORD0→+4 WORD0만 쓴다. 짧은 물리는 해당 store에서 거부하고 완료한 prefix는 보존한다.
void ResetStateZone(std::span<std::uint8_t> zone);
}
