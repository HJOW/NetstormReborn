// 보행 프로세스의 도착 프레임 접두(0048bd90 / CD 00480580)와 Squid 방향 조회를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
struct PathAnimationHooks {
    std::function<void(Sid,std::uint32_t)> unpop;
    std::function<void(Sid,std::uint32_t)> repop;
};
class RawPathAnimation {
public:
    // 동일 raw 풀의 프레임 표와 가상 Unpop/현재 위치 Pop을 연결한다.
    RawPathAnimation(SidPool& pool,std::span<const RiftTypeFrames> frames,PathAnimationHooks hooks);
    // 004ad680 / CD 004ae3f0: 코드의 side를 signed char로 읽는다.
    static int Side(const RiftTypeFrames& frames,std::int32_t frame);
    // 004ad710 / CD 004ae4b0: signed side - 'A'다. 방향을 0..7로 제한하지 않는다.
    static int Direction(const RiftTypeFrames& frames,std::int32_t frame);
    // 원본 도착 프레임은 현재 글자 구간의 첫 물리 번호 +1이다. 코드의 number=1 검색과 구별한다.
    static std::int32_t RestFrame(const RiftTypeFrames& frames,std::int32_t frame);
    // 같은 프레임이어도 Unpop(0)→쓰기→Repop(0x40)한다. 후속 경로/프로세스 정리는 호출자에게 남긴다.
    void Stop(Sid sid) const;
private:
    // 패치 DWORD/CD BYTE의 현재 프레임을 읽고 자산/배열 범위를 확인한다.
    std::int32_t Current(Sid sid) const;
    SidPool& pool_;
    std::vector<RiftTypeFrames> frames_;
    PathAnimationHooks hooks_;
};
}
