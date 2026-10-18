// 원본 공통 비행 Frame과 맨 오 워 초기화/애니메이션을 같은 raw 필드로 실행한다.
#pragma once
#include "o/RawFlyerProcess.h"
#include "o/RawSunAviaryProcess.h"

namespace netstorm::o {
// 실제 Frame이 매 위치에서 다시 읽는 장기 전역이다.
struct FlyerFrameGlobals {
    const bool& server; // 00540bc0의 현재 서버다.
    const bool& boss; // 00540bc4의 삭제/권한이며 server와 구별한다.
    const bool& checking; // 005e4794의 원본 진단이다.
    const double& now; // 0055b4d0의 현재 게임 시각이다.
    const std::uint32_t& frame; // 0055b4a8의 현재 프레임 번호다.
    const std::uint32_t& sunType; // 0054117c의 현재 훨리기그73이다.
    const std::uint32_t& windType; // 00541280의 현재 더스트 데빌138이다.
    const std::uint32_t& rainType; // 00541298의 현재 맨 오 워144이다.
    const std::uint32_t& sunAviaryProcess; // 005410f0의 현재 타입32다.
    const std::uint32_t& soundProcess; // 0054112c의 현재 소리 프로세스 타입47이며 실제 음향 장부와 공유한다.
};
// TYPE+44와+70을 같은 번호로 공급한다.
struct FlyerFrameType {
    float range{}; // 현재 칸 단위 사거리다.
    std::uint32_t sound{}; // 실제 firingSound 토큰이다.
};
// 원문의 큰 하위 함수와 가상 호출만 외부 경계로 연결한다.
struct FlyerFrameHooks {
    std::function<void(std::uint32_t)> assertion; // 원본 Flyer.cpp 진단 줄 번호다.
    std::function<bool(FlyerProcessFields&)> robust; // 실제 Baseprocess.isRobust의 진단 경계다.
    std::function<bool(Sid,bool)> sidRobust; // 004ac140의 유효 SID/선택 진단이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 현재 부모의 가상+10이다.
    std::function<void(FlyerProcessFields&)> finish; // 현재 outer+14의 비행 종료/부모 삭제 몸체다.
    std::function<std::int32_t(FlyerProcessFields&)> otherFrame; // 다른 비행 종류의 실제 outer+8이다.
    std::function<std::array<float,2>(Sid,bool)> offset; // 현재 부모의+cc 또는 홈일 때+d0이다.
    std::function<std::array<float,2>(Sid,Sid)> scatter; // 0045bb80(target,현재부모)의 포위 위치이며 Form과 구별한다.
    std::function<void(Sid)> refreshTime; // 현재 부모의 실제004d5000이다.
    std::function<void(Sid,Sid)> hit; // 현재 부모/표적의 실제004d4f00이다.
    std::function<void(Sid,std::uint32_t)> removeSound; // 004b1a90(parent,type,0,1)의 실제 제거다.
    std::function<void(Sid,std::uint32_t)> unpop; // 현재 가상+48이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 현재 가상+90이다.
    std::function<void(Sid,std::uint32_t)> repop; // 현재 가상+4c이다.
    std::function<void(Sid)> movedCell; // 정수 칸이 바뀐 뒤 가상+d4다.
    std::function<bool(Sid)> checkForms; // 004ad730의 현재 종속 진단이다.
    std::function<bool(Sid,Sid)> mayTarget; // 0045ae60의 현재 발사 문맥→0046af80이다.
    std::function<std::int32_t(Sid,Sid)> closerCount; // 0045b840(target,부모)의 이미 가까운 플라이어 수다.
    std::function<std::int32_t(Sid)> capacity; // 0045b9c0의 표적당1/3 자리다.
    std::function<void(FlyerProcessFields&)> findTarget; // 실제 outer+10의0045c4b0이다.
    std::function<void(Sid,Sid)> addDepend; // 004b15f0(target,현재Form)이다.
    std::function<void(Sid,Sid)> removeDepend; // 004ae780(현재Form,target,0)이다.
    std::function<void(FlyerProcessFields&)> refreshForm; // 실제0041c010의 동일 Host 갱신이다.
    std::function<double(FlyerProcessFields&)> returnDelay; // 현재 outer+c의 실제 반환 지연이다.
    std::function<void(Sid,std::uint32_t)> command; // 004e2e30의 비행 명령2/3이다.
    std::function<SunAviaryProcessFields&(Sid,std::uint32_t)> sunProcess; // 실제 첫 타입32 Form의 같은 이미지다.
    std::function<void(SunAviaryProcessFields&)> refreshSun; // 같은 훨리베이스의 실제 Form 갱신이다.
    std::function<void(Sid,std::uint32_t,std::uint32_t)> homeNotice; // 살아 있는 홈의 가상+34(0,0)이다.
    std::function<std::int32_t(Sid)> runCount; // 004af110의 현재 프레임 글자 개수다.
    std::function<void(Sid,Sid,std::uint32_t)> launch; // 맨 오 워의00472690(flyer,target,0)이다.
    std::function<void(float,float,std::uint32_t)> sound; // 실제004a9c30의 한 번 재생이다.
    std::function<void(FlyerProcessFields&)> sunInitialize; // 정확한 훨리기그 전용표의45bd00 초기화다.
    std::function<void(FlyerProcessFields&,std::uint32_t,std::uint32_t,std::uint32_t)> sunNotify; // 정확한 전용표의45bd60 통지다.
};
// 제어와 수치를 직접 복원하며 표적 검색/의존/장치의 큰 몸체는 명시적으로 요구한다.
class RawFlyerFrame {
public:
    RawFlyerFrame(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const FlyerFrameType> details,
        std::span<std::uint32_t> soundFrames,FlyerFrameGlobals globals,FlyerFrameHooks hooks); // PATCH와 동일 번호/장기 저장소를 확인한다.
    void Initialize(FlyerProcessFields& fields,bool rain) const; // 0045bf00/0045b800의 기본/맨 오 워 첫 상태다.
    void Frame(FlyerProcessFields& fields) const; // 0045d630 수명 wrapper 뒤 같은 공통 Frame이다.
    void BaseFrame(FlyerProcessFields& fields) const; // 0045c9d0의 이동/공격/표적/상태 제어다.
    std::int32_t RainFrame(FlyerProcessFields& fields) const; // 0045bf50의 현재 프레임/발사/타입별 중복 소리 억제다.
    void ClearTarget(FlyerProcessFields& fields) const; // 0045b6f0의 의존 해제/현재 dirty/WORD/통지/전송 순서다.
    void Notify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const; // 0045b760의 실제 선택된 사건만 처리한다.
    void BaseNotify(FlyerProcessFields& fields,std::uint32_t message,std::uint32_t info1,std::uint32_t info2) const; // 전용 통지가 다른 사건을 실제45b760으로 넘길 때 쓴다.
    void ParentDying(FlyerProcessFields& fields) const; // 0045b7a0의 현재 홈 생존/타입/통지 관문이다.
    bool CheckForms(Sid parent) const; // 004ad730의 자산/종속 타입·free/void 체인 검사다.
    const SidPool& Pool() const; // 다른 월드의 Frame 연결을 거부한다.
private:
    std::array<float,2> Center(Sid sid) const; // 실제004adea0의 현재 발자국 중심이다.
    bool ReturnHome(FlyerProcessFields& fields,Sid parent,Sid home) const; // 착륙/재생산 뒤 계속하면true, 종료 가상 호출 뒤 전체 Frame 반환이면false다.
    SidPool& pool_; // 비행체/홈/그림자와 공유하는 단일 풀이다.
    std::vector<RiftTypeRecord> types_; // 현재 불변 TYPE의 발자국 표다.
    std::vector<FlyerFrameType> details_; // 같은 번호의 현재 사거리/사운드다.
    std::span<std::uint32_t> soundFrames_; // TYPE+1f0과 같은 단일 프레임별 재생 장부다.
    FlyerFrameGlobals globals_; // 효과 뒤 현재 값을 다시 읽는 전역 참조다.
    FlyerFrameHooks hooks_; // 원본에서 선택된 실제 하위 경계다.
};
}
