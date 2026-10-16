// unsigned32 감김으로 원본 CRT의 idiv 이전15비트 값을 보존한다.
#include "o/CrtRandom.h"

namespace netstorm::o {
// 원본 __getptd가 만드는 첫 holdrand 또는 명시 srand 상태를 받는다.
CrtRandom::CrtRandom(std::uint32_t state):state_(state) {}
// 부호 있는 곱셈 overflow 없이 실제 low32를 만든다.
std::uint32_t CrtRandom::Next() { state_=state_*214013U+2531011U;return (state_>>16)&32767U; }
// 진단은 스트림을 전진시키지 않는다.
std::uint32_t CrtRandom::State() const { return state_; }
// 0도 유효한 원본 srand 입력이다.
void CrtRandom::SetState(std::uint32_t state) { state_=state; }
// 각 스레드에 하나만 두고 Session/미션 재생성으로 리셋하지 않는다.
CrtRandom& ThreadCrtRandom() { static thread_local CrtRandom random;return random; }
}
