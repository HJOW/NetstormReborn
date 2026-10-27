// 제네레이터 Meltdown의 대상 선택·요청·폭발 패킷·실제 피해 순서를 복원한다.
#pragma once
#include "o/RawBombTargets.h"
#include "o/RawFormPacket.h"

namespace netstorm::o { //10.78의41c290/420340..420919 전용 몸체다.
// 권한·현재 로컬 소유자·동맹·패킷 헤더를 수신 시점까지 빌리는 원본 전역 묶음이다.
struct MeltdownState {
    std::function<std::uint32_t()> boss,localOwner; // 현재540bc4/540c70을 효과 뒤에도 재조회한다.
    std::span<const std::uint32_t> alliances; // 현재595200의81DWORD이며 공격자*9+대상 순서다.
    FormPacketHeader request{0x76,1,6},effect{0x75,1,0}; // 실제 정적76h 헤더와544c8c의 최초BSS0 선언 길이다. 송신 observer가 실제 길이를 확정한다.
};
// 원문 몸체가 호출하는 검색·전송·월드·시청각 효과의 연결 경계다.
struct MeltdownHooks {
    std::function<bool(Sid)> hasHitPoints; // 실제4adc60이며 현재 HP 값과 다르다.
    std::function<void(std::uint32_t)> assertion; // Bomb.cpp664/6c3의 복귀 가능한 권한 진단이다.
    std::function<void(float,float,std::int32_t,BombTargetList&)> find; // 실제4203a0이며 Session은 같은 객체의 Find로 연결한다.
    std::function<void(std::int32_t,std::span<const std::uint8_t>)> send; // 실제4e2dc0(9,6BYTE)다.
    std::function<void(std::span<std::uint8_t>)> broadcast; // 실제4e2e30의220BYTE이며 원문 observer의 길이 쓰기를 허용한다.
    std::function<void(std::uint32_t,std::uint32_t,std::span<const std::uint8_t>)> receive; // 배포 뒤 현재 로컬owner·0·같은75h를 전달한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 현재 가상10이다. 반환값을 새 관문으로 사용하지 않는다.
    std::function<void(Sid,std::int32_t,std::uint32_t,Sid)> damage; // 비동맹 대상의 가상98(100,owner,parent)다.
    std::function<void(float,float)> ground; // 피해 전 캡처한 대상 중심으로470ac0을 호출한다.
    std::function<void(Sid,float,float)> edge; // 서버의421f70 다리 끝 칸 변환이다.
    std::function<void(float,float,std::uint32_t,float)> visual; // 세 위치의45a750(variant1,0.04)이다.
    std::function<void(std::string_view)> sound; // 전역 whirligigFire-1000.wav의 실제 재생이다.
};
// 제네레이터의 가상 명령과75/76 패킷을 같은 물리 세계에서 처리한다.
class RawMeltdown {
public:
    RawMeltdown(const SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,MeltdownState& state,MeltdownHooks hooks); // 같은 현재 풀/해시/188TYPE와 원본 전역을 빌린다.
    void Command(Sid parent,std::uint32_t code,const std::function<void(Sid,std::uint32_t)>& base) const; //41c290은 f8 요청 뒤에도 공통 부모를 호출하며 원문 반환값은 void다.
    bool MayTarget(Sid target) const; //420340의 hard 다리 제외 또는 HP 보유/허용genus 자격이다.
    void Find(float x,float y,std::int32_t radius,BombTargetList& output) const; //4203a0의 엄격 거리 비교·원문 SID 구간·현재 목록 한도다.
    void Apply(float x,float y,std::int32_t radius,std::uint32_t owner,Sid target,Sid parent) const; //4204f0의 피해/다리 삭제·끝 칸 순서다.
    void ReceiveRequest(std::span<const std::uint8_t> packet) const; //4207a0은 dead만 거르고 검색 결과 전체를75h에 복사한 뒤 배포·로컬효과·부모삭제한다.
    void ReceiveEffect(std::span<const std::uint8_t> packet) const; //4205f0의 세 시각효과·소리·현재 대상 목록 적용이다.
    static void FinalizePacket(std::span<std::uint8_t> packet,bool writing); //40b150의 실제 가변 선언 길이 확정이다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset,std::size_t width=4) const; // 현재 비정렬 raw 필드를 지정 폭으로 읽는다.
    const RiftTypeRecord& Type(Sid sid) const; // 효과 뒤 현재 TYPE를 다시 읽는다.
    std::array<float,2> Center(Sid sid) const; //4adea0의 현재 좌표−발자국 절반이다.
    const SidPool& pool_;const SquidHash& hash_; // 같은 세계의 유일한 물리 객체/공간 저장소다.
    std::span<const RiftTypeRecord> types_;std::span<const RiftTypeFrames> frames_; // 현재 타입/프레임 코드다.
    MeltdownState& state_;MeltdownHooks hooks_; // 선택 시점의 실제 전역과 큰 외부 효과다.
};
}
