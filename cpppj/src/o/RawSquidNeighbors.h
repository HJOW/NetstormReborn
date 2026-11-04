// SquidFinder(객체 번호, flags)의 연결 이웃을 실제 raw 풀·일반 해시에서 찾는다.
// 원본: 004b23e0/004b1d70/004b1e80/004b1e70 ↔ CD 004ebad0/004ebf30/004ec040/004ec030.
// 끝 칸 변환(flag 0)과 다리/섬 연결(flag 4)의 일반 해시 순회와 flag 8의 표면 지도 순회를 구별한다.
#pragma once
#include "o/RawSquidFinder.h"

namespace netstorm::o {
// 탐색기 flags(원본 finder +0x48)의 비트다. 표면 지도 순회에서도 같은 연결/상태 필터를 사용한다.
namespace NeighborFlag {
inline constexpr std::uint32_t kNoJoint = 1;      // 프레임 접합 검사를 하지 않는다.
inline constexpr std::uint32_t kAnyType = 2;      // surface 타입이 아닌 후보도 받는다.
inline constexpr std::uint32_t kSkipAbstract = 4; // abstract(extra 비트 1) 후보를 제외한다.
inline constexpr std::uint32_t kSurfaceMap = 8; // 004b1c20의 현재 단계0 지도 y/x 순회를 선택한다.
inline constexpr std::uint32_t kSupported = kNoJoint | kAnyType | kSkipAbstract | kSurfaceMap; // 원문 네 비트의 지원 조합이다.
}
// 원본 float 발자국 사각형이다(004ade70 ↔ CD 004aeec0). 왼쪽/위는 1..255로 보정한 값이다.
struct SquidFootprint { /* left: 사각형의 왼쪽 경계. */ /* top: 사각형의 위쪽 경계. */ /* right: 사각형의 오른쪽 경계. */ /* bottom: 사각형의 아래쪽 경계. */ float left{},top{},right{},bottom{}; };
// 삭제 부모가 값으로 받은8DWORD 정보 중 탐색에 사용하는 위치/타입/프레임이다. 후보 raw 타입에는 적용하지 않는다.
struct SquidNeighborSource {
    float x{},y{}; // 삭제 호출 전 포착한 두 단정도 좌표다.
    std::uint32_t type{},frame{}; // 삭제 정보의 타입과 현재 프레임 인덱스이며 raw 슬롯 타입과 독립적이다.
};
// SquidFinder(객체 번호, flags)의 연결 이웃을 실제 raw 풀·일반 해시에서 찾는다.
class RawSquidNeighbors {
public:
    // 타입·프레임 표는 복사한다. 풀·해시·spot은 조회 때 다시 읽으며 이 객체보다 오래 살아야 한다.
    RawSquidNeighbors(const SidPool& pool,const SquidHash& hash,std::span<const std::uint8_t> spots,
        std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames);
    // 004b23e0(source, 0)의 생성 직후 +0x34 값이다. 연결 이웃이 없으면 0이다.
    // 내부 발자국이면 탐색하지 않는다. 일반 해시의 단계/y/x/next 순서에서 첫 통과 후보만 읽는다.
    Sid First(Sid source) const;
    SquidNeighborSource Source(Sid sid) const; // raw 입력을 값으로 포착한다. 진단 복귀 뒤 원본 인자 정보가 바뀌지 않게 한다.
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
    void BindCurrentTypes(std::span<const RiftTypeRecord> types); //실제PATCH World의현재TYPE를빌리고빈span은독립소유사본으로돌린다.
    const SidPool& Pool() const;
    const SquidHash& Hash() const;
    std::span<const RiftTypeRecord> Types() const;
private:
    friend class RawSquidNeighborWalk;
    // 원본 float 발자국 사각형과 중심 좌표다. 소수 좌표를 정수 표면 스냅샷으로 바꾸지 않는다.
    struct Object {
        // left: 사각형의 왼쪽 경계.
        // top: 사각형의 위쪽 경계.
        // right: 사각형의 오른쪽 경계.
        // bottom: 사각형의 아래쪽 경계.
        // centerX: 중심 X.
        // centerY: 중심 Y.
        float left{},top{},right{},bottom{},centerX{},centerY{};
        // type: 타입 표에서 객체 종류를 찾는 번호.
        std::uint32_t type{};
        // frame: 사용할 그림 프레임 또는 프레임 처리 자료.
        FrameCode frame{};
    };
    // 유한 임시 좌표·양수 발자국을 확인한 뒤 원본 점 유효성/1..255 모서리 보정을 적용한다.
    Object ReadObject(Sid sid,bool readFrame) const;
    // 삭제 정보의 범위 밖 프레임은 접합의 두 번째 코드가 실제 필요한 지점까지 판독을 늦춘다.
    Object ReadObject(const SquidNeighborSource& source,bool readFrame,bool lazyFrame) const;
    // source 발자국 spot의 AND와 후보 기준점 spot을 서로 구별한다.
    bool Interior(const Object& source) const;
    // 표면 타입 → 가로/세로 확장 교차의 XOR → 중심 방향 접합 → dead/abstract/내부 제외 순서다.
    bool Accept(Sid candidate,const Object& source,std::uint32_t flags) const;
    // pool_: 객체 슬롯의 생성·조회·반납에 사용하는 SID 풀.
    const SidPool& pool_;
    // hash_: 좌표별 객체 후보를 찾는 공간 해시.
    const SquidHash& hash_;
    // spots_: 칸별 점유·통행 비트를 읽는 spot 지도.
    std::span<const std::uint8_t> spots_;
    // types_: 타입 번호로 속성·플래그를 조회하는 타입 표.
    std::vector<RiftTypeRecord> types_;
    std::span<const RiftTypeRecord> currentTypes_; //같은World의주소고정TYPE배열이며프레임코드소유권과구별한다.
    const RiftTypeRecord& CurrentType(std::size_t type) const; //기존at의물리범위를지키며현재표면플래그를조회한다.
    // frames_: 타입별 프레임 자료.
    std::vector<RiftTypeFrames> frames_;
};
// 004b23e0(source, flags)로 만든 탐색기의 순회다. 생성 직후 Current()가 첫 이웃(원본 +0x34)이다.
// source의 발자국·타입·프레임·중심은 생성 때 한 번 읽는다. 후보의 필드와 해시 체인은 Next 때마다 다시 읽으므로
// 순회 도중 호출자가 객체를 등록/삭제하면 원본처럼 그 변경이 뒤의 결과에 반영된다.
class RawSquidNeighborWalk {
public:
    // neighbors는 이 객체보다 오래 살아야 한다. flags는 NeighborFlag의 조합(0~15)이다.
    RawSquidNeighborWalk(const RawSquidNeighbors& neighbors,Sid source,std::uint32_t flags);
    // 원문004b24c0/CD004ebbe0의 값 인자를 사용하며 원천만 치환한다. lazyFrame은 삭제 정보의 지연 접합 판독이다.
    RawSquidNeighborWalk(const RawSquidNeighbors& neighbors,const SquidNeighborSource& source,std::uint32_t flags,bool lazyFrame=false);
    // 현재 이웃 번호다. 0이면 끝이다.
    Sid Current() const;
    // 004b1e70 ↔ CD 004ec030: flag8이면 현재 표면 지도, 아니면 일반 Next와 연결 필터를 사용한다. 끝난 뒤에는0을 유지한다.
    Sid Next();
private:
    // finder_: 검색기.
    RawSquidFinder finder_;
    std::function<Sid()> surfaceNext_; // flags8의 원천 snapshot·y/x 커서·signed short 중복 캐시를 값으로 보관하는 현재 지도 순회다.
    // current_: 현재 처리 위치 또는 현재 선택 상태.
    Sid current_{};
};
}
