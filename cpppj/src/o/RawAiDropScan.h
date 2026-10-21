// 실제 표면 지도와 다리 J/K 주변 후보에서 AI 배치 스캔/30개 선택 목록을 복원한다.
#pragma once
#include "o/RawAiDropEvaluation.h"
#include "o/RawPathAnimation.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 원본 스캔 전역의 현재 저장소를 빌린다.
struct AiDropScanGlobals {
    const std::uint32_t& bridgeType; //005411a0의현재다리TYPE다.
    const AiBuildSearchBounds& bounds; //00544d14..20의현재경계이며CRT004fc530초기값은10,10,246,246이다.
};
// 이미복원된실제전체발자국/후보승인과선택된원본진단만연결한다.
struct AiDropScanHooks {
    std::function<bool(std::uint32_t,std::int32_t,std::int32_t)> free; //0040f130의같은현재TYPE/전체spot발자국이다.
    std::function<bool(std::uint32_t,std::uint32_t,RawPathPoint,std::int32_t,RawPathPoint&,Sid&)> consider; //00412b90의실제후보승인이며점수포인터는같은본체3e8이다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //0040f390의현재원본진단이다.반환하면현재count로계속한다.
};
// 같은본체/월드지도/원본공용난수와실제후보승인만사용한다.
class RawAiDropScan {
public:
    RawAiDropScan(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,std::span<const std::uint8_t> spots,std::span<const std::uint16_t> surfaces,std::span<const std::uint32_t> alliances,GameRandom& random,AiDropScanGlobals globals,AiDropScanHooks hooks); //같은PATCH물리풀/188TYPE/지도65536과동맹81칸을빌린다.
    bool AdjustBridge(Sid surface,std::int32_t& x,std::int32_t& y) const; //00412ce0:J는상/하·K는좌/우의전체BYTE0세칸뒤기준축을먼저쓰고다른축에공용난수3을더한다.
    bool Scan(std::uint32_t token,std::uint32_t type,std::int32_t direction) const; //00412e10의실제유일호출점수3e8/af8·afc진행/원본예산·30후보다.true는미완료다.
    void Remove(std::uint32_t token,std::int32_t index) const; //0040f390(this+3dc):현재count선감소→XY/표적안정압축이다.미사용꼬리는보존한다.
    const SidPool& Pool() const; //같은실제월드연결확인용이다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //현재살아있는실제0xb08본체만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재본체DWORD를읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정DWORD만쓰고주변을보존한다.
    std::uint8_t Spot(std::int32_t x,std::int32_t y) const; //원본선형주소를검사하며행끝의물리이웃을좌표clamp하지않는다.
    std::size_t Point(std::int32_t index) const; //정상30개좌표목록의물리인덱스를검사한다.
    std::size_t Target(std::int32_t index) const; //정상30개표적목록의물리인덱스를검사한다.
    void Choice(std::uint32_t token,std::int32_t index,RawPathPoint point,Sid target) const; //선택한XY와같은표적DWORD만기록한다.
    RawAiBrainMemory& memory_; //같은본체/진행/후보/점수저장소다.
    const SidPool& pool_; //현재표면TYPE/소유자/프레임을읽는실제풀이다.
    std::span<const RiftTypeRecord> types_; //각스캔호출처음의TYPE flags2를읽는다.
    std::span<const RiftTypeFrames> frames_; //현재물리frame의signed첫문자다.
    std::span<const std::uint8_t> spots_; //원본현재65536BYTEspot이다.
    std::span<const std::uint16_t> surfaces_; //원본5c84bc의현재0단계65536WORD표면이다.
    std::span<const std::uint32_t> alliances_; //원본현재방향별9×9DWORD동맹이다.
    GameRandom& random_; //SP/전투/시설과같은원본공용난수다.
    AiDropScanGlobals globals_; //현재다리TYPE와전체스캔경계의참조다.
    AiDropScanHooks hooks_; //선택된실제하위/진단이다.
};
}
