// 건설이 빌리는 실제 Canon/핫스폿/SHP와 현재 진단의 Client 수명을 비MP 경계로 검사한다.
#include "DudeSpawnSupport.h"
#include "client/PriestPlacementAssets.h"
#include "client/RawSurfaceWorld.h"
#include <algorithm>
#include <cstring>

using namespace netstorm::client;
using namespace netstorm::o;
namespace { // 실제 보호 자산을 읽으며 통신/패킷/게임 실행은 선택하지 않는다.
// Native 결합에 필요한 실제 장치 형식만 공급하고 선택되면 검사 자체를 실패시킨다.
void AssetsNativeDevices(RawSurfaceStorage& storage) {
    storage.BindTransmissionDevices({[](auto) { throw std::logic_error("자산 수명 검사에서 방송 선택 금지"); },[](auto)->bool { throw std::logic_error("자산 수명 검사에서 연결 조회 금지"); },[](auto,auto) { throw std::logic_error("자산 수명 검사에서 송신 선택 금지"); }});
}
// 예상한 계약 오류만 허용하며 다른 예외는 검사 실패로 남긴다.
template<class Function> bool AssetsRejected(Function&& function) { try { function();return false; }catch (const std::logic_error&) { return true; } }
// padding 비교 없이 실제 SHP 헤더의 모든 의미 필드를 대조한다.
bool SameAssetFrame(const SquidDisplayFrame& a,const SquidDisplayFrame& b) {
    return a.width==b.width && a.height==b.height && a.hotspotX==b.hotspotX && a.hotspotY==b.hotspotY && a.cellWidth==b.cellWidth && a.cellHeight==b.cellHeight && a.cellHotspotX==b.cellHotspotX && a.cellHotspotY==b.cellHotspotY && a.muzzleX==b.muzzleX && a.muzzleY==b.muzzleY && a.walkStepX==b.walkStepX && a.walkStepY==b.walkStepY;
}
}

// 실제188TYPE 전체의 코드/기본프레임/핫스폿/SHP 필드와 같은 장기 저장소 주소를 확인한다.
TEST_CASE(ConstructionAssets_ActualNativeCanonOwnsEveryProtectedFrameWithoutWorld) {
    const auto& game=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;auto storage=MakeRawSurfaceStorage(game);AssetsNativeDevices(*storage);const auto before=std::vector<std::uint8_t>(storage->pool.Bytes().begin(),storage->pool.Bytes().end());auto& assets=storage->PlacementAssets();std::array<int,188> defaults{};
    for (const auto& asset:game.Types()) defaults.at(70+asset.block)=asset.definition.specialFrames.defaultFrame; // 실제 로딩 AST의 기본 프레임 원본 필드다.
    constexpr std::array<std::uint32_t,8> patterns{107,82,94,157,131,129,140,142}; // 보호 PE의 패턴 TYPE 전역 초기값이다.
    CHECK(&assets==&storage->PlacementAssets() && !storage->Borrowed() && storage->Generation()==0 && assets.frames.size()==188 && assets.hotspots.size()==188 && assets.shapes.size()==188 && assets.geometry.patternTypes==patterns);
    for (std::size_t type=0;type<assets.frames.size();++type) { // 원본 코드 순서와 stale 기본값까지 모든 TYPE를 대조한다.
        const auto a=assets.frames[type].frames.Codes(),b=storage->frames[type].Codes();CHECK(a.size()==b.size());if (a.size()==b.size()) CHECK(a.empty() || std::memcmp(a.data(),b.data(),a.size_bytes())==0);CHECK(assets.frames[type].defaultFrame==defaults[type]);const auto& loaded=game.TypeTable().Types()[type];CHECK(assets.hotspots[type].x==loaded.hotspotX && assets.hotspots[type].y==loaded.hotspotY);
        const auto& shape=assets.shapes[type];const auto& other=storage->shapes[type];CHECK(shape.loaded==other.loaded && shape.frameCount==other.frameCount && shape.frames.size()==other.frames.size());
        for (std::size_t index=0;index<std::min(shape.frames.size(),other.frames.size());++index) CHECK(SameAssetFrame(shape.frames[index],other.frames[index])); // 각 실제 프레임의 추가 헤더도 생략하지 않는다.
    }
    CHECK(std::equal(before.begin(),before.end(),storage->pool.Bytes().begin()) && storage->Generation()==0);
    auto* address=assets.frames.data();assets.frames[107].defaultFrame=123;{ auto lease=storage->AcquirePrepared();CHECK(&storage->PlacementAssets()==&assets && assets.frames.data()==address && assets.frames[107].defaultFrame==123); }CHECK(!storage->Borrowed());{ auto lease=storage->AcquirePrepared();CHECK(assets.frames.data()==address && assets.frames[107].defaultFrame==123); }CHECK(storage->Generation()==2);
}

// 반납 후 실제 Canon 순회가 같은 장기 진단 장치와 교체된 현재 checking DWORD를 사용한다.
TEST_CASE(ConstructionAssets_CurrentCanonDiagnosticsSurviveLeaseAndOwnerChanges) {
    std::optional<std::string> detail;std::uint32_t first{},second{},assertions{};auto storage=MakeRawSurfaceStorage(netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game);AssetsNativeDevices(*storage);storage->BindDisplayDiagnostics(detail,[&](auto expression,auto file,auto line) { CHECK(!expression.empty() && !file.empty() && line==0x325);++assertions; });storage->BindChecking(RawCheckingFlag{first});auto& assets=storage->PlacementAssets();
    const auto original=assets.frames[107].frames.Codes();std::vector<FrameCode> codes(original.begin(),original.end());for (auto& code:codes) code.flags|=0x20U; // 동일 프레임 선택 순서를 유지하며 원문의 checking 진단 입력만 바꾼다.
    assets.frames[107].frames=RiftTypeFrames(std::move(codes));{ auto lease=storage->AcquirePrepared(); }storage->BindChecking(RawCheckingFlag{second});first=1;
    const auto visit=[&] { CanonDecoder decoder(assets.frames[107].frames,TerritoryPatterns()[3],0,0,0,[] { throw std::logic_error("실제 Canon 프레임 미발견"); },OriginalEdition::Patch1078,&assets.geometry.decoderDiagnostics);static_cast<void>(decoder.Bounds(storage->types[107].footX,storage->types[107].footY)); }; // 실제 decoder가 현재 진단을 소비하게 한다.
    visit();CHECK(assertions==0);second=0x80000000U;visit();CHECK(assertions>0);const auto observed=assertions;second=0;visit();CHECK(assertions==observed && !storage->Borrowed() && &assets==&storage->PlacementAssets());
}

// 실제 Native/Source 없는 경로는 새 자산/World를 합성하지 않으며 독립 변환은 종전대로 가능하다.
TEST_CASE(ConstructionAssets_RejectMissingNativeOrSourceWithoutChangingTypedContract) {
    const auto& game=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078).game;auto typed=MakeRawSurfaceStorage(game);CHECK(AssetsRejected([&] { typed->PlacementAssets(); }));CHECK(typed->Generation()==0 && !typed->Borrowed());const PriestPlacementAssets local(game);CHECK(local.frames.size()==188);
    RawSurfaceStorage missing(OriginalEdition::Patch1078,typed->types,typed->frames,typed->shapes);AssetsNativeDevices(missing);CHECK(AssetsRejected([&] { missing.PlacementAssets(); }));CHECK(missing.Generation()==0 && !missing.Borrowed());
}
