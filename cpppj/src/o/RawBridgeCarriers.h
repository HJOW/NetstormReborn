//위험한 다리 위 사제/rainWalker의 현재 이동 조회와 같은 Graph의 가까운 표면 대피를 복원한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/RawPathRuntime.h"

namespace netstorm::o { //421c30의 이동체 확인과427e00의 실제 후보 선택을 함께 제공한다.
//큰 이동/Graph/진단 효과만 빌리고 후보 순서와 선택은 원문의 루프에서 계산한다.
struct BridgeCarrierHooks {
    std::function<std::uint32_t(Sid)> graphFor; //462bc0의 현재 번호이며254는462c80의 NULL이다.
    std::function<std::uint32_t(std::uint32_t)> movementType; //488fe0(type,0,0)의 현재 이동 프로세스 종류다.
    std::function<Sid(Sid,std::uint32_t)> movementForm; //4b1a90(parent,type,0,0)의 현재 Form이다.
    std::function<bool(Sid,RawPathPoint)> start; //489080(parent,x,y,0,0,0,0,0)의 실제 이동 시작이다.
    std::function<bool(RawPathPoint)> valid; //40e800의 현재 mapLimit/엄격한 내부 좌표 검사다.
    std::function<void(std::uint32_t)> assertion; //원문의 진단 보고가 돌아오면 같은 현재 상태로 계속한다.
    std::function<bool(std::uint32_t)> wontWalkOffEdge; //427da0의 현재 다섯 TYPE 비교이며 진단 관문에서만 조회한다.
    std::function<void(SquidSearchArea)> scan; //독립 관측에서만 실제 Begin 사각형을 기록한다. 게임에서는 비워 둔다.
};
//자체 finder 두 개의 수명을 분리하여 대피 중에도 바깥 칸 위 순회를 보존한다.
class RawBridgeCarriers {
public:
    RawBridgeCarriers(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        const bool& boss,const bool& checking,const std::uint32_t& priest,const std::uint32_t& rainWalker,
        const std::int32_t& avoidLife,BridgeCarrierHooks hooks); //모든 현재 참조와 callback 대상은 이 실행기보다 오래 살아야 한다.
    void Check(Sid bridge) const; //실제4202f0(flags0) 순회에서 두 TYPE만 현재 이동이 없을 때 대피한다.
    void Retreat(Sid parent) const; //427e00의 권한/Graph→반경5→정상 표면/같은Graph/거리→실제 이동 순서다.
private:
    RawPathPoint Position(Sid sid) const; //효과 뒤에도 현재 float 두 개를 다시 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; //현재 BYTE TYPE를 같은 물리 표에서 조회한다.
    std::uint32_t Graph(Sid sid) const; //Graph0과 NULL을 구별하는 비교 식별값을 만든다.
    bool Dangerous(Sid sid) const; //422510의 bridge genus·비영 수명·현재7 경계를 읽는다.
    const SidPool& pool_; //현재 부모/후보/칸 위 이동체의 실제50바이트 풀이다.
    const SquidHash& hash_; //서로 중첩하는 두 일반 finder가 공유하는 실제4단계 해시다.
    std::span<const RiftTypeRecord> types_; //현재 flags1/flags2/발자국이다.
    const bool &boss_,&checking_; //원문의 조회 시점마다 현재 권한/진단 전역을 읽는다.
    const std::uint32_t &priest_,&rainWalker_; //005412d0/c8의 현재 두 TYPE다.
    const std::int32_t& avoidLife_; //0052f964의 현재 signed 경계이며 최초7이다.
    BridgeCarrierHooks hooks_; //현재 큰 이동 장치와 이미 복원된 Graph만 공급한다.
};
}
