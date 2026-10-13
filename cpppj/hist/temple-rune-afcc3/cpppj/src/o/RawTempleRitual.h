// 신전 의식의 현재 단계·SHP 목표점·사제/룬 조회와 실제 하위 요청 순서를 복원한다.
#pragma once
#include "o/RawPriestRitualActions.h"
#include "o/RawSquidNeighbors.h"
#include "o/SquidDisplay.h"
#include "o/SquidReward.h"
#include <string_view>

namespace netstorm::o {
class SquidFactory; // 같은 실제 풀에 Rune 생성자를 적용하는 기존Factory다.
class SquidOwner; // Rune의 공통가상+0x74 몸체를 재사용한다.
class SquidProcessHost; // 같은 실제Kernel에0x307 SharedRegular를 부착한다.
// 의식 helper가 효과 뒤에도 다시 읽는 현재 원본 전역이다.
struct TempleRitualState {
    bool boss{true}; // 00540bc4/CD00540a2c: 후속 모드에서 룬을 생성할 권한이다.
    std::uint32_t priestType{158},runeType{166}; // 005412d0/005412f0 ↔ CD0051cbbc/0051cbdc의 현재 타입 번호다.
    float pixelScaleX{16},pixelScaleY{11}; // 0059a92c/930 ↔ CD00565c44/48: SHP 정수 핫스폿을 월드 좌표로 바꾼다.
};
// 새 좌표 자료형은 원본의 단정도 저장 x/y 두 개다.
struct TempleRitualPoint {
    float x{},y{}; // pixel offset마다 원본 float 저장 경계를 유지하는 현재 좌표다.
};
// 실제 Factory/공통 HP/owner/Pop/위치 음향/공유 예약의 필수 하위 연결이다.
struct TempleRitualHooks {
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // Factory.Create(현재 runeType,flags0)이며 실제 슬롯 번호를 반환한다.
    std::function<void(Sid,std::int32_t)> setHitPoints; // 기존 공통 setter004adca0/CD004aed20이며 Rune HP0을 쓴다.
    std::function<void(Sid,std::uint32_t)> setOwner; // 원본 가상+0x74를 현재 신전 owner BYTE로 요청한다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 원본 가상+0x90을 현재 StageAnchor 위치와flags0으로 요청한다.
    std::function<void(float,float,std::string_view,std::uint32_t)> playAt; // 현재 사제 좌표에서 단계별 for*.wav를flags0으로 요청한다.
    std::function<bool()> reserveShared; // 위치 음향 뒤 원본 new0x28 성공 여부를 공급한다.
    std::function<void(Sid,std::uint32_t,float)> addShared; // 같은 Host.AddSharedRegular(신전,0x307,단계별지연)이다.
};
// 기존 실제 Footprint/방향 프레임/최대HP/사제 의식 동작을 같은 풀에서 재사용한다.
// 사용: PathCleanup의 ritual 경계와 신전0x307 후속에서 Ritual을 호출한다. 하위 효과와 참조는 더 오래 살아야 한다.
class RawTempleRitual {
public:
    // 표/프레임/SHP/spot은 원본 타입 인덱스와 같은 목록이며 다른 풀/해시 연결은 거부한다.
    RawTempleRitual(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        std::span<const RiftTypeFrames> frames,std::span<const SquidDisplayShape> shapes,std::span<const std::uint8_t> spots,
        const RawSquidNeighbors& footprint,const SquidReward& hitPoints,const RawPriestRitualActions& actions,
        TempleRitualState& state,TempleRitualHooks hooks);
    // 실제 dais +0x80의 현재 방향N/M/L 우선 및 frame64/120 경계로1..3을 반환한다.
    std::uint32_t Count(Sid temple) const;
    // 현재 count별 첫 프레임을 빼고11/22/33/44/55 경계로0..5를 읽는다. frame-1은 현재 raw 프레임이다.
    std::uint32_t Stage(Sid temple,std::int32_t frame=-1) const;
    // 원본00447aa0/CD00481c80: 신전 좌표+첫단계의SHP 핫스폿이다. 정상stage0..4만 받는다.
    TempleRitualPoint StageAnchor(Sid temple,std::uint32_t stage) const;
    // Anchor+룬SHP 핫스폿이며stage4의 절삭 칸 spot6/!8이면(-9,-6)픽셀 보정한다.
    TempleRitualPoint StagePoint(Sid temple,std::uint32_t stage) const;
    // 현재 단계의룬SHP 핫스폿을 빼고 실제 일반 점 finder의 첫 현재 runeType을 반환한다.
    Sid RuneAt(Sid temple,TempleRitualPoint point) const;
    // 신전 발자국±1의 일반 finder에서 같은 owner이며HP>=현재 최대HP/2인 첫 사제를 찾는다.
    Sid FindStrongOwnedPriest(Sid temple) const;
    // 신전 발자국의 첫 현재 runeType 중extra1인 후보가 있는지 실제 finder로 읽는다.
    bool HasAbstractRune(Sid temple) const;
    // 생성→HP0→현재stage 프레임→void assert→abstract1→현재owner→다시현재stage Anchor→Pop 순서다.
    void SpawnRune(Sid temple) const;
    // 전체00447e40/CD00481fc0: 목표점 일치와Rune 없음 뒤mode0은frame/음향/307,나머지는604후속/권한Rune이다.
    void Ritual(Sid temple,Sid priest,std::uint32_t mode) const;
    // 동일 월드의 PathCleanup/Host 연결을 검증한다.
    const SidPool& Pool() const;
private:
    // 실제 현재 raw 프레임은 패치signedDWORD/CDBYTE 폭이다.
    std::int32_t CurrentFrame(Sid sid) const;
    // 원본 float 좌표 두 개와 현재 owner BYTE를 별도 raw 조회로 읽는다.
    TempleRitualPoint Position(Sid sid) const;
    std::uint32_t Owner(Sid sid) const; // 원본 effect 뒤에도 현재 owner를 읽도록 값을 보관하지 않는다.
    // 현재 타입의SHP배열에서 원본 signed short 핫스폿을 읽고 손상 범위는 사전에 거부한다.
    std::array<std::int16_t,2> Hotspot(std::uint32_t type,std::int32_t frame) const;
    // 각 축의 정수픽셀/현재scale 덧셈 뒤단정도로 저장한다.
    TempleRitualPoint AddPixels(TempleRitualPoint point,std::int32_t x,std::int32_t y) const;
    // 발자국 사각형을 원본 절삭/거의올림 우하단으로 바꿔 일반 finder를 시작한다.
    SquidSearchArea SearchArea(const SquidFootprint& area) const;
    SidPool& pool_; // 현재 슬롯 및Rune raw 필드를 쓰는 유일한 저장소다.
    const SquidHash& hash_; // 사제와Rune이 공유하는 실제 네 단계 해시다.
    std::span<const RiftTypeRecord> types_; // 현재 타입genus와일반finder발자국이다.
    std::span<const RiftTypeFrames> frames_; // 현재dais방향을 기존PathAnimation에서 읽는 표다.
    std::span<const SquidDisplayShape> shapes_; // 실제SHP핫스폿 메타 자료를 참조한다.
    std::span<const std::uint8_t> spots_; // 마지막목표점의 지면 보정이읽는 지도다.
    const RawSquidNeighbors& footprint_; // 기존실제발자국몸체를 재사용한다.
    const SquidReward& hitPoints_; // 기존실제최대HP를 재사용한다.
    const RawPriestRitualActions& actions_; // 기존실제K..P선택/604예약몸체다.
    TempleRitualState& state_; // 효과뒤현재권한/타입/픽셀scale다.
    TempleRitualHooks hooks_; // 모든하위요청의실제명시연결이다.
};
// 실제 Factory/공통HP setter/Owner/Host를 연결한다. Pop/음향/확보 경계는 실제 caller가 공급한다.
TempleRitualHooks MakeTempleRitualHooks(SidPool& pool,SquidFactory& factory,SquidOwner& owner,SquidProcessHost& host,TempleRitualHooks hooks);
}
