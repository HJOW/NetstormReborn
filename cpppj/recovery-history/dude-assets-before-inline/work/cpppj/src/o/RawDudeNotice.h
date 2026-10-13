// 원본 충돌 주변 Dude의 현재task/Regular·거주지 선택·대피 요청을 같은 raw 월드에 연결한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/SquidPostPop.h"
#include "o/SquidProcess.h"
#include "o/RiftType.h"
#include <optional>

namespace netstorm::o {
// TYPE+e0의 현재속도는 경로요청동안2배가 되고 이후 원래값의 정수절삭으로 복원된다.
struct DudeNoticeTypeData {
    float speed{}; // 현재월드의 mutable 이동속도다. 읽기용 타입 표와 별도 공유한다.
};
// 같은 실제 Host 시계와 원본 프로세스/표면 분류 전역이다.
struct DudeNoticeState {
    SquidProcessState& clock; // 피해 상태변경의 현재double 게임 시각이다.
    std::uint32_t burningProcessType{14},pathProcessType{24},bridgeType{82}; // 실제typed Form 조회와task 표면통지 종류다.
    std::uint32_t raisedGenusMask{0x50444200}; // 표적 진입점에서높이를보정하는 실제005424b8/CD005395cc 마스크다.
};
// 실제하위생성/현재경로조회 경계다. 누락된 Dude/Path 생성은 원본몸체의 호출시점에서 거부한다.
struct DudeNoticeHooks {
    std::function<RegularProcess*(Sid)> taskRegular; // 실제Host의(maskfff0,value5100) 기존Regular 조회다.
    std::function<RegularProcess*(Sid)> addTaskRegular; // 실제payload5/event5107/flags50 Regular 생성이다.
    std::function<void(Sid)> createBurning; // 원본new90/Burning14 모드0 생성 경계다.
    std::function<std::optional<std::array<float,2>>(Sid)> pathEndpoint; // 현재type24 경로의마지막꼭짓점 조회다.
    std::function<bool(Sid,float,float)> startPath; // 실제DudePath24 생성 요청이며 현재2배speed를읽어야한다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 경로성공/현재I단계에서 실제A구간 첫frame 쓰기다.
    std::function<void(Sid)> notifySurface; // bridge타입task WORD쓰기뒤 실제표면통지다.
};
// 실제 finder/상태/거리/현재frame·속도대입을복원하며 부모/프로세스의외부생성은엄격히분리한다.
class RawDudeNotice {
public:
    // types/frames/speeds/residences/clock은 같은월드의현재저장소이며이처리기보다오래살아야한다.
    RawDudeNotice(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,std::span<DudeNoticeTypeData> speeds,
        const SquidPostPopList& residences,DudeNoticeState state,DudeNoticeHooks hooks);
    // 00451470/CD004e8e70: 좌표−1 거의올림의4칸사각형에서genus400 후보를원본순서로처리한다.
    void Around(float x,float y) const;
    // 패치00450db0/CD인라인: dead/notice/fall은그대로두고현재Regular5107·프로세스·대피요청을수행한다.
    void Notice(Sid sid) const;
    // 0048fd30/CD00406ef0: 현재owner가맞는거주지중float 저장거리보다엄격히가까운첫후보다.
    Sid NearestResidence(Sid sid) const;
    // 004505a0/CD004e8a30의현재표적 발자국 진입점이다. 일반Gun 중심과구별한다.
    std::array<float,2> ResidencePoint(Sid sid) const;
    // 기존같은경로목적지면성공하며그외실제2배speed 요청후정수복원·현재I단계frame을처리한다.
    bool Move(Sid sid,std::array<float,2> destination,bool quickly=true) const;
    // 같은Host/공간어댑터의SID 풀혼용을막는다.
    const SidPool& Pool() const;
private:
    // 현재raw SID/타입/숫자입력과contained typed chain을매조회검사한다.
    std::span<const std::uint8_t> Slot(Sid sid) const;
    Sid Process(Sid sid,std::uint32_t type) const;
    static std::array<float,2> Snap(std::array<float,2> point);
    SidPool& pool_; // 현재객체/contained Form이공유하는원본SID 저장소다.
    const SquidHash& hash_; // Around의실제사각형 후보순회를수행하는네단계해시다.
    std::span<const RiftTypeRecord> types_; // 현재타입의genus/발자국/판본자료다.
    std::span<const RiftTypeFrames> frames_; // Move후현재단계와A구간을조회하는실제코드표다.
    std::span<DudeNoticeTypeData> speeds_; // 경로요청과공유하는현재속도저장소다.
    const SquidPostPopList& residences_; // RawAssetPostPop의현재거주지active 목록이다.
    DudeNoticeState state_; // 같은현재Host시계와원본타입/분류 전역참조다.
    DudeNoticeHooks hooks_; // 실제Regular/Burning/DudePath 생성및현재경로조회경계다.
};
// 기존Regular 조회/생성만같은실제Host에연결하고미복원Burning/Path factory 경계를유지한다.
DudeNoticeHooks MakeDudeNoticeHostHooks(const SidPool& pool,SquidProcessHost& host,DudeNoticeHooks hooks);
}
