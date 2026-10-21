// AI의 에너지 부족 배치 좌표 목록을 같은0xb08 본체/12바이트 기록에서 처리한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/SpStore.h"
#include <string>

namespace netstorm::o {
// 실제 Mana 판정과 원본 로그 장치를 빌리며 임의 배치 성공으로 대신하지 않는다.
struct AiDrySpotHooks {
    std::function<std::uint32_t(std::string&,float,float,std::uint32_t,std::uint32_t)> energy; //004734d0의 초기빈문자열/좌표/TYPE/소유자와원래DWORD반환이다.
    std::function<void(std::uint32_t,bool)> log; //00417c70의Killed [Fake] Dry Spot과004105b0의현재실제로그다.
};
// 별도 좌표 사본 없이 원본adc..aec와 같은 소유 backing을 사용한다.
class RawAiDrySpots {
public:
    RawAiDrySpots(RawAiBrainMemory& memory,const SidPool& pool,AiDrySpotHooks hooks); //같은PATCH풀/본체등록부를빌린다.
    bool Add(std::uint32_t token,float x,float y,std::uint32_t type) const; //0040f1c0:용량우선→좌표만중복검사→원문12바이트추가다.
    void Remove(std::uint32_t token,std::int32_t index) const; //0040f260:뒤기록을안정압축하고count만감소한다.꼬리/cursor는보존한다.
    std::uint32_t Scan(std::uint32_t token) const; //0040f2b0:다음signed나머지한항목만검사하고충족하면현재기록TYPE를읽어제거한다.
    std::uint32_t Maintain(std::uint32_t token) const; //00417c70의해당하위만실행하고현재fakeTYPE/디버그플래그로로그를분배한다.
    void Seed(std::uint32_t token,GameRandom& random) const; //004133e0:능력 BYTE20h에서 같은 공용 난수8로 경계 좌표를 고르고 현재 fake TYPE로 등록한다. full/중복이어도 af0/af4를 유지한다.
    const SidPool& Pool() const; //같은실제월드연결을확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재살아있는실제0xb08본체만빌린다.
    std::uint32_t Field(std::uint32_t token,std::size_t offset) const; //현재본체DWORD를읽는다.
    void Field(std::uint32_t token,std::size_t offset,std::uint32_t value) const; //선택된본체DWORD만쓴다.
    std::span<std::uint8_t> Records(std::uint32_t token) const; //현재ae4가가리키는동일backing을다시읽는다.
    std::size_t Offset(std::span<const std::uint8_t> raw,std::int32_t index) const; //음수/물리블록밖접근만해당지점에서거부한다.
    RawAiBrainMemory& memory_; //생성자/후속전략과같은실제소유본체/목록등록부다.
    const SidPool& pool_; //같은판본/월드정체성을유지한다.
    AiDrySpotHooks hooks_; //선택된실제에너지/로그효과다.
};
}
