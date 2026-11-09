// 거주 구역의 현재 주민/장식/지면 테마와 Fort 전용 Dude 생성 부모를 공유한다.
#pragma once
#include "o/RawSquidFinder.h"
#include "o/SpStore.h"
#include "o/SquidPostPop.h"
#include <functional>
namespace netstorm::o { // 같은 실제 Pool/Hash/타입/신전 목록을 직접 빌린다.
struct ResidenceRegularHooks { // 선택한 원문 하위만 요구하고 효과 중 현재 상태 변경을 허용한다.
    std::function<GameRandom&()> random; // 선택된 난수 호출 위치에서 같은 현재 owner를 조회한다.
    std::function<void(Sid)> notify,dude; // 주민 통지와 Fort 전용 생성 부모다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> frame; // 실제 가상 프레임 지정기다.
    std::function<Sid(float,float)> surface; // 현재 지면 finder다.
    std::function<void(Sid,std::uint32_t)> owner; // 현재 가상74의 전체 본문이다.
    std::function<std::int32_t(Sid)> element; // 일치한 신전의 현재 TYPE+98이다.
    std::function<std::uint32_t()> themeMode,decorationType; // 현재 설정과005411f0을 선택 시 읽는다.
    std::function<void(std::int32_t)> recordScan; // signed 최대 조사 DWORD를 현재값과 비교해 기록한다.
};
float RunResidenceRegular(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,const SquidPostPopList& temples,Sid sid,std::uint32_t event,std::uint32_t count,float payload,const ResidenceRegularHooks& hooks); // PATCH49a4e0의 비가상 기본 반환까지 보존한다.
struct ResidenceDudeHooks { // 실제450a90이 선택한 기존 생성/진입점/예약 본문이다.
    std::function<std::array<float,2>(Sid)> entrance; // 실제4505a0이다.
    std::function<Sid(float,float,std::uint32_t)> spawn; // 실제450a20이다.
    std::function<void(Sid,std::uint32_t,float)> task; // 실제450670이다.
    std::function<void()> assertion; // srcSid 진단은 정상 복귀를 허용한다.
};
void RunResidenceDudeSpawn(const SidPool& pool,const bool& fort,const SquidPostPopList& population,const SquidPostPopList& providers,Sid source,const ResidenceDudeHooks& hooks); // Fort/현재 공급원 수와 Dude 수의 선행 관문 뒤 현재 raw/owner와 새 Dude 상태를 읽는다.
}
