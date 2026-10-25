// 원본10.78 Cursor.cpp00445b30의 실제 미리보기 생성/재사용과 다리 연결 위치 보정이다.
#pragma once
#include "client/RawCursorPlacement.h"
#include "o/RawBridgeConnect.h"
#include "o/RawSquidHitPoints.h"
#include "o/SquidFactory.h"
#include "o/SquidReward.h"

namespace netstorm::client {
// 생성/가상 효과와 Canon 순회만 공급한다. 위치/프레임/임시 목록 계산은 실제 몸체가 수행한다.
struct CursorPreviewHooks {
    std::function<void()> queryCursor,prepareCursor; // 실제445440/4458e0이며 생성 앞/뒤에 한 번씩 호출한다.
    decltype(CursorPlacementHooks::walk) walk; // 같은425c20/425860의 현재 칸 공급자다.
    std::function<o::Sid(std::uint32_t,std::uint32_t)> create; // 4af530(type,2)다.
    std::function<void(o::Sid,std::uint32_t)> setWord,setOwner; // 4ac220의 하위WORD와 실제 가상+74다.
    std::function<bool(o::Sid)> hasHitPoints; // 공통4adc60의 현재 HP 유무다.
    std::function<void(o::Sid,std::int32_t)> setHitPoints; // 공통 비가상4adca0(sid,0)다.
    std::function<const o::RiftTypeFrames&(std::uint32_t)> frames; // 49a840의 현재 bridge TYPE 프레임 표다.
    std::function<std::int32_t(o::Sid,std::span<std::uint32_t>)> connect; // 4213b0(sid,현재목록꼬리,&count)다.
};
// 수명/내려놓기/배치와 같은 실제 풀·전역에 미리보기를 만든다. 공간 Pop은 호출하지 않는다.
class RawCursorPreviews {
public:
    // 풀/타입/상태/프레임/효과는 이 객체보다 오래 살아야 한다. 현재 복원은10.78만 허용한다.
    RawCursorPreviews(o::SidPool& pool,std::span<const o::RiftTypeRecord> types,CursorDropState& drop,
        CursorLifecycleState& lifecycle,const o::BridgeConnectState& surfaces,CursorPreviewHooks hooks);
    // 445b30: 첫 SID 유무로 생성/재사용을 고르고 Canon·연결 객체·섬 받침 둘을 처리한 뒤 화면을 준비한다.
    void Create() const;
    // 연결기에서 같은 실제 풀인지 확인한다.
    const o::SidPool& Pool() const;
private:
    // 현재 abstract TYPE 포인터의 범위를 확인한다.
    const o::RiftTypeRecord& Type() const;
    // 목록에 새 SID를 쓰고 현재 signed 개수를 증가시킨다.30칸 밖 호스트 쓰기는 거부한다.
    void Append(o::Sid sid) const;
    // 현재 슬롯의 float 위치와 x*16+0.5/y*11+0.5 절삭 WORD 화면 캐시를 쓴다.
    void Position(o::Sid sid,float x,float y) const;
    // 유효 목록 인덱스의 원래 DWORD를 읽고 실제 SID 변환은 소비할 때만 한다.
    std::uint32_t At(std::int32_t index) const;
    o::SidPool& pool_; // 모든 미리보기/연결 객체를 같은 raw 저장소에서 읽고 쓴다.
    std::span<const o::RiftTypeRecord> types_; // 메뉴/생성/위치 판정과 같은 현재 TYPE 표다.
    CursorDropState& drop_; // 원래 Form/타입/인자/소유자/위치의 공유 전역이다.
    CursorLifecycleState& lifecycle_; // 실제30DWORD/현재 개수/abstract TYPE의 공유 전역이다.
    const o::BridgeConnectState& surfaces_; // 같은 bridge/island/stalag 타입 전역이다.
    CursorPreviewHooks hooks_; // 복원 계산을 대신하지 않는 실제 순회/가상 효과다.
};
// Take/취소가 사용하는 수명 몸체의 실제 미리보기 생성 경계를 연결한다.
void BindCursorPreviews(CursorLifecycleHooks& lifecycle,RawCursorPreviews& previews);
// 같은 실제 위치 판정/준비와 기존 Canon 공급자를 연결한다. 위치 상태의 사본을 만들지 않는다.
void BindCursorPreviewPlacement(CursorPreviewHooks& hooks,const o::SidPool& pool,RawCursorPlacement& placement,
    const CursorPlacementHooks& placementHooks);
// 실제 Factory/HP/연결 객체와 현재 프레임 표를 연결한다. 파생 가상 Owner·표면 진단은 호출자가 공급한다.
void BindCursorPreviewWorld(CursorPreviewHooks& hooks,o::SidPool& pool,o::SquidFactory& factory,
    const o::SquidReward& reward,o::RawBridgeConnect& connect,std::span<const o::PriestPlainCanonType> frames,
    const o::BridgeConnectState& surfaces,std::function<void(o::Sid)> notifySurface);
}
