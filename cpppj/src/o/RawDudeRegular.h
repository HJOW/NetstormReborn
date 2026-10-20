// Dude의 실제 정기 분배·보행·복귀·드롭·낙하를 같은 현재 월드에 연결한다.
#pragma once
#include "o/RawDudeBehavior.h"
#include "o/RawPathRuntime.h"
#include "o/SpStore.h"

namespace netstorm::o {
inline constexpr std::size_t kDudePopulationCapacity=30; //4fc9b0→44fb20의 실제 push1e 초기 capacity다.
// 원본 static initializer의30칸/count0을 빈 Session 목록에 준비한다. 배열의 미사용 heap 값은 게임 상태가 아니다.
void InitializeDudePopulation(SquidPostPopList& population);
// 현재 검사 모드·공급자 active 목록·보행 순환 전역을 복사하지 않는다.
struct DudeRegularState {
    const bool& checking; //005e4794의 현재 진단 모드다.
    const SquidPostPopList& providers; //00592f84/88/8c의 기존 에너지 공급자 목록이다.
    std::int32_t& providerIndex; //0055a4fc의 같은 signed 순환 값이다.
    std::uint32_t pollEvent{0x1a2b},pathType{24}; //005326c0/005410d0의 현재 사건·경로 종류다.
};
// 현재 몸체의 효과만 요구한다. 도달하지 않은 효과는 빈 성공으로 대체하지 않는다.
struct DudeRegularHooks {
    std::function<bool(Sid)> checkSpace; //450e80의 같은 지면/Fall/이동 제거 몸체다.
    std::function<Sid(Sid,std::uint32_t)> findPath; //4b1a90(parent,24,0,false)의 현재 종속 조회다.
    std::function<void(Sid,std::uint32_t,float)> task; //450670의 실제 Regular 재예약이다.
    std::function<Sid(Sid)> nearestResidence; //48fd30의 현재 소유자/위치와 같은 거주 구역 목록이다.
    std::function<RawPathPoint(Sid)> entrance; //4505a0의 현재 목표 발자국 진입점이다.
    std::function<bool(Sid,RawPathPoint,bool)> move; //450760의 현재 속도·실제24 경로 시작이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; //4acee0의 실제 가상 공간/프레임 변경이다.
    std::function<void(Sid,std::uint32_t)> unpop,destroy; //현재 부모의 실제+48/+10이다.
    std::function<void(Sid,RawPathPoint,std::uint32_t)> pop; //현재 부모+90의 좌표/flags다.
    std::function<void(std::uint32_t)> assertion; //복귀하는 진단의 행 번호이며 Session은 예외로 진단한다.
};
// 10.78의450fa0/451190/451210/451270/451340/4508b0을 같은 저장소로 실행한다.
class RawDudeRegular {
public:
    RawDudeRegular(SidPool& pool,std::span<const RiftTypeFrames> frames,GameRandom& random,DudeRegularState state,DudeRegularHooks hooks); // 같은 프레임/난수/현재 목록을 빌린다.
    bool Handles(Sid sid) const; //70/VT508cc8만 실제 부모 정기로 분배한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const; //poll·공통 fallback·지면·현재 경로·각 task 순서다.
    float Wander(Sid sid,std::int32_t count,bool quickly) const; //451190의 확률 관문·공급자 순환·실패 task5103이다.
    float Return(Sid sid,Sid path,std::int32_t count) const; //451210은5회부터 삭제하며 그 전에는 같은 거주 구역으로 이동한다.
    float Drop(Sid sid,Sid path,std::int32_t count) const; //451270은0회에 진입점으로 옮긴 뒤 원래 위치로 걸어 나온다.
    float Fall(Sid sid,std::int32_t count) const; //451340의 누적 y·현재 단계/RNG 프레임·100회 제한이다.
    bool MoveTo(Sid sid,Sid target,bool quickly) const; //4508b0의 SID0/목표 진단과 실제 진입점 이동이다.
    const SidPool& Pool() const; //현재 Session의 같은 풀 연결을 검증한다.
private:
    std::span<const std::uint8_t> Raw(Sid sid) const; //효과 뒤 현재 raw를 다시 읽으며 좌표 복사본을 보관하지 않는다.
    RawPathPoint Position(Sid sid) const; //현재 비정렬float 두 좌표다.
    std::int32_t Stage(Sid sid) const; //4ad710의 현재 signed side−A다.
    void Write(Sid sid,std::size_t offset,std::uint32_t value,std::size_t width) const; //원본 BYTE/WORD/DWORD 폭만 쓴다.
    SidPool& pool_; //부모·목표·실제 종속의 유일한 저장소다.
    std::span<const RiftTypeFrames> frames_; //현재 타입의 코드/Run 표다.
    GameRandom& random_; //게임 전역의 같은32비트 난수다.
    DudeRegularState state_; //같은 목록/순환/진단 전역의 참조다.
    DudeRegularHooks hooks_; //실제 선택한 하위 효과다.
};
}
