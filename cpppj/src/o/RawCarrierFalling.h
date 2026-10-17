// 캐리어의 위험한 다리 판정/가속 낙하/포획 사제 해방을 같은 현재 상태에서 처리한다.
#pragma once
#include "o/RawContainedFinder.h"
#include "o/RiftType.h"
#include <functional>
namespace netstorm::o {
// 생성/탐색/가상 효과와 진단만 기존 월드에 연결한다.
struct CarrierFallingHooks {
    std::function<Sid(Sid,std::uint32_t,std::uint32_t,std::uint32_t)> find; // 4ac9f0의 mask/kind/index 검색이다.
    std::function<Sid(Sid,std::uint32_t)> findKind; // 4b26f0(parent,kind,0,0)의 첫 내용물 검색이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 현재 사제 TYPE으로 4af530(type,0)을 실행한다.
    std::function<void(Sid,std::uint32_t)> setWord,setOwner; // 4ac220과 현재 가상74다.
    std::function<void(Sid,std::uint32_t)> destroy,unpop; // 현재 가상10/48의 실제 효과다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 캡처한 좌표로 현재 가상90을 실행한다.
    std::function<bool(Sid)> fallWalker; // 현재 가상c8이며 해방에서는 반환값을 사용하지 않는다.
    std::function<void(std::uint32_t)> assertion; // Carrier.cpp의 3bf/3d0 진단이며 복귀하면 현재 상태를 계속 읽는다.
};
// PATCH4225b0/426ef0/426c80/40e800을 복원한다. 입력 저장소는 이 객체보다 오래 살아야 한다.
class RawCarrierFalling {
public:
    RawCarrierFalling(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint16_t> surface,std::span<const std::uint8_t> players,const bool& boss,const bool& checking,const std::uint32_t& priestType,const std::int32_t& mapLimit,CarrierFallingHooks hooks); // 같은 풀/188TYPE/65536hash0/Player 기록을 받는다.
    std::uint32_t NeedsEscape(float x,float y) const; // 거의 올림의 signed 좌표 범위 뒤 현재 다리 framecode&20을 반환한다.
    float Fall(Sid parent,std::uint32_t count) const; // signed count와 권한에 따라 낙하/사제 해방/삭제를 실행한다.
    void ReleasePriest(Sid parent) const; // 현재 내용물 WORD와 이전 소유자의 사제 위치로 실제 사제를 복원한다.
    bool ValidPoint(float x,float y) const; // 40e800의 엄격한 내부 좌표/단정도 mapLimit 비교다.
    const SidPool& Pool() const; // 다른 월드의 풀 혼합을 검사한다.
private:
    SidPool& pool_; // 캐리어/내용물/새 사제의 동일 물리 풀이다.
    std::span<const RiftTypeRecord> types_; // 현재 TYPE+ec genus다.
    std::span<const RiftTypeFrames> frames_; // 현재 TYPE+124의 프레임 코드다.
    std::span<const std::uint16_t> surface_; // 실제 hash0 표면 머리다.
    std::span<const std::uint8_t> players_; // Player당 b4바이트의 현재 기록이다.
    const bool &boss_,&checking_; // 현재 권한/진단 전역이며 콜백 뒤 다시 읽는다.
    const std::uint32_t& priestType_; // 현재 사제 종류이며 검색 뒤 생성 시 다시 읽는다.
    const std::int32_t& mapLimit_; // 현재 signed 좌표 제한이다.
    CarrierFallingHooks hooks_; // 실제 큰 하위 효과다.
};
}
