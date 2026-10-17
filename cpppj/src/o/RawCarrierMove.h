// 원본 운반체의 이동 요청427340과 서버 수신428570을 같은 현재 월드에 복원한다.
#pragma once
#include "o/RawFormPacket.h"
#include "o/RawPathStart.h"
#include <functional>

namespace netstorm::o {
// 현재 요청 헤더는 TYPE/에디터 상태와 별개인 원본52edec의 BYTE/BYTE/WORD다.
struct CarrierMoveState {
    FormPacketHeader request{0x40,1,18}; //40a850이 매번 읽는 원본 초기 헤더다.
};
// 현재 선택/경로/후보는 같은 실제 모듈을 빌리고 요청에서 서버를 직접 실행하지 않는다.
struct CarrierMoveHooks {
    std::function<Sid()> selected; //4d5430의 현재 선택 번호다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> select; //4d5c90(0,0,1)의 실제 선택 해제다.
    std::function<void(std::uint32_t)> assertion; //Carrier.cpp 571 진단 후 현재 genus를 다시 읽는다.
    std::function<bool(RawPathPoint,RawPathPoint)> connected; //488340의 같은 현재 지면 연결 검사다.
    std::function<std::uint32_t(std::uint32_t)> movement; //488fe0(type,0,0)의 실제 현재 경로 TYPE다.
    std::function<RawPathRuntime*(Sid,std::uint32_t)> path; //4b1a90→4ae2d0의 같은 현재 종속/Host 경로다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; //4e2dc0(9,packet)의 실제 송신이다.
    std::function<bool(Sid,std::uint32_t)> regular; //4afb40의 현재 Regular/Shared 사건 조회다.
    std::function<std::pair<Sid,bool>(Sid,RawPathPoint)> candidate; //426900의 실제 포획 후보다. bool은 수신 분기에서 읽지 않는다.
    std::function<Sid(RawPathPoint)> returnPlace; //263a0(x,y,0)의 같은 실제 지점 조회다.
    std::function<void(Sid,Sid)> target; //4894e0(parent,target,0,0)의 실제 접근점/경로 시작이다.
    std::function<Sid(RawPathPoint,std::uint32_t)> genusAt; //4b2070의 첫 현재 genus 객체다.
    std::function<RawPathRuntime*(Sid,Sid)> geyser; //427600의 실제 간헐천 경로다.
    std::function<bool(Sid,Sid)> altarAllowed; //4498e0(altar,carrier)의 실제 희생 조건이다.
    std::function<void(Sid,Sid)> sacrifice; //494eb0의 실제 의식 시작이다.
    std::function<RawPathRuntime*(RawPathStartRequest)> start; //489080의 같은 현재 생성/BFS/Host 시작이다.
};
// 호출자의 packet은 외부 효과 뒤 원본 위치에서 다시 읽는다. 소유권/정지 관문을 생략하지 않는다.
class RawCarrierMove {
public:
    RawCarrierMove(const SidPool& pool,std::span<const RiftTypeRecord> types,const bool& fort,
        CarrierMoveState& state,CarrierMoveHooks hooks); //PATCH10.78의 현재 참조와188 TYPE를 빌린다.
    void Request(Sid parent,RawPathPoint destination,std::uint32_t sacrifice) const; //선택 해제/현재 같은 목적지 억제/18BYTE 송신이다.
    void Receive(std::uint32_t owner,std::span<const std::uint8_t> packet) const; //실제40h 수신의 분기·현재 읽기 순서다.
    const SidPool& Pool() const; //다른 Session의 경로/풀 혼합을 확인한다.
private:
    std::uint32_t Genus(Sid parent) const; //효과 뒤 현재 raw TYPE의flags2를 다시 읽는다.
    const SidPool& pool_; //실제 운반체의 물리50BYTE 저장소다.
    std::span<const RiftTypeRecord> types_; //현재188 TYPE 표다.
    const bool& fort_; //594fb8의 현재fort 모드이며5c85a4 에디터와 구별한다.
    CarrierMoveState& state_; //현재 요청 헤더이며 송신 효과가 변경할 수 있다.
    CarrierMoveHooks hooks_; //도달한 실제 하위가 빠지면 그 위치에서 오류를 보인다.
};
}
