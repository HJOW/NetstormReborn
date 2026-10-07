// SquidFinder(객체 번호, flag 0)의 첫 연결 이웃을 실제 raw 풀·일반 해시에서 찾는다.
// 원본: 004b23e0/004b1d70/004b1e80 ↔ CD 004ebad0/004ebf30/004ec040.
// 끝 칸 변환은 이 경로를 사용하며 flag 8의 표면 지도 탐색으로 대신할 수 없다.
#pragma once
#include "o/RawSquidFinder.h"

namespace netstorm::o {
class RawSquidNeighbors {
public:
    // 타입·프레임 표는 복사한다. 풀·해시·spot은 조회 때 다시 읽으며 이 객체보다 오래 살아야 한다.
    RawSquidNeighbors(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames);
    // 004b23e0(source, 0)의 생성 직후 +0x34 값이다. 연결 이웃이 없으면 0이다.
    // 내부 발자국이면 탐색하지 않는다. 일반 해시의 단계/y/x/next 순서에서 첫 통과 후보만 읽는다.
    Sid First(Sid source) const;
    // 끝 칸 변환과 탐색 필터가 동일한 타입 프레임 표를 읽도록 제공한다.
    const RiftTypeFrames& Frames(std::uint32_t type) const;
private:
    // 원본 float 발자국 사각형과 중심 좌표다. 소수 좌표를 정수 표면 스냅샷으로 바꾸지 않는다.
    struct Object {
        float left{},top{},right{},bottom{},centerX{},centerY{};
        std::uint32_t type{};
        FrameCode frame{};
    };
    // 지도 안 유한 좌표·양수 발자국을 확인한 뒤 원본 1..255 모서리 보정을 적용한다.
    Object ReadObject(Sid sid,bool readFrame) const;
    // source 발자국 spot의 AND와 후보 기준점 spot을 서로 구별한다.
    bool Interior(const Object& source) const;
    // 가로/세로 확장 교차의 XOR → 중심 방향 접합 → dead/내부 제외 순서다.
    bool Accept(Sid candidate,const Object& source) const;
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const std::uint8_t> spots_;
    std::vector<RiftTypeRecord> types_;
    std::vector<RiftTypeFrames> frames_;
};
}
