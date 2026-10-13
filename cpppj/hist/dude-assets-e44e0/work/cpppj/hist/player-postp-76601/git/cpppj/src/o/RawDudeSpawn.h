// 실제 Dude 생성과 최초 Pop의 작업 번호·population 등록을 연결한다.
#pragma once
#include "o/SquidFactory.h"
#include "o/SquidOwner.h"
#include "o/SquidFrame.h"

namespace netstorm::o {
// 원본 population 포인터/capacity/count와 생성 차단 전역을 공유하는 상태다.
struct DudeSpawnState {
    SquidPostPopList& population; // 생성 한도와 최초 Pop 등록이 같은 DWORD 목록을 사용한다.
    bool blocked{}; // 패치0055a4e8/CD005417c8이0이 아니면 생성하지 않는다.
    std::uint32_t dudeType{70}; // 원본00541170/CD0051ca5c의 실제 Dude 자산 번호다.
};
// 새 Dude의 실제 SHP/공간 Pop을 같은 월드에 연결하는 경계다.
struct DudeSpawnHooks {
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 실제 가상+90의 좌표 x/y/flags0 등록이다.
};
// 00450a20/CD004e8b30: 한도→차단→실제 factory→frame0→owner→Pop 순서를 복원한다.
class RawDudeSpawn {
public:
    // factory/owner와 후속 Pop은 같은 SID 풀을 사용해야 한다.
    RawDudeSpawn(SidPool& pool,SquidFactory& factory,SquidOwner& owner,DudeSpawnState& state,DudeSpawnHooks hooks);
    // 생성 조건이 거짓이면 SID0이며, 성공 시 실제 Pop을 완료한 새 Dude SID를 반환한다.
    Sid Spawn(float x,float y,std::uint32_t owner) const;
    // 최초 Pop 처리기와 같은 월드의 생성 연결을 검사한다.
    const SidPool& Pool() const;
private:
    SidPool& pool_; // 새 Dude와 population SID가 공유하는 raw 저장소다.
    SquidFactory& factory_; // 기존 실제 할당/생성자/postCreate 몸체다.
    SquidOwner& owner_; // 실제 가상+74가 직접 호출하는 공통 소유자 몸체다.
    DudeSpawnState& state_; // 같은 최초 Pop이 갱신하는 현재 population/차단 전역이다.
    DudeSpawnHooks hooks_; // 같은 SHP/해시/표시의 실제 Pop 연결이다.
};
// 최초 Dude Pop의 I 첫 프레임과 실제 공통 postPop을 연결하는 경계다.
struct DudePostPopHooks {
    std::function<void(Sid,std::int32_t,std::uint32_t)> setFrame; // 실제004acee0/CD004acbc0의 I 첫 프레임 쓰기다.
    std::function<void(Sid,std::uint32_t)> validateBase; // 같은 공통 후처리의 미연결 효과를 검사한다.
    std::function<void(Sid,std::uint32_t)> base; // 실제004b0d30/CD004ae180 공통 후처리 호출이다.
};
// 00450cd0/CD004e9230의 직접 전체 몸체다. 공유 Pop의 가상 분배 등록은 별도 연결한다.
class RawDudePostPop {
public:
    // frames의 현재 I Run과 population은 같은 월드 자료이며 객체보다 오래 살아야 한다.
    RawDudePostPop(SidPool& pool,std::span<const RiftTypeFrames> frames,DudeSpawnState& state,DudePostPopHooks hooks);
    // 실제 Dude 타입과 생성자 가상 표 둘 다 일치하는 경우에만 파생 분배가 가능하다.
    bool Handles(Sid sid) const;
    // 최초 쓰기 전에 현재 Run/목록과 실제 공통 후처리 연결을 확인한다.
    void Validate(Sid sid,std::uint32_t flags) const;
    // 최초 flags1은 task5101/한도 내 population 추가/I 프레임, 이후 항상 공통 postPop이다.
    void PostPop(Sid sid,std::uint32_t flags) const;
    // 공유 가상 분배와 같은 SID 풀인지 검사한다.
    const SidPool& Pool() const;
private:
    SidPool& pool_; // 최초 task WORD와 population SID가 공유하는 저장소다.
    std::span<const RiftTypeFrames> frames_; // 현재 TYPE+150과 같은 I 첫 프레임 Run이다.
    DudeSpawnState& state_; // 생성 한도와 같은 mutable DWORD 목록이다.
    DudePostPopHooks hooks_; // 실제 프레임/공통 후처리 연결이다.
};
// 직접 후처리를 기존 실제 Frame/Base에 연결하며 자동 가상 표 확장은 하지 않는다.
DudePostPopHooks MakeDudePostPopHooks(const SidPool& pool,SquidFrame& frame,SquidPostPop& base);
}
