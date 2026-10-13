// 보행 프로세스의 도착 프레임 접두(0048bd90 / CD 00480580)와 Squid 방향 조회를 복원한다.
#pragma once
#include "o/SidPool.h"
#include <functional>

namespace netstorm::o {
// 경로 애니메이션 외부 동작과 저장소 조회를 연결하는 콜백 묶음이다.
struct PathAnimationHooks {
    // unpop: 대상을 공간 등록에서 해제하는 Unpop 콜백.
    std::function<void(Sid,std::uint32_t)> unpop;
    // repop: 변경한 대상을 공간에 다시 반영하는 Repop 콜백.
    std::function<void(Sid,std::uint32_t)> repop;
};
// 보행 프로세스의 도착 프레임 접두(0048bd90 / CD 00480580)와 Squid 방향 조회를 복원한다.
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
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    SidPool& pool_;
    // frames_: 타입별 프레임 자료.
    std::vector<RiftTypeFrames> frames_;
    // hooks_: 외부 저장소 조회와 가상 동작을 연결하는 콜백 묶음 (PathAnimationHooks).
    PathAnimationHooks hooks_;
};
}
