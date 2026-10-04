// 원본: FUN_00452f90 @ 00452f90(넣기), FUN_00452fe0 @ 00452fe0(명령 넣기), FUN_00452f00 @ 00452f00(꺼내기) — 패치판 10.78.
// 범위: 큐 자체. 현재 키·버튼 상태를 읽는 폴링(FUN_00452e00)은 창이 필요해 ClientMain 쪽에 둔다.
#include "client/InputEvent.h"

namespace netstorm::client {
namespace {
// 꺼낼 때 바꾸는 글자: 0x7f(DEL)는 Ctrl+Backspace(8)로 바뀐다.
constexpr std::uint32_t kDeleteCharacter = 0x7f;
constexpr std::uint32_t kCharacterMask = 0x03ffffff;
// 명령 사건에 남기는 코드 비트.
constexpr std::uint32_t kCommandCodeMask = 0x0100ffff;
}

// 쓰기 위치를 한 칸 옮긴다. 읽기 위치와 만나면 한 칸 되돌려 방금 넣은 것을 버린다.
void InputQueue::Push(std::uint32_t code, std::int32_t x, std::int32_t y) {
    events_[write_] = {code, x, y};
    write_ = (write_ + 1) % kCapacity;
    if (write_ == read_) write_ = (write_ + kCapacity - 1) % kCapacity;
}
// 코드만 다듬고 같은 방식으로 넣는다.
void InputQueue::PushCommand(std::uint32_t code, std::int32_t x, std::int32_t y) {
    Push((code & kCommandCodeMask) | InputCode::kCommand, x, y);
}
// 원본은 꺼낸 뒤에 뗌 여부를 보므로, 걸러진 뗌 사건도 큐에서는 빠진다.
InputEvent InputQueue::Pop(bool skipReleases) {
    if (read_ != write_) {
        auto event = events_[read_];
        read_ = (read_ + 1) % kCapacity;
        if ((event.code & kCharacterMask) == kDeleteCharacter) event.code = (event.code & 0xff000008u) | 0x10000008u;
        if (!skipReleases || (event.code & InputCode::kRelease) == 0) return event;
    }
    return {};
}
// 읽기 위치와 쓰기 위치가 같으면 비었다.
bool InputQueue::Empty() const { return read_ == write_; }
}
