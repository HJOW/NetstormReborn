// 설치 시의 현재 화면 명명색과 같은 Client 최초 lookup 입력을 구별하는 미등록 검사 초안이다.
#include "DudeSpawnSupport.h"
#include "client/ClientColorSortInput.h"
#include "client/PaletteColors.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include <algorithm>
#include <array>
#include <memory>
#include <stdexcept>
#include <string>

using namespace netstorm::client;
using namespace netstorm::o;
namespace {
// 기존 Theme 기술 검사의 장치 경계를 원문 그대로 재사용하며 게임 계산은 실제 Session에 맡긴다.
struct ThemeSessionServices {
    SoundList names{OriginalEdition::Patch1078}; // 세션보다 오래 사는 실제 소리 이름표다.
    std::array<std::uint8_t,4> you{'Y','o','u',0}; // 명시 영어 번역 바이트다.
    // 소리/번역/표시만 외부 경계로 두고 Player·기술·Factory·내용물 사슬은 대체하지 않는다.
    RawCampaignSessionServices Make() {
        names.Initialize();RawCampaignSessionServices result;
        result.soundNames=&names;result.unixSeconds=[] { return 946684800; };
        result.production.allocationBytes=[](auto bytes) { std::fill(bytes.begin(),bytes.end(),std::uint8_t{0x71}); };
        result.sound.wallSeconds=[] { return 100.0; };
        result.sound.play=[](SoundHandle sound,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.playLoopAt=[](float,float,SoundHandle current,SoundHandle sound) { return current ? current : sound; };
        result.sound.playOnceAt=[](float,float,SoundHandle sound,std::uint32_t,std::uint32_t) { return sound; };
        result.sound.stop=[](SoundHandle) {};result.sound.isPlaying=[](SoundHandle) { return 0U; };
        result.install.translateYou=[this] { return std::span<const std::uint8_t>(you); };
        result.install.setConfig=[](std::string_view,std::span<const std::uint8_t>) {};result.install.refreshWindowTitle=[] {};result.install.resetCamera=[] {};
        result.ownership.refreshProduction=[] {};result.ownership.themeMode=[] { return 0U; };
        result.assets.postPop.playImpactSound=[](std::uint32_t) {};result.assets.queueCommand=[](std::uint32_t,std::int32_t,std::int32_t) {};
        result.priest.shield.noticeSound=[](const PriestShieldNotice&) {};result.priest.shield.translate=[](std::string_view key) { return std::string(key); };result.priest.shield.tell=[](std::string_view) {};
        result.playerContent.gump=[](std::uint32_t) { return 0U; };
        result.playerContent.handle=[](std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t) { throw std::logic_error("기술검사의 미설치 UI 대상 처리"); };
        result.deletion.selected=[] { return Sid{}; };result.deletion.emit=[](const SquidDestroyEvent&) { throw std::logic_error("기술검사의 미선택 삭제 효과"); };
        result.player.log=[](std::uint32_t,std::string_view) {};return result;
    }
};


// 기존 독립 adapter가 사용하던 파일 색/예약 BYTE와 검정·흰색 끝점을 그대로 구성한다.
std::array<ScreenColor,256> CursorFilePalette(const GameAssets& assets) {
    std::array<ScreenColor,256> result{}; // 원래 COL의 같은 256색 번호다.
    for (std::size_t index=0;index<result.size();++index) { // 예약 BYTE를 RGB 채널에 섞지 않는다.
        const auto rgb=assets.Palette().Color(static_cast<std::uint8_t>(index)); // 원래 번호의 파일 RGB다.
        result[index]={rgb.blue,rgb.green,rgb.red,assets.Palette().Reserved(static_cast<std::uint8_t>(index)).value_or(0)};
    }
    result[0]={0,0,0,0};result[255]={255,255,255,0};
    return result;
}

// 명명색 기대에 기존 검증된 검색 몸체를 쓰기 위한 RGBX view이며 lookup 명도 재계산의 대체가 아니다.
std::array<std::uint32_t,256> CursorLogical(const std::array<ScreenColor,256>& palette) {
    std::array<std::uint32_t,256> result{}; // 현재 화면/초기 고정 입력은 호출자가 각각 구별한다.
    for (std::size_t index=0;index<result.size();++index) { // 원문 논리 R/G/B 순서만 변환한다.
        const auto& color=palette[index]; // 원래 번호의 현재 입력 항목이다.
        result[index]=std::uint32_t(color.red)|(std::uint32_t(color.green)<<8)|(std::uint32_t(color.blue)<<16)|(std::uint32_t(color.reserved)<<24);
    }
    return result;
}
}

// 같은 고정 소유자를 Session 수명까지 보유하고 getter를 연결할 때의 이전 화면을 캐시하지 않는다.
TEST_CASE(SessionCursorColorInput_CurrentNamedColorsAndSameFixedOwner) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 창 없이 읽는 보호 자산이다.
    const auto file=CursorFilePalette(assets.game); // 기존 COL adapter와 다른 현재 입력을 구별한다.
    const auto initial=CursorLogical(file); // 최초 고정 lookup 입력은 이후 변경하지 않는다.
    std::array<ScreenColor,256> changed{}; // 다음 화면은 검정에 별도 흰색/노랑 번호를 둔다.
    changed[17]={255,255,255,0};changed[19]={0,255,255,0};changed[255]={255,255,255,0};
    const auto expected=BuildPaletteColorTable(CursorLogical(changed),OriginalEdition::Patch1078); // 현재 명명색의 기존 검증 몸체다.
    const auto fileExpected=BuildPaletteColorTable(initial,OriginalEdition::Patch1078); // COL fallback과 실제 구별되는지 확인한다.
    CHECK(expected.basic[7]!=fileExpected.basic[7] || expected.basic[5]!=fileExpected.basic[5]);
    const auto* current=&file; // 서비스 설치 후 화면이 바뀌는 명시 장치 입력이다.
    std::size_t reads=0; // 최초 Cursor 연결이 현재 getter를 실제로 선택했는지 센다.
    std::weak_ptr<ClientColorSortInput> borrowed; // Session 밖 마지막 owner를 놓아 수명 공유를 확인한다.
    {
        ThemeSessionServices devices;auto services=devices.Make(); // 기존 설치용 번역/소리 경계만 공급한다.
        auto owner=std::make_shared<ClientColorSortInput>();owner->Capture(initial); // 원래 같은 장기 lookup 소유자다.
        const auto* same=owner->Logical().data(); // 입력의 주소/값 모두 유지돼야 한다.
        borrowed=owner;services.colorSortInput=owner;
        services.screenPalette=[&] { ++reads;return std::span<const ScreenColor>(*current); }; // 그때의 실제 저장소를 조회한다.
        current=&changed;
        auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable()); // 기존 성공한 실제 설치 입력이다.
        RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",std::move(services));
        owner.reset();session.Load();
        CHECK(reads==1 && !borrowed.expired());
        const auto held=borrowed.lock(); // 수명 계약 실패도 빈 포인터 역참조 없이 검사 실패로 보고한다.
        CHECK(held);if (!held) return;
        CHECK(session.Cursor().Display().TextColors()==(std::array<std::uint32_t,2>{expected.basic[7],expected.basic[5]}));
        CHECK(session.Cursor().Lifecycle().warningFont==expected.basic[7] && session.Cursor().Lifecycle().costFont==expected.basic[5]);
        CHECK(held->Logical().data()==same && std::equal(initial.begin(),initial.end(),held->Logical().begin()));
    }
    CHECK(borrowed.expired());
}

// 실제 서비스 일부 누락이나 미캡처 owner를 파일 COL 성공으로 바꾸지 않는다.
TEST_CASE(SessionCursorColorInput_RejectsPartialOrUncapturedServices) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 모든 경우에 같은 실제 보호 fort를 쓴다.
    const auto palette=CursorFilePalette(assets.game); // getter의 장기 테스트 장치 저장소다.
    for (int mode=0;mode<3;++mode) { // owner만/getter만/둘 다 있지만 미캡처인 세 실제 경계를 구별한다.
        ThemeSessionServices devices;auto services=devices.Make(); // 다른 설치 오류와 구별할 기존 장치 묶음이다.
        if (mode!=1) services.colorSortInput=std::make_shared<ClientColorSortInput>();
        if (mode!=0) services.screenPalette=[&] { return std::span<const ScreenColor>(palette); };
        auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable()); // 각 Session은 독립 설치다.
        RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",std::move(services));
        std::string rejected; // 선택되지 않은 다른 오류를 같은 실패로 세지 않는다.
        try { session.Load(); } catch (const std::logic_error& error) { rejected=error.what(); }
        CHECK(rejected==(mode==2 ? "ColorSort 최초 논리RGB 미캡처" : "캠페인 커서의 같은 ColorSort/현재 화면 색 서비스 미연결"));
    }
}

// 두 서비스를 모두 생략한 기존 headless adapter의 명명색 결과는 유지한다.
TEST_CASE(SessionCursorColorInput_PreservesExplicitIndependentColAdapter) {
    const auto& assets=netstorm::test::dudespawn::Assets(OriginalEdition::Patch1078); // 제품 정상 Client factory와 별개의 독립 검사 입력이다.
    ThemeSessionServices devices;auto fort=FortTemplate::Parse(assets.files.Read("d/bc8fury76.fort"),assets.game.TypeTable());
    RawCampaignSession session(assets.game,assets.files,std::move(fort),"bc8fury76",devices.Make());session.Load();
    const auto expected=BuildPaletteColorTable(CursorLogical(CursorFilePalette(assets.game)),OriginalEdition::Patch1078); // 원래 adapter의 명명색만 대조한다.
    CHECK(session.Cursor().Display().TextColors()==(std::array<std::uint32_t,2>{expected.basic[7],expected.basic[5]}));
}
