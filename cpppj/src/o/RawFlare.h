// 연구·업그레이드 효과가 사용하는 원본 플레어 생성과 Regular 수명을 복원한다.
#pragma once
#include "o/SquidProcess.h"
#include "o/SpStore.h"
#include <stdexcept>
#include <utility>

namespace netstorm::o {
// 동일 월드의 실제 억제/로딩/현재 효과 타입 전역을 빌린다.
struct FlareState {
    const double* suppressedTime; // 독립 typed 경로가 빌리는005c85f0 주소이며 Native는 현재 장치를 선택한다.
    const std::uint32_t& loadingDepth; // 005c89b8의 현재 로딩 깊이다.
    const std::uint32_t& type; // 00541200의 실제 flare106 타입이다.
    bool native{}; // 실제 Native는 원문에 없는 TYPE/할당 사전검사를 추가하지 않는다.
    std::function<double()> currentSuppression; // Native의 같은005c85f0 owner를 생성 관문에서만 읽는다.
    FlareState(const double& time,const std::uint32_t& loading,const std::uint32_t& kind):suppressedTime(&time),loadingDepth(loading),type(kind) {} // 기존 typed 참조 계약을 유지한다.
    FlareState(std::function<double()> time,const std::uint32_t& loading,const std::uint32_t& kind):suppressedTime(nullptr),loadingDepth(loading),type(kind),native(true),currentSuppression(std::move(time)) {} // Native 생성 전역은 선택한 시점에 조회한다.
    double SuppressedTime() const { if (suppressedTime) return *suppressedTime;const auto get=currentSuppression;if (!get) throw std::logic_error("플레어의 현재 억제 시각 미연결");return get(); } // 실제 생성 관문 이외에는 장치를 요구하지 않는다.
};
// 생성/표시/수명/예약은 현재 같은 월드의 실제 소비자에 연결한다.
struct FlareHooks {
    std::function<std::int32_t()> processCount; // 471ab0의 signed Kernel 개수다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // 4af530의 현재 TYPE/클라이언트 할당이다.
    std::function<std::array<std::int16_t,2>(Sid)> sourceHotspot; // 419850이 반환한 SHP 추가 헤더+1c/+1e다. 폭/높이가 아니다.
    std::function<void(Sid,std::uint32_t)> owner; // 4ac220의 실제 소유자 지정이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 가상+90의 실제 공간·표시 등록이다.
    std::function<bool()> reserveRegular; // 각 new40의 독립 성공 여부다.
    std::function<void(Sid,std::uint32_t,float)> addRegular; // 성공한 같은 예약의 flags50 Regular 생성이다.
    std::function<void(Sid,std::int32_t,std::uint32_t)> frame; // 4acee0의 실제 프레임·표시 갱신이다.
    std::function<void(Sid)> update; // 가상+88의 실제 마지막 그림 갱신이다.
    std::function<void(Sid,std::uint32_t)> destroy; // 가상+10의 실제 프로세스·공간·풀 수명이다.
    std::function<void(float,float,std::string_view,std::uint32_t)> sound; // 중간B프레임의 powerup.wav 위치 소리다.
};
// 45a6c0→45a480과45a250을 같은 raw 슬롯/TYPE 프레임/게임 난수로 실행한다.
class RawFlare {
public:
    // 모든 빌린 현재 자료는 효과·Kernel 실행을 마칠 때까지 살아 있어야 한다.
    RawFlare(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,GameRandom& random,FlareState state,FlareHooks hooks);
    RawFlare(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::function<GameRandom&()> random,FlareState state,FlareHooks hooks); // 현재 난수를 실제 소비할 때만 같은 장기 owner에서 읽는다.
    bool Handles(Sid sid) const; // 실제 flare VT5096a0만 처리한다.
    // 원본 wrapper의 기본A구간으로 여러 클라이언트 효과를 만든다.
    void Spawn(Sid parent,std::int32_t count,std::uint32_t event) const;
    void SpawnDirection(Sid parent,std::int32_t count,std::uint32_t event,std::uint32_t type,std::uint32_t direction) const; // 45a480의 현재 효과 종류/방향별 첫 프레임을 사용한다.
    // 마지막 전 프레임의 후속 생성·마지막 갱신/삭제·실제 frame·B소리·float 주기를 처리한다.
    float Handle(Sid sid,std::uint32_t event,std::uint32_t count,float payload) const;
    RegularHandler Handler(RegularHandler fallback={}) const; // 다른 실제 부모의 기존 분배는 보존한다.
    const SidPool& Pool() const; // 단일 월드 조립 검사용 참조다.
private:
    std::uint32_t Read(Sid sid,std::size_t offset) const; // 비정렬 DWORD의 네 바이트만 읽는다.
    void Put(Sid sid,std::size_t offset,std::uint32_t value) const; // 원본 직접 frame 쓰기의 DWORD만 기록한다.
    std::span<std::uint8_t> Bytes(Sid sid) const; // Native는 실제 슬롯만 읽고 typed의 기존 할당 검사와 구별한다.
    double Random(float low,float high) const; // 실제455930의 같은 전역 난수와53비트 계산이다.
    RiftTypeFrames::LetterRun Run(Sid sid) const; // 현재 프레임 방향의 TYPE first/count를 조회한다.
    SidPool& pool_; // 모든 현재 효과와 부모가 사용하는 원본 풀이다.
    std::span<const RiftTypeRecord> types_; // 원본 부모 genus를 읽는 현재188TYPE다.
    std::span<const RiftTypeFrames> frames_; // 현재 frame code와 first/count다.
    std::function<GameRandom&()> random_; // 다른 게임 효과와 같은00532710을 선택 시 조회한다.
    FlareState state_; // 원본 현재 전역의 안정 참조다.
    FlareHooks hooks_; // 선택한 실제 외부 몸체의 연결이다.
};
}
