// 원본 네 다리 전략의 공통 관문·목록 이동·현재 목표·확장 순서를 복원한다.
#pragma once
#include "o/RawAiBrain.h"
#include "o/RawAiBuildSearch.h"
#include "o/SpStore.h"

namespace netstorm::o {
// 같은 현재 전역을 각 원본 조회 위치에서 읽는다.
struct AiBridgeStrategyGlobals {
    const double& now; //0055b4d0의현재QWORD게임시각이다.
    const std::int32_t& mapLimit; //00531928을float로저장한뒤좌표유효성을검사한다.
    std::function<std::uint32_t()> invalidGraph; //00540410의현재DWORD이며BYTE장부도같은getter로연결한다.
    std::function<bool()> checking,disabled; //005e4794와0054db88의단락관문이다.후자는checking일때만읽는다.
    const std::uint32_t& geyserLog; //0052f3e4의현재간헐천로그종류다.
};
// 이미 복원된 실제 월드/계획/경로 하위와 현재 장치 경계만 연결한다.
enum class AiBridgeStrategyLog { Requeued,Abandoned,Trying }; //기존간헐천의재등록/포기/시도세원본문구다.
struct AiBridgeStrategyHooks {
    std::function<Sid(std::uint32_t)> technology; //0048fc90의현재owner기술부모다.
    std::function<void(std::uint32_t,Sid)> refresh; //0040fcb0의현재다리재고스캔/보충이다.
    std::function<std::uint32_t(Sid)> graph; //00462bc0의현재실제object graph다.
    std::function<RawPathPoint(Sid)> center; //004adea0의현재float중심이다.원본0040f330의거의올림은전략몸체가적용한다.
    std::function<bool(std::uint32_t,Sid)> connected; //00410d40의현재간헐천가장자리연결이다.
    std::function<void(std::uint32_t)> delay; //00411310의현재count별40/120초기한이다.
    std::function<RawPathPoint(std::uint32_t)> home; //004112b0의현재신전중심/fallback이다.
    std::function<void(std::uint32_t)> reclassify; //00411350의현재연결해제간헐천재분류다.
    std::function<Sid(std::uint32_t,RawPathPoint,std::uint32_t,std::uint32_t)> select; //00410a30의같은348목록선택이다.
    std::function<RawPathPoint(std::uint32_t,const AiBuildSearchBounds&)> randomStart; //00411470의현재열린지면20회탐색이다.
    std::function<bool(std::uint32_t,RawPathPoint)> advance; //004174c0의현재3d0다리시작/연장/timeout이다.
    std::function<RawPathPoint(std::uint32_t,RawPathPoint,RawPathPoint)> extend; //004173b0의동일배치/소비/보충이다.
    std::function<bool(std::int32_t,std::int32_t,std::int32_t,std::int32_t)> rectangle; //004119b0의inclusive빈spot사각형이다.
    std::function<void(std::string_view,std::uint32_t)> assertion; //004e0620의선택된현재진단이며반환하면원본처럼계속한다.
    std::function<void(std::uint32_t,std::uint32_t,AiBridgeStrategyLog,std::uint32_t)> log; //004105b0의현재B00/장치관문에세로그를전달한다.
};
//00417bd0 입구와네case전체를동일실제본체/목록/난수에적용한다.
class RawAiBridgeStrategies {
public:
    RawAiBridgeStrategies(RawAiBrainMemory& memory,const SidPool& pool,std::span<const RiftTypeRecord> types,GameRandom& random,AiBridgeStrategyGlobals globals,AiBridgeStrategyHooks hooks); //같은PATCH188세계와현재전역을빌린다.
    void Update(std::uint32_t token) const; //공통관문→현재재고갱신→현재목표1..4를원본순서로한번처리한다.
    const SidPool& Pool() const; //실제Session조립에서다른월드혼합을확인한다.
private:
    std::span<std::uint8_t> Body(std::uint32_t token) const; //살아있는정확한0xb08본체만허용한다.
    std::uint32_t Field(std::uint32_t token,std::size_t at) const; //현재비정렬DWORD를읽는다.
    void Field(std::uint32_t token,std::size_t at,std::uint32_t value) const; //지정네BYTE만쓴다.
    RawPathPoint Point(std::uint32_t token,std::size_t at) const; //두float의원문비트를읽는다.
    void Point(std::uint32_t token,std::size_t at,RawPathPoint point) const; //X/Y순서로정확한비트를쓴다.
    Sid Object(std::uint32_t value) const; //물리풀검사뒤에만WORD로좁힌다.
    std::uint32_t Genus(std::uint32_t value) const; //현재raw타입의실제flags2다.
    std::uint32_t Item(std::uint32_t token,std::size_t list,std::int32_t index) const; //현재목록포인터의실제DWORD다.
    void Append(std::uint32_t token,std::size_t list,std::uint32_t value) const; //full일때만생략하며중복을유지한다.
    void Remove(std::uint32_t token,std::size_t list,std::uint32_t value) const; //모든일치를안정압축하고미사용꼬리를보존한다.
    bool Ready(std::uint32_t token,std::size_t deadline) const; //현재신전/graph/NaN거부의QWORD기한관문이다.
    bool Valid(RawPathPoint point) const; //float지도상한의strict두축조건이다.
    RawPathPoint RoundedCenter(std::uint32_t value) const; //0040f330의실제중심→+0.99999f→signedCRT하위DWORD→float다.
    void ResetPoint(std::uint32_t token) const; //원본순서Y→X로3d0/3d4를-10000으로쓴다.
    void Temple(std::uint32_t token) const; //목표1:현재살아있는다른graph신전으로진행하거나대상을재설정한다.
    void Geysers(std::uint32_t token) const; //목표2:연결/포기/재시도목록을원본순서로처리한다.
    void Expand(std::uint32_t token) const; //목표3:주변3칸/자기영역의열린시작점에서무작위목표로연장한다.
    void Coordinate(std::uint32_t token) const; //목표4:현재명시좌표주변빈사각형을확인하고실패하면해제한다.
    RawAiBrainMemory& memory_; //기존AI와같은본체/목록의유일소유자다.
    const SidPool& pool_; //현재종류/생사/좌표를읽는동일풀이다.
    std::span<const RiftTypeRecord> types_; //현재실제188TYPE의genus다.
    GameRandom& random_; //다른모듈과같은게임공용난수다.
    AiBridgeStrategyGlobals globals_; //현재전역의참조/getter다.
    AiBridgeStrategyHooks hooks_; //선택된실제하위효과다.
};
}
