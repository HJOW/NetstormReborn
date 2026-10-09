// Squid의 프레임 지정(004acee0 ↔ CD 004acbc0)을 raw SID 풀 위에서 복원한다.
// 표시 갱신·Unpop·Pop은 가상 함수이므로 호출자가 연결하고 순서·인자만 원본대로 호출한다.
// 근거와 검증 범위: docs/exe/cpp-setframe-reconstruction.md, 세 실제 PE의 제한 x86 기대값 setframe-x86.tsv.
#pragma once
#include "o/SidPool.h"
#include <array>
#include <functional>

namespace netstorm::o {
// 프레임 지정이 받는 flags다. 0은 +0x88, 0x2000은 +0x8c 가상 표시 갱신을 쓴다. 그 밖의 값은 원본 assert다.
inline constexpr std::uint32_t kFrameUpdateAlternate = 0x2000;
// 해시 단계가 맞지 않아 다시 등록할 때 Pop에 더하는 flags다(원본 `or edi, 0x50`).
inline constexpr std::uint32_t kFrameRepopFlags = 0x50;
// 프레임 지정이 부르는 외부 효과다. 호출 순서는 원본과 같다.
struct SquidFrameHooks {
    // 00419850(type, 0, frame): 그 프레임의 SHP 추가 헤더 +0·+4의 float 둘(칸 단위 크기)이다.
    // 패치판은 프레임이 표 밖이거나 SHP가 없으면 assert한다. 이 훅이 예외로 거부해야 한다.
    std::function<std::array<float,2>(Sid sid,std::int32_t frame)> frameSize;
    // 가상 표시 갱신. flags에 0x2000이 있으면 +0x8c, 없으면 +0x88이다(base의 +0x8c는 +0x88로 넘어간다).
    std::function<void(Sid sid,std::uint32_t flags)> update;
    // 가상 Unpop(vtable +0x48).
    std::function<void(Sid sid,std::uint32_t flags)> unpop;
    // 가상 표 +0x4c(0041c0d0 ↔ CD 00401c60)가 부르는 가상 Pop(+0x90): 현재 위치에 flags로 다시 등록한다.
    std::function<void(Sid sid,float x,float y,std::uint32_t flags)> pop;
};
class SquidFrame {
public:
    // 풀은 이 객체보다 오래 살아야 한다. 타입 표는 복사하고 훅은 모두 연결돼 있어야 한다.
    SquidFrame(SidPool& pool,std::span<const RiftTypeRecord> types,SquidFrameHooks hooks);
    // 004acee0 ↔ CD 004acbc0. **현재** 프레임으로 구한 해시 단계를 단계 바이트에 쓰고, 그것이 쓰기 전의 단계 바이트와
    // 같으면 표시 갱신 → 프레임 쓰기 → 표시 갱신(새 프레임이 현재와 같아도 한다), 다르면 새 프레임이 현재와 다를 때만
    // Unpop(flags) → 프레임 쓰기 → 현재 위치에 Pop(flags | 0x50)을 한다. 새 프레임의 범위는 원본도 검사하지 않는다.
    void Set(Sid sid,std::int32_t frame,std::uint32_t flags) const;
    // 004ace40 ↔ CD 004acb00: 객체의 현재 프레임으로 해시 단계를 구해 단계 바이트(패치 +0x21, CD +0x1f)에 쓰고 돌려준다.
    // island·bridge genus는 0, 그 밖은 프레임 크기의 큰 쪽이 2 이하면 1, 4 이하면 2, 넘으면 3이다.
    int StoreLevel(Sid sid) const;
    // 프레임 진행/이벤트 연결 시 같은 raw 월드인지 확인하는 읽기 전용 풀 참조다.
    const SidPool& Pool() const;
private:
    // 판본별 프레임 번호 필드를 읽는다. 패치는 +0x24 DWORD, CD는 +0x22 바이트다.
    std::int32_t Frame(Sid sid) const;
    SidPool& pool_;
    std::vector<RiftTypeRecord> types_;
    SquidFrameHooks hooks_;
};
}
