// 간헐천 채집 통지의 현재 수량/cost·게임 난수·고갈 삭제/emptyGeyser 생성을 복원한다.
#pragma once
#include "o/SidPool.h"
#include "o/SquidHash.h"
#include "o/SpStore.h"
#include <functional>
#include <string_view>

namespace netstorm::o {
class RawPacketDispatch; // 원본 단일 패킷 분배기는 등록 helper에 참조로만 전달한다.
// handler가 효과 callback 뒤에도 다시 읽는 실제 전역이다.
struct GeyserMinedState {
    bool boss{true},checking{}; // 원본 권한과 패치 bridge 수량 assert를 제어한다.
    std::uint32_t nuggetType{154},emptyGeyserType{84},isleType{97},bridgeType{82}; // 실제 두 PE의 현재 비용/생성/표면/수량 setter 타입 번호다.
    const bool* sharedBoss{}; // 실제 수신/음향 효과와 같은 Client boss 주소다.
    const bool& Boss() const { return sharedBoss ? *sharedBoss : boss; } // 음향과 수량 갱신 뒤 고갈 삭제 관문에서 현재값을 읽는다.
};
// 음향·가상 삭제/생성·Pop은 같은 raw 월드에 연결해야 하는 효과 경계다.
struct GeyserMinedHooks {
    std::function<void(float,float,std::string_view,std::uint32_t)> sound; // geyserMined1..6.wav를 현재 위치에서 flags0으로 요청한다.
    std::function<void(Sid,std::uint32_t)> destroy; // 고갈한 간헐천의 실제 +0x10(flags0x1000)이다.
    std::function<Sid(std::uint32_t,std::uint32_t)> create; // emptyGeyser의 실제 factory(type,flags0)이다.
    std::function<void(Sid,float,float,std::uint32_t)> pop; // 생성물의 가상 +0x90에 삭제 전 보존한 좌표와 flags0을 넘긴다.
};
// 패치004266c0/CD004e41e0의 packet handler다. 경제/운반 상위 상태를 별도 캐시에 보관하지 않는다.
class RawGeyserMined {
public:
    // 풀/hash/타입/난수/전역은 처리기보다 오래 살아야 한다. 효과 네 개는 모두 필수다.
    RawGeyserMined(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
        GameRandom& random,GeyserMinedState& state,GeyserMinedHooks hooks);
    // raw 수량은 unsigned WORD로 읽고 cost 차감의 ftol 하위 WORD를 기록한다. free 객체는 모든 효과를 건너뛴다.
    void Handle(Sid geyser) const;
    // 기존 packet의 +4 WORD SID를 읽는다. 사용하지 않는 첫 두 handler 인자는 해석하지 않는다.
    void HandlePacket(std::span<const std::uint8_t> packet) const;
    // 실제 월드 어댑터가 다른 풀을 연결하지 않도록 참조를 공개한다.
    const SidPool& Pool() const;
private:
    // 원본 좌표와 저장된 수량을 실제 raw 슬롯에서 읽는다.
    static float Coordinate(std::span<const std::uint8_t> bytes,std::size_t offset);
    static std::uint16_t Quantity(std::span<const std::uint8_t> bytes);
    // ftol의 유효 signed64 절삭/불확정값 하위0과 WORD 저장을 보존한다.
    static std::uint16_t ReducedQuantity(std::uint16_t quantity,float cost);
    // 좌표 ftol의 signed64 불확정값과 하위 DWORD 감김까지 원본과 같이 보존한다.
    static std::int32_t CoordinateInteger(float coordinate);
    // 실제 수량 setter는 기록 뒤 patch bridge assert를 수행하고 CD bridge 접두는 빈 반환이다.
    void SetQuantity(Sid sid,std::uint16_t quantity) const;
    // 좌표를 편향 없이 절삭해 현재 hash0 머리의 타입을 확인한다. Graph/소유자를 추가 조건으로 쓰지 않는다.
    bool OnIsle(float x,float y) const;
    SidPool& pool_; // 실제 간헐천/생성물의 유일한 raw 슬롯 저장소다.
    const SquidHash& hash_; // 삭제 callback이 바꾼 현재 표면 머리를 읽는다.
    std::span<const RiftTypeRecord> types_; // sound 뒤 현재 nugget 타입 cost를 읽는 공유 타입 표다.
    GameRandom& random_; // 소리 하나마다 정확히 한 번 전진하는 전역 게임 난수다.
    GeyserMinedState& state_; // 삭제/sound callback 뒤의 현재 권한/타입 번호를 읽는다.
    GeyserMinedHooks hooks_; // 같은 월드의 필수 음향/가상 삭제/생성/Pop이다.
};
// 양판본 초기화의 0x5b·빈 observer·version1/mismatch0·실제 handler를 연결한다. handler는 분배기보다 오래 살아야 한다.
void RegisterGeyserMined(RawPacketDispatch& dispatch,RawGeyserMined& handler);
}
