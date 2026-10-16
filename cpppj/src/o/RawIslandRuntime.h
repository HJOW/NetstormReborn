// 실제100개/32바이트 섬 목록과 현재owner 변경 효과를 같은 pool/해시/목록/Player 상태에서 실행한다.
#pragma once
#include "o/RawFortFinishOwner.h"
#include "o/RawPathSearch.h"

namespace netstorm::o {
// 실제 섬 목록이 보고하는 진단이며 callback 뒤 원본처럼 조건부 작업을 이어간다.
enum class IslandRuntimeAssertion {
    // MissingIsland: 존재/범위 검사에 실패한 섬의 getter/setter를 요청했다.
    MissingIsland,
    // InvalidBounds: owner변경 뒤 원본 경계 검사에 실패해 현재 거주 구역 전파 전에 보고한다.
    InvalidBounds
};
// 지면 소유자를 실제 거주 구역 가상owner에 전달하고 원본 진단을 소비자에게 보고한다.
struct IslandRuntimeHooks {
    // setResidenceOwner: 현재residences의 실제 SID에 가상+0x74를 실행한다. 같은 raw pool을 갱신해야 한다.
    std::function<void(Sid,std::uint32_t)> setResidenceOwner;
    // assertion: 원본 진단 뒤 계속 진행하는 경계를 보존한다. 빈효과로 복원 완료를 주장하지 않는다.
    std::function<void(IslandRuntimeAssertion)> assertion;
};
// 패치0046edf0..0046f320와 CD004bd3f0..004bda70의 mutable IslandList 몸체다.
class RawIslandRuntime {
public:
    // countDWORD+100개32바이트 기록의 실제 바이트 수다. records는 CoreData가 소유한다.
    static constexpr std::size_t kBytes=4+100*32;
    // 기존 Fort모드/Player 첫 지역과 revision/변경DWORD·실제residences/hash/pool을 공유한다.
    RawIslandRuntime(const SidPool& pool,const SquidHash& hash,std::span<std::uint8_t> records,
        FortFinishOwnerState& modes,SquidPostPopList& residences,std::uint32_t& revision,std::uint32_t& ownerChanged,
        IslandRuntimeHooks hooks,std::int32_t worldExtent=256);
    // 100개 기록의Temple/존재/현재owner/원래owner/지역/경계를 초기화하고 padding2BYTE는 보존한다.
    void Reset();
    // 현재countDWORD를 반환한다. 삭제 뒤 줄이거나 빈 구간을 재사용하지 않는다.
    std::uint32_t Count() const;
    // 유효번호의 존재DWORD를 그대로 반환하며 범위 밖은0이다.
    std::uint32_t Exists(std::int32_t island) const;
    // 현재 끝에 원래ownerBYTE/currentownerDWORD/지역BYTE/존재1만 쓰고 이전count를 반환한다.
    std::uint32_t Add(std::uint32_t owner,std::uint8_t territory);
    // 존재하는 기록만 Reset의 필드값으로 바꾸고 count/padding을 유지한다. 패치의 missing진단을 보존한다.
    void Delete(std::int32_t island);
    // 일반모드는 signed원래owner/지역BYTE, special/chaff는 nonzero현재owner/지역BYTE로 판단한다.
    bool Matches(std::int32_t island,std::int32_t owner,std::int32_t territory) const;
    // 현재 신전DWORD를 조회한다. 패치의 missing진단 뒤0반환을 보존한다.
    std::uint32_t Temple(std::int32_t island) const;
    // 실제 신전DWORD를 지정하며 타입이나 번호세대 해석을 추가하지 않는다.
    void Temple(std::int32_t island,std::uint32_t temple);
    // 현재ownerDWORD를 조회하며 원래ownerBYTE와 합치지 않는다.
    std::uint32_t Owner(std::int32_t island) const;
    // 허용모드에서owner/revision/패치changed→현재bounds→residence전파→Player 첫지역을 원본 순서로 쓴다.
    void Owner(std::int32_t island,std::uint32_t owner);
    // 원본은 missing진단 뒤에도 물리기록의4float를 읽는다. 물리100개 밖은 메모리 경계로 진단한다.
    FortRandomRectangle Bounds(std::int32_t island) const;
    // 존재할 때만 원본float비트를 그대로 대입한다. 패치의 missing진단을 보존한다.
    void Bounds(std::int32_t island,const FortRandomRectangle& bounds);
    // count와100개 원본기록을 같은 월드의 다른 소비자에게 읽기 전용으로 제공한다.
    std::span<const std::uint8_t> Bytes() const;
    // 실제pool의 동일성을 월드/후처리 바인딩에서 확인한다.
    const SidPool& Pool() const;
    // 실제9 Player BYTE의 첫 지역DWORD를 직접 공유한다. 연결 뒤 modes.playerRegions 사본은 쓰지 않는다.
    void BindPlayerRecords(std::span<std::uint8_t> players);
private:
    // 현재owner의 PATCH+78/CD+70 첫 지역DWORD를 읽고 쓴다.
    std::int32_t FirstRegion(std::uint32_t owner) const;
    void FirstRegion(std::uint32_t owner,std::int32_t region);
    // 실제 little endian BYTE/WORD/DWORD를 지정한 폭으로 읽는다.
    std::uint32_t Read(std::size_t offset,std::size_t width=4) const;
    // 지정한 폭의 원본 필드만 쓰고 이웃/padding을 보존한다.
    void Write(std::size_t offset,std::uint32_t value,std::size_t width=4);
    // 물리기록0..99의 시작offset를 얻는다. logical존재와 별개다.
    static std::size_t Record(std::int32_t island);
    // 현재기록 한 개를 원본의 필드별Reset로 지운다.
    void ResetRecord(std::int32_t island);
    // missing진단이 판본/해당getter에서 필요한지 결정하고 현재존재를 반환한다.
    bool Require(std::int32_t island,bool bothEditions=false) const;
    // 실제 네float의 경계검사를 판본별 x87/정수비트 조건으로 수행한다.
    bool ValidBounds(const FortRandomRectangle& bounds) const;
    // candidate의 현재float가 원본bounds 안인지 판본별 unordered조건을 보존한다.
    bool Contains(const FortRandomRectangle& bounds,float x,float y) const;
    // pool_: 가상owner와 현재type/좌표/표면owner 조회의 단일raw 풀이다.
    const SidPool& pool_;
    // hash_: 기존RawPathSurfaceAt이 읽는 현재hash0 WORD 지도다.
    const SquidHash& hash_;
    // records_: CoreData가 소유한 실제count/100개 mutable32바이트기록이다.
    std::span<std::uint8_t> records_;
    // modes_: 기존 FortFinishOwner의fort/special/chaff/Player첫지역/거주 구역타입을 공유한다.
    FortFinishOwnerState& modes_;
    // residences_: AssetPostPop의 실제 등록목록이며 매 반복 currentcount/entries를 다시 읽는다.
    SquidPostPopList& residences_;
    // revision_: 원본전역 revisionDWORD를 지역투표/도장과 동일하게 공유한다.
    std::uint32_t& revision_;
    // ownerChanged_: 패치 owner변경에서만1을 쓰는 별도DWORD다. CD는 유지한다.
    std::uint32_t& ownerChanged_;
    // hooks_: 필수 실제거주 구역owner와 원본 진단효과다.
    IslandRuntimeHooks hooks_;
    // worldExtent_: 실제isValid에서float로 읽는 원본월드상한DWORD다.
    std::int32_t worldExtent_;
    std::span<std::uint8_t> players_; // 생성/신전/경제와 같은 실제9 Player 기록이다. 미연결 독립 검사는 기존 상태를 쓴다.
};
}
