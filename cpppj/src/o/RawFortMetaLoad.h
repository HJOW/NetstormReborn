// Fort Badges의실제28개표와Technology wrapper를같은loadingDepth/contents로연결한다.
#pragma once
#include "o/RawFortObjectLoad.h"

namespace netstorm::o {
// 원본MemStream의현재pointer존재/pos/end와실제저장바이트다.
struct FortMetaStream {
    std::span<const std::uint8_t> bytes; // 현재섹션의원본바이트이며badge는직접읽는다.
    std::uint32_t position{},end{}; // lookup은pos0을설정하고wrapper/하위contents가현재pos/end를쓴다.
    bool present{true}; // 원본data pointer의nonnull여부다. bytes.empty와구별한다.
};
// 부모Template와현재로컬플레이어·conversion포인터의논리상태다.
struct FortMetaLoadState {
    bool templatePresent{true}; // 실제Template첫DWORD가0이면보고후계속한다.
    std::int32_t templateSlot{1},localOwner{1}; // 실제this=TemplateBase+slot*20과현재플레이어DWORD의동일성이다.
    bool hasTypeConversion{}; // 원본typeConversion이nonnull인지매해제시점다시읽는다.
};
// 원본보고뒤계속할수있는각진단의의미다.
enum class FortMetaAssertion {
    MissingBadgesTemplate, // Badges wrapper의Template첫DWORD가0이다.
    BadgeCountRange, // 직접setter count가0..255범위를벗어났다.
    MissingTechnologyTemplate, // Technology wrapper의Template첫DWORD가0이다.
    TechnologyNotMine // 실제현재플레이어Template주소와this가다르다.
};
// 내용물생성/TypeNames/화면출력은실제하위몸체로공급하며noop으로복원을대체하지않는다.
struct FortMetaLoadHooks {
    std::function<void(FortMetaAssertion)> assertion; // 원본진단보고뒤현재상태로계속한다.
    std::function<void(std::uint32_t)> loadingDepthChanged; // 같은Money/객체/사제모듈에현재깊이를동기화한다.
    std::function<Sid()> technologyParent; // 0048fc60/CD00406d90 현재technology 부모SID를조회한다.
    std::function<void()> prepareTypeNames; // 004bd800/CD00427e00의실제변환표준비경계다.
    std::function<void(Sid,FortMetaStream&,std::uint32_t,bool)> contents; // 004bd130/CD00426dd0의samePool contents(flags11,relaxedConversion1)다.
    std::function<void()> releaseTypeConversion; // 현재nonnull표의_free/CD00426830하위free경계다.
    std::function<void()> refreshTechnologyUi; // 마지막0042f540/CD004b2e70 표시경계다.
};
// 실제badge유형DWORD/수량DWORD와Technology순서를보존하는독립wrapper다.
class RawFortMetaLoad {
public:
    // badges는장기소유한원본28×220BYTE이며pool/depth/state/hooks보다이객체수명이짧아야한다.
    RawFortMetaLoad(const SidPool& pool,std::span<std::uint8_t> badges,std::uint32_t& loadingDepth,
        FortMetaLoadState& state,FortMetaLoadHooks hooks);
    // 0047d800/CD00475660:범위진단뒤현재표의첫같은type에전체countDWORD를쓴다.
    void SetBadge(std::uint32_t type,std::int32_t count) const;
    // CD00475560와patchsetter내부의같은28개linear검색이다.못찾으면−1이다.
    std::int32_t FindBadge(std::uint32_t type) const;
    // 004be8e0/CD00428fc0:조회pos0→개수BYTE→type/count BYTE쌍→실제setter순서다.
    void LoadBadges(FortMetaStream& stream) const;
    // 실제 manager 선택을 판본별 깊이/Template 진단 뒤에 지연하며 빈 섹션 여부는 본문이 판단한다.
    void LoadBadgesSelected(const std::function<FortMetaStream&()>& select) const;
    // 004beee0/CD004293d0:깊이→Template/isMine→부모조회→Tech조회→변환준비/Contents→해제→UI→깊이순서다.
    void LoadTechnology(FortMetaStream& stream) const;
    // 실제 기술 부모를 조회한 뒤 현재 manager에서 Technology를 선택한다. 선택 중 실패도 깊이를 복구한다.
    void LoadTechnologySelected(const std::function<FortMetaStream&()>& select) const;
    // 실제contents어댑터가다른SID풀을섞지않았는지확인한다.
    const SidPool& Pool() const;
private:
    // raw표DWORD를정확히읽고type/count사이의212바이트를보존한다.
    std::uint32_t ReadBadge(std::size_t offset) const;
    // 지정DWORD만쓰며28개기록의다른필드와padding을보존한다.
    void WriteBadge(std::size_t offset,std::uint32_t value) const;
    std::uint8_t ReadByte(FortMetaStream& stream) const; // 원본badgeBYTE읽기이며손상물리메모리는계약밖이다.
    void ChangeDepth(std::uint32_t value) const; // 현재DWORD쓰기를동일소비자에전달한다.
    const SidPool& pool_; // 같은판본/실제contents부모의SID저장소다.
    std::span<std::uint8_t> badges_; // 실제type/count/중간padding을가진28개220BYTE장기기록이다.
    std::uint32_t& loadingDepth_; // Money/Template/사제와공유하는현재DWORD다.
    FortMetaLoadState& state_; // 진단/하위호출뒤현재Template/conversion상태를다시읽는다.
    FortMetaLoadHooks hooks_; // 미복원하위몸체를기본성공값으로대체하지않는다.
};
// 이미원본TypeNames파싱을끝낸같은FortTemplate의실제Contents몸체만연결한다.
// 변환표준비/해제·현재technology부모·UI의전체복원은이함수의범위가아니다.
FortMetaLoadHooks MakeFortMetaContentsHooks(const SidPool& pool,const RawFortObjectLoad& objects,
    const FortTemplate& fort,FortMetaLoadHooks hooks);
}
