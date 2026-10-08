// SquidFinder(객체 번호, flags)의 연결 이웃을 실제 raw 풀·일반 해시에서 찾는다.
// 원본: 004b23e0/004b1d70/004b1e80/004b1e70 ↔ CD 004ebad0/004ebf30/004ec040/004ec030.
// 끝 칸 변환(flag 0)과 다리/섬 연결(flag 4)은 이 경로를 사용하며 flag 8의 표면 지도 탐색으로 대신할 수 없다.
#pragma once
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 탐색기 flags(원본 finder +0x48)의 비트다. 8(표면 지도 순회, 004b1c20)은 이 클래스가 지원하지 않는다.
namespace NeighborFlag {
inline constexpr std::uint32_t kNoJoint = 1;      // 프레임 접합 검사를 하지 않는다.
inline constexpr std::uint32_t kAnyType = 2;      // surface 타입이 아닌 후보도 받는다.
inline constexpr std::uint32_t kSkipAbstract = 4; // abstract(extra 비트 1) 후보를 제외한다.
inline constexpr std::uint32_t kSupported = kNoJoint | kAnyType | kSkipAbstract;
}
// 원본 float 발자국 사각형이다(004ade70 ↔ CD 004aeec0). 왼쪽/위는 1..255로 보정한 값이다.
struct SquidFootprint { float left{},top{},right{},bottom{}; };
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
    // 004ade70(sid, 0) ↔ CD 004aeec0: 객체의 float 발자국 사각형이다.
    SquidFootprint Footprint(Sid sid) const;
    // 004adea0 ↔ CD 004aef30: 위치에서 발자국 크기의 절반을 뺀 중심이다(단정도로 저장한 값).
    std::array<float,2> Center(Sid sid) const;
    // 0041ce90(this=from, to, 1) ↔ CD 0043f620: from에서 to를 보는 네 방향(0 북, 2 동, 4 남, 6 서)이다.
    // 세로 차이는 단정도로 저장한 값을, 가로 차이는 x87 스택의 값을 쓴다. 크기가 같으면 세로다.
    static int Direction(float fromX,float fromY,float toX,float toY);
    // 풀·해시·타입 표를 같은 판본의 다른 계층에 연결할 때 쓴다.
    const SidPool& Pool() const;
    const SquidHash& Hash() const;
    std::span<const RiftTypeRecord> Types() const;
private:
    friend class RawSquidNeighborWalk;
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
    // 표면 타입 → 가로/세로 확장 교차의 XOR → 중심 방향 접합 → dead/abstract/내부 제외 순서다.
    bool Accept(Sid candidate,const Object& source,std::uint32_t flags) const;
    const SidPool& pool_;
    const SquidHash& hash_;
    std::span<const std::uint8_t> spots_;
    std::vector<RiftTypeRecord> types_;
    std::vector<RiftTypeFrames> frames_;
};
// 004b23e0(source, flags)로 만든 탐색기의 순회다. 생성 직후 Current()가 첫 이웃(원본 +0x34)이다.
// source의 발자국·타입·프레임·중심은 생성 때 한 번 읽는다. 후보의 필드와 해시 체인은 Next 때마다 다시 읽으므로
// 순회 도중 호출자가 객체를 등록/삭제하면 원본처럼 그 변경이 뒤의 결과에 반영된다.
class RawSquidNeighborWalk {
public:
    // neighbors는 이 객체보다 오래 살아야 한다. flags는 NeighborFlag의 조합(0~7)이다.
    RawSquidNeighborWalk(const RawSquidNeighbors& neighbors,Sid source,std::uint32_t flags);
    // 현재 이웃 번호다. 0이면 끝이다.
    Sid Current() const;
    // 004b1e70 ↔ CD 004ec030: flag 8이 없으면 일반 Next(004b1810)에 연결 필터를 건 것이다. 끝난 뒤에는 0을 유지한다.
    Sid Next();
private:
    RawSquidFinder finder_;
    Sid current_{};
};
}
