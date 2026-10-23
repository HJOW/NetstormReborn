#include "client/RawProductionControls.h"
#include "client/RawMinimapControls.h"
#include "client/CampaignUserInputServices.h"
// 원본 004d0120·004cebf0·004cf270·004ce580의 메뉴/명령 흐름을 표시 기반과 연결한다.
// 전체 Gump/StyleText와 게임 규칙은 후속이다. 저장 미션 월드는 GameWorld에 연결한다.
#include "client/UberGump.h"
#include "client/DialogCommandLifetime.h"
#include "client/ThemeTechnologyCommand.h"
#include "client/ProductionShortcuts.h"
#include "client/RawMenuControls.h"
#include "client/RawGameMenus.h"
#include "client/GameMenuCommands.h"
#include "client/GameMenuActions.h"
#include "client/TellParameters.h"
#include "client/ClientAudio.h"
#include "client/ClientMain.h"
#include "client/CampaignClientServices.h"
#include "client/CampaignObjectMenuClient.h"
#include "client/RawCampaignMissionStartBindings.h"
#include "client/RawCampaignTutorialOneBindings.h"
#include "client/MissionGuideInput.h"
#include "client/MissionLifecycle.h"
#include "client/GuideMissionTransitions.h"
#include "client/MissionTechnology.h"
#include "o/RawMissionBattleOptions.h"
#include "client/MissionFortRequest.h"
#include "client/CampaignTemplateStorage.h"
#include "client/TellMapMenus.h"
#include "client/RawKnowledgeMenu.h"
#include "o/RawMissionGuide.h"
#include "client/GifImage.h"
#include "client/GumpVisual.h"
#include "client/GumpBackground.h"
#include "client/GameWorld.h"
#include "client/RawCampaignCursor.h"
#include "client/RawCampaignCursorDisplay.h"
#include "client/ClientColorSortInput.h"
#include "client/CampaignCursorClientEffects.h"
#include "client/UserInput.h"
#include "client/WindowTitle.h"
#include "client/WarningMessages.h"
#include "client/DialogBodyLayout.h"
#include "client/HtmlConditionEffect.h"
#include "o/OriginalText.h"
#include "o/Template.h"
#include <algorithm>
#include <sstream>
#include <stdexcept>
#include <bit>
#include <iostream>
#include <utility>

namespace netstorm::client {
// 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 미션 진행/적 상태/기술은 바꾸지 않는다.
void UberGump::InspectBridgePatterns(std::string_view patterns) { rawButtons_->ConfigureTestBridgePatterns(patterns); }
namespace {
constexpr std::uint32_t kDialogDoNothing=0x02000089U,kStartDemo=0x020000c0U; // 보호00542918/005429f4의 원본 명령이다.
// HTML·메타·Help가 같은 현재 원본 명령 번호를 조회한다. 호스트 전환 이름을 원본 번호로 만들지 않는다.
std::uint32_t DialogScriptCode(std::string_view name) {
    const auto lower=o::AsciiLower(name);const auto mission=MissionControlScriptCode(name); // 이름과 실제 Guide 명령 표를 한 번 조회한다.
    if (lower=="tell") return 0x02000095U;
    if (lower=="donothing") return kDialogDoNothing;
    if (lower=="startdemo") return kStartDemo;
    if (lower=="config") return kScriptConfig;
    if (lower=="warning") return kScriptWarning;
    if (mission!=0xffffffffU) return mission;
    const auto knowledge=KnowledgeScriptCode(name); // 기술 창/지급도 동일 입력 큐를 사용한다.
    return knowledge!=0xffffffffU ? knowledge : GameMenuScriptCode(name);
}
// 팝업 아래에 남는 기존 UI 글자를 같은 원본 글리프 몸체로 먼저 합성한다. 월드 전용 SceneImage는 UI 글자를 생략한다.
void RasterMenuBackdropText(IndexedImage& image,std::span<const RenderText> texts) {
    const auto width=static_cast<int>(image.width),height=static_cast<int>(image.height);const ScreenRect all{0,0,width,height};
    GumpDrawState clip;GumpPixelTarget target(image.indices,width,height,clip);GumpGlyphColorState colors;const auto hooks=MakeGumpGlyphPixelHooks(target,colors);
    // 현재 각 글자의 내부 클립과 원본 출력 문맥을 그대로 적용한다.
    for (const auto& text:texts) {
        SetGumpDrawClip(clip,text.clip ? ClipScreenRect(all,*text.clip) : all);
        const GumpTextContext context{(text.shadow ? 2U : 0U)|(text.outline ? 4U : 0U),0,text.color,0,0,text.shadowColor,text.shadowOffset.x,text.shadowOffset.y};
        DrawBitmapGlyphText(text.bytes,text.font,{text.x,text.y},context,hooks,static_cast<std::int32_t>(text.bytes.size()));
    }
}
}
// 화면 검사도 GUI가 표시하는 같은 raw 월드의 삭제/프로세스 경로를 사용한다.
bool UberGump::InspectDestroySurface(std::string_view type,float x,float y) {
    if (!world_ || (type!="bridge" && type!="island" && type!="noIsland")) throw std::invalid_argument("검사할 raw 월드/표면 종류 오류");
    const auto number=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+client_.Assets().Find(type).block);
    const auto sid=world_->Surfaces().Find(x,y,number);
    if (!sid.value) throw std::out_of_range("검사할 raw 표면 없음");
    return world_->Surfaces().Destroy(sid);
}
// 검사 호출 시점만 커널로 옮기고 실제 섹션 해석·페이지 교체·정지는 일반 Tell에 맡긴다.
void UberGump::InspectTell(std::string target) { Tell(std::move(target)); }
namespace {
// 원본 메인 메뉴의 돌 버튼 폭·높이·피치와 목록 행 높이.
constexpr int kButtonWidth = 75, kButtonHeight = 19, kButtonPitch = 79, kRowHeight = 18;
// 화면 조합은 GIF/VFX의 원래 팔레트 번호를 사용한다.
void Paste(IndexedImage& canvas, const IndexedImage& image, int x, int y, ScreenRect clip) {
    DrawIndexedImage(canvas.indices, static_cast<int>(canvas.width), static_cast<int>(canvas.height), image, x, y, clip);
}
// 크기 변경 없이 화면 좌표에 맞춰 질감을 반복한다.
void Tile(IndexedImage& canvas, const IndexedImage& image, ScreenRect rect) {
    // 영역 위에서 아래로 원본 크기의 타일을 반복한다.
    for (int y = rect.top; y < rect.bottom; y += static_cast<int>(image.height)) {
        // 한 줄의 마지막 타일은 판정 영역에 맞춰 자른다.
        for (int x = rect.left; x < rect.right; x += static_cast<int>(image.width)) Paste(canvas, image, x, y, rect);
    }
}
// 화면 경계를 벗어나는 UI 선은 버퍼 경계에서 자른다.
void Fill(IndexedImage& canvas, ScreenRect rect, std::uint8_t color) {
    // 각 줄의 유효 픽셀만 채운다.
    for (int y = std::max(0, rect.top); y < std::min(static_cast<int>(canvas.height), rect.bottom); ++y) {
        const int left = std::clamp(rect.left, 0, static_cast<int>(canvas.width)), right = std::clamp(rect.right, left, static_cast<int>(canvas.width));
        std::fill(canvas.indices.begin() + y * canvas.width + left, canvas.indices.begin() + y * canvas.width + right, color);
    }
}
// 돌 가장자리의 원본 관찰 색을 현재 게임 팔레트의 가장 가까운 색으로 고른다.
std::uint8_t Color(std::span<const ScreenColor> palette, int red, int green, int blue) {
    int distance = 200000, result = 0;
    // UI 가장자리 두 색에만 적용한다. GIF 팔레트를 재배치하지 않는다.
    for (std::size_t i = 0; i < palette.size(); ++i) {
        const auto c = palette[i];
        const int next = (c.red - red) * (c.red - red) + (c.green - green) * (c.green - green) + (c.blue - blue) * (c.blue - blue);
        if (next < distance) { distance = next; result = static_cast<int>(i); }
    }
    return static_cast<std::uint8_t>(result);
}
// 눌림 여부에 따라 돌 테두리의 명암 방향을 바꾼다.
void Bevel(IndexedImage& canvas, ScreenRect r, std::uint8_t light, std::uint8_t dark, bool pressed = false) {
    if (pressed) std::swap(light, dark);
    Fill(canvas, {r.left, r.top, r.right, r.top + 1}, light); Fill(canvas, {r.left, r.top, r.left + 1, r.bottom}, light);
    Fill(canvas, {r.left, r.bottom - 1, r.right, r.bottom}, dark); Fill(canvas, {r.right - 1, r.top, r.right, r.bottom}, dark);
}
// 메뉴 파일 이름을 치환하기 전까지 cur.file 참조를 보존한다.
std::string Replace(std::string text, std::string_view from, std::string_view to) {
    std::size_t pos = 0;
    // 한 번 치환한 부분을 다시 읽지 않아 자기 참조를 만들지 않는다.
    while ((pos = text.find(from, pos)) != std::string::npos) { text.replace(pos, from.size(), to); pos += to.size(); }
    return text;
}
// 설정의 UTF-8 문자열을 원본 글꼴의 Windows-1252 바이트로 되돌린다.
std::string FontBytes(std::string_view text) {
    const auto encoded = o::EncodeOriginalText(text); return std::string(encoded.begin(), encoded.end());
}
// 현재 단계에서 실제 처리 경로가 있는 메뉴 명령만 활성화한다.
bool Supported(std::string_view command) {
    const auto name = o::AsciiLower(command);
    return MissionControlScriptCode(name)!=0xffffffffU || KnowledgeScriptCode(name)!=0xffffffffU || GameMenuScriptCode(name)!=0xffffffffU || name == "tell" || name == "missionbegin" || name == "startdemo" || name == "donothing" ||
        name == "quit" || name == "menu" || name == "toggle" || name == "resolution" || name == "volume" || name == "resume";
}
// 자동 검사 보고에는 개행/탭을 제거하여 한 항목 한 줄을 유지한다.
std::string Field(std::string text) { std::replace(text.begin(), text.end(), '\t', ' '); std::replace(text.begin(), text.end(), '\n', ' '); std::replace(text.begin(), text.end(), '\r', ' '); return text; }
}

// 원본 Mission의 같은+68/+70/+88 상태를 실제 Start와 후속 프레임 소비자에 보존한다.
struct UberGump::MissionRuntime {
    std::uint32_t saveToken; // 같은 Runtime의 이름/필드를 식별하는 호스트 수명 토큰이며 원본 게임 규칙 값이 아니다.
    TutorialInitState init; // 원본+4/+b0와 Init 지역 문자열의 현재 입력이다.
    o::MissionFrameGateState gate; // demo/started/시각/안내 단계의 유일한 현재 미션 저장소다.
    o::MissionStartState state; // Start의 순번/현재 tellAt 항목/필드를 동일 수명으로 유지한다.
    std::unique_ptr<RawCampaignMissionStartBindings> start; // 단계1의 실제 Session 설치 뒤 생성하고 단계3에서만 실행한다.
    std::uint32_t mode64{}; // 원본 Mission+64의 현재 값이다. 일반 진입은0이며 Reload는 해제 전에 bool로 캡처한다.
    bool guided{}; // 실제 Tutorial 가상표를 사용하는 미션만 Guide0..6에 분배한다. 기존 Battle 진입은 별도다.
    std::unique_ptr<RawCampaignTutorialOneBindings> tutorial; // 지정11개를 포함한 Tutorial 형식 전체의 공통 프레임/입력과 선택된 Early 판정이다.
    // 원본00482bf0의 초기 demo0은 일반 MissionBegin,00463a00의 직접 쓰기는 StartDemo가 공급한다.
    MissionRuntime(MissionScript& script,std::uint32_t demo,std::uint32_t mode,std::uint32_t token)
        :saveToken(token),mode64(mode),guided(o::AsciiLower(script.MissionType())=="tutorial") {
        gate.demo=demo;init.name=FontBytes(script.Name()); // 번호/demo/이름 정규화는 실제 단계1까지 앞당기지 않는다.
    }
};

// 원본 GIF는 색 표의 RGB를 쓰지 않고 파일의 번호를 화면 팔레트에 바로 쓴다(00419ec0→004dae60).
UberGump::UberGump(Client& client) : client_(client) {
    serializationGlobals_=std::make_shared<o::FortSerializationGlobals>(); // 원본BSS와같은한DWORD를모든manager/객체소비자보다오래소유한다.
    templateStorage_=std::make_shared<CampaignTemplateStorage>(serializationGlobals_); // 시작 초기화와 미션별 명시 정리를 별개 수명으로 유지한다.
    auto& config = client_.Configuration();
    SoloMapCatalogEffects mapEffects; // 현재 실제 장치만 제공하며 목록 분류/행 처리는 기존 몸체를 사용한다.
    mapEffects.serializationGlobals=serializationGlobals_; // 목록의두임시MapProbe도실제manager생성자의같은전역0쓰기를공유한다.
    mapEffects.typeCount=[this] { return static_cast<std::int32_t>(client_.Assets().TypeTable().Types().size()); };
    mapEffects.allocationBytes=[](auto) { throw std::logic_error("맵 목록의 빈 State 생성에 필요한 현재 미정 할당 바이트 미연결"); }; // 정상 보호 파일은 이 경계를 선택하지 않는다. 미정 바이트를 임의0으로 만들지 않는다.
    mapEffects.userName=[this] { return world_ ? std::string(world_->Session().CurrentHandle()) : mapUserName_; };
    mapEffects.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
    mapEffects.fileSpecKey=[this] { return mapFileSpec_.Current(); };
    mapCatalog_=std::make_unique<SoloMapCatalog>(client_.Files(),config,std::move(mapEffects));
    title_ = DecodeGif(client_.Files().Read(config.PathSpec("BackgroundGifSpec", "titleMenu")));
    clouds_ = DecodeGif(client_.Files().Read(config.PathSpec("CloudFileSpec")));
    const auto& type = client_.Assets().Find("fortGump");
    // 원본 타입의 클러스터 이름으로 질감·모서리·체크 표시를 고른다.
    for (const char* name : {"A00", "A01", "I00", "I01", "I02", "I03", "J02"}) {
        const auto frame = client_.Assets().FrameIndex(type, name, 0);
        decorations_.emplace(name, client_.Assets().Shapes().Decode(type.block, frame));
        // 원본 버튼 생성자004254b0은 전역00542664의 프레임1(A01 Sandy marble)을 배경 자식에 전달한다.
        if (std::string_view(name)=="A01") {
            buttonTextureType_=static_cast<std::uint32_t>(o::kFirstAssetTypeNumber+type.block);
            buttonTextureFrame_=static_cast<std::uint32_t>(frame);
            buttonTextureMetrics_=client_.Assets().Shapes().SquidMetrics(type.block,frame);
            const auto rect=client_.Assets().Shapes().Blocks()[type.block].frames[frame].rect;
            buttonTextureOffset_={rect.left,rect.top};
        }
    }
    rawButtons_=std::make_unique<RawButtonControls>(client_.Assets().Edition(),RawButtonHooks{
        [this] { return client_.WallSeconds(); },[this](auto code) { return client_.Poll(code); },
        [this] { if (client_.Audio()) client_.Audio()->Sounds().PlayByName("button.wav",0,0,0,0,0); },
        [this](std::uint32_t form,std::uint32_t flags) {
            if (!world_ || form>65535U) throw std::logic_error("현재Client UI Form삭제의월드/SID오류");
            world_->Session().GumpForms().Destroy(o::Sid{static_cast<std::uint16_t>(form)},flags);
        }});
    rawButtons_->ConfigureTestBridgePatterns(client_.TestBridgePatterns()); // 테스트 전용 임시 조작이며 원본 분석 결과가 아니다. 첫 조각 생성 전에 명시 CLI만 반영한다.
    if (!client_.TestBridgePatterns().empty()) std::clog<<"테스트 전용 다리 조각 순서: "<<client_.TestBridgePatterns()<<" (원본 분석 결과 아님)\n";
    RawMenuHooks menuHooks;
    menuHooks.command=[this](auto command,auto id,auto parameter) {
        if (command==0x01ffffffU) {
            if (!parameter || parameter>actions_.size()) throw std::logic_error("Client 팝업 DialogAction 연결 번호 오류");
            state_.Post(actions_[parameter-1]); // 기존 DialogAction의 임시 호스트 번호이며 원본 게임 명령 토큰과 구별한다.
        } else {
            // 실제 메뉴 항목이 선택한 미연결 명령은 여기서 거부한다. 돌 버튼의 별도 기본 클릭 통지와 구별한다.
            if (command!=0x02000060U && command!=0x03000080U && command!=0x03000032U && command!=0x03000063U && command!=0x0200005dU && command!=0x02000017U && command!=0x030000afU && command!=0x030000e2U && command!=0x03000015U && command!=0x0300011aU && command!=0x03000076U && !IsGameMenuCommand(command)) throw std::logic_error("현재 Client 원본 메뉴 항목 명령 미연결: "+std::to_string(command));
            client_.Input().PushCommand(command,std::bit_cast<std::int32_t>(id),std::bit_cast<std::int32_t>(parameter));
        }
    };
    menuHooks.readInt=[this](std::string_view key,std::int32_t& value) { value=client_.Configuration().GetInt(key); };
    menuHooks.sound=[this](std::string_view name) { client_.WorldAudio().Sounds().PlayByName(name,0,0,0,0,0); };
    menuHooks.poll=[this](auto code) { return client_.Poll(code); };menuHooks.cursor=[this](auto number) { client_.GetCursor().Set(static_cast<int>(number)); };
    menuHooks.objectForms=[this]() -> o::GumpFormHost& { if (!world_) throw std::logic_error("현재Client 메뉴Form의월드미설치");return world_->Session().GumpForms(); };
    menuHooks.gameSeconds=[this] { return client_.Time().game; }; // 비용 표시도 같은 프레임의 게임 시각을 읽는다.
    rawButtons_->EnableMenus(client_.Assets(),client_.Fonts(),std::move(menuHooks)).SetPalette(client_.GetScreen().Colors());
    RawHelpHooks help;help.globals=&htmlGlobals_; // Tell과 같은 HTML 전역/Config/입력 고리를 빌린다.
    help.translate=[this](auto text) { return FontBytes(client_.Translate(text)); };help.config=[this](auto key) { return FontBytes(client_.Configuration().PathSpec(key)); };
    help.substitute=[this](auto text) { const auto decoded=o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()});return FontBytes(client_.Configuration().SubstituteLocal(decoded)); };
    help.load=[this](auto name) -> std::optional<std::string> { const auto bytes=client_.Files().TryRead(name);if (!bytes) return std::nullopt;return std::string(bytes->begin(),bytes->end()); };
    help.text=[this](auto token) { return std::string(scriptStrings_.Resolve((token&0xf0000000U)==0x70000000U ? token&0x0fffffffU : token)); };
    help.storeText=[this](auto text) { const auto token=scriptStrings_.Store(text);if (token>0x0fffffffU) throw std::overflow_error("Help 문자열 별칭 토큰 소진");return 0x70000000U|token; }; // 원본 주소와 달리101부터인 고리 토큰이 TYPE<256 관문에 걸리지 않도록 같은 수명에 별칭을 준다.
    help.typeName=[this](auto type) { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("Help 현재 TYPE 이름 범위");return types[type].name; };help.lookup=[this](auto name) { return client_.Assets().TypeTable().Find(name); };
    help.type=[this](auto type) { return world_ ? world_->Session().HelpTypeInfo(type) : ReadAssetQuickHelpType(client_.Assets(),type); };
    help.description=[this](auto type) { const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("Help 현재 TYPE 설명 범위");return FontBytes(client_.Translate(types[type].description)); };
    help.hitPoints=[this](auto type) { if (world_) return world_->Session().HitPoints().TypeHitPoints(type);const auto types=client_.Assets().TypeTable().Types();if (type>=types.size()) throw std::out_of_range("월드 설치 전 Help HP 범위");return types[type].maxHitPoints; };
    help.helpFrame=[this](auto type) { const auto types=client_.Assets().Types();if (type<o::kFirstAssetTypeNumber || type-o::kFirstAssetTypeNumber>=types.size()) return -1;return types[type-o::kFirstAssetTypeNumber].definition.specialFrames.helpFrame; };help.nearestColor=[this](auto r,auto g,auto b) { return client_.GetScreen().FindColor(r,g,b); };
    help.queue=[this](auto command,auto first,auto second) { const auto x=scriptStrings_.Store(first),y=scriptStrings_.Store(second);client_.Input().PushCommand(command,std::bit_cast<std::int32_t>(x),std::bit_cast<std::int32_t>(y)); };
    help.script=[this](auto text) { QueueScriptCommand(text,client_.Input(),scriptStrings_,{DialogScriptCode,[](auto name) { std::clog<<"도움말 스크립트 명령 미연결: "<<name<<'\n'; }}); };
    help.external=[](auto text) { throw std::logic_error("도움말 외부 OS 프로토콜 미복원: "+std::string(text)); };help.learn=[](auto type) { throw std::logic_error("도움말 기술 학습492a30 미연결: "+std::to_string(type)); };
    help.scripted=[this] { return state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing; };help.scriptName=[this] { return FontBytes(pageName_); };
    help.battle=[this] { return world_ && world_->Session().Core().ownerMode.battle; };help.challenge=[this] { return world_ && world_->Session().Core().ownerMode.challenge; };
    help.html.command=[this](auto&,auto text) { QueueScriptCommand(text,client_.Input(),scriptStrings_,{KnowledgeScriptCode,[](auto name) { std::clog<<"도움말 HTML 명령 미연결: "<<name<<'\n'; }}); };
    BindHtmlConditionConfig(help.html,client_.Configuration(),client_.Assets().Edition());BindHtmlPictureText(help.html);
    help.html.background=[](auto&,auto name) { throw std::logic_error("도움말 배경 태그 미복원: "+std::string(name)); };
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) rawButtons_->Menus()->EnableHelp(std::move(help)); // 10.72에10.78 전용 이미지/창을 임의로 설치하지 않는다.
    objectMenus_=std::make_unique<CampaignObjectMenuClient>(client_,*rawButtons_,[this](std::string_view key) { state_.Post({"Tell",std::string(key)}); });
    MessageGumpHooks pauseContext; // 같은 현재 Session의 모드/이름과 Client 번역을 Pause 생성 시 조회한다.
    pauseContext.challenge=[this] { return world_ && world_->Session().Core().ownerMode.challenge ? 1U : 0U; };
    pauseContext.playerName=[this](std::int32_t owner) {
        if (owner<=0 || owner>=9) return; // 원본48f480은 범위 밖에서 빈 문자열 포인터만 반환한다.
        if (!world_) throw std::logic_error("Pause 이름 조회의 현재 Session 없음");
        static_cast<void>(world_->Session().PlayerRecords().subspan(static_cast<std::size_t>(owner)*180U+12U)); // 반환 이름은 종류11에서 읽지 않는다.
    };
    pauseContext.challengePlayerName=[](auto) { throw std::logic_error("Challenge 메시지 현재 이름 관리자 미연결"); };
    pauseContext.translate=[this](auto value) { return FontBytes(client_.Translate(value)); };
    warnings_=std::make_unique<WarningMessages>(client_.Fonts(),client_.GetScreen().Colors(),[this] { return client_.WallSeconds(); },[this] {
        client_.WorldAudio().Sounds().PlayByName("newWarnMessage.wav",0,0,0,0,0);
    },std::move(pauseContext));
    dialogBody_=std::make_unique<DialogBodyLayout>(client_.Fonts(),client_.GetScreen().Colors(),client_.Assets(),client_.Assets().Edition());
    o::MissionGuideHooks guideHooks; // 같은 실제 현재 미션/전환 큐/창과 복원한 Guide 반복 몸체다.
    guideHooks.current=[this]() -> o::MissionStartState* { return missionRuntime_ && missionRuntime_->guided ? &missionRuntime_->state : nullptr; };
    guideHooks.pending=[this] { return state_.Pending() ? 1U : 0U; };
    guideHooks.dialogsClear=[this] { return !pageAlive_; }; // Kill은 즉시 모달을 반납하지만 GuideOpen 개수는 Cleanup까지 남는다.
    guideHooks.stage=[this](o::MissionStartState& state,std::uint32_t sequence) {
        TutorialMissionStageHooks hooks; // 현재 단계가 실제로 도달한 소비자만 호출한다.
        hooks.initialize=[this] { InitializeMission(); };hooks.start=[this] { StartMissionWorld(); };
        hooks.tellGlobal=[this](std::string_view name,std::uint32_t mode) {
            if (mode!=1) throw std::logic_error("Guide init Tell의 원본 인자 위반");static_cast<void>(TryTell(std::string(name)));
        };
        hooks.frame=[this] { if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("Guide 프레임의 실제 미션 연결 누락");missionRuntime_->tutorial->Frame(); };
        hooks.showStage=[this] { if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("Guide First의 현재 안내 연결 누락");missionRuntime_->tutorial->TellStage(); };
        hooks.tellMission=[this](std::string_view section) { bool created=false;return mission_ && mission_->Section(section) && TryTell(std::string(section),false,&created) && created ? 1U : 0U; };
        hooks.closeTransient=[this] { rawButtons_->RequestClosePopups(); };
        hooks.pause=[this] { if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("Guide 안내의 실제 자기 정지 연결 누락");missionRuntime_->tutorial->Gate().PauseForDialog(); };
        RunTutorialMissionStage(state,sequence,hooks);
    };
    guideHooks.postpone=[this](o::MissionStartState&) {
        if (!missionRuntime_) throw std::logic_error("Guide 재지연의 현재 미션 없음");
        if (missionRuntime_->tutorial) missionRuntime_->tutorial->Gate().Postpone();
        else if (missionRuntime_->gate.nextStageSeconds!=0.0) missionRuntime_->gate.nextStageSeconds=static_cast<double>(missionRuntime_->gate.delaySeconds)+client_.Time().game; // 설치 전 기본0은 그대로 유지한다.
    };
    guideHooks.release=[this] { MainMenu();if (client_.Audio()) client_.Audio()->StartScene(false); };
    guideHooks.invalidStage=[] { throw std::logic_error("Guide 현재 미션 순번0..6 위반"); };
    if (client_.Assets().Edition()==o::OriginalEdition::Patch1078) guide_=std::make_unique<o::RawMissionGuide>(client_.Assets().Edition(),std::move(guideHooks)); // Guide 몸체의 복원 판본은10.78이며 CD 메인 메뉴 생성은 기존대로 둔다.
    MainMenu(); Compose(); client_.ShowScene();
}
// 입력 참조를 먼저 없애고 커널이 소유한 월드를 이 구현 파일에서 제거한다.
UberGump::~UberGump() { ClearWorld();DisposeRetiredPages(); }
// 입력 참조를 먼저 없애고 커널의 실제 소유권을 해제한다. 위치 효과음은 카메라 0의 전체 메뉴 영역으로 되돌린다.
void UberGump::ClearWorld() {
    ClearPage(); // 현재 부모의 OnExit를 월드/미션 수명보다 먼저 같은 큐에 반납한다.
    if (world_) mapUserName_=world_->Session().CurrentHandle(); // 원본 Client 전역 이름은 월드를 해제한 뒤에도 다음 목록 조회에 남는다.
    state_.Cancel("LoadMissionFort"); // 현재 미션이 설치 전에 교체돼도 그 소유자의 이전 요청을 새 미션에서 소비하지 않는다.
    state_.Cancel("LoadMissionWorld"); // 비 Tutorial의 지연 설치도 같은 소유자 해제 때 취소한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseMissionLoading(); // 로딩 중 이탈한 부모를 다음 장면에 남기지 않는다.
    missionFortRequest_.reset(); // Session이 살아 있으면 공유 소유권으로 실제 설치/해제 완료까지 이미지를 보존한다.
    if (rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->CloseHelp(); // 동적 정보/그림의 현재 Session보다 먼저 실제 창을 반납한다.
    if (world_ && rawButtons_ && rawButtons_->Menus()) rawButtons_->Menus()->Close(); // 같은Form을반납한뒤에만Session/월드포인터를해제한다.
    gameMenuState_.reset(); // 같은Session의검산참조를월드보다먼저반납한다.
    if (rawButtons_) rawButtons_->DisableMinimap(); // 같은Session/공통Gump보다먼저공간통지와미니맵을반납한다.
    if (rawButtons_) rawButtons_->DisableProduction(); // 실제 Session보다 먼저 UI Form을 반납한다.
    client_.ClearThunderFlashes();
    if (world_) client_.GetRenderer().SetExtraCollection({}); // 월드 반납 전에 빌린 실제 커서 수집/Draw callback을 해제한다.
    client_.GetRenderer().SetOverlayProcedure({}); // 실제 메뉴 픽셀 소비자가 반납한 월드를 다시 읽지 않도록 먼저 제거한다.
    if (objectMenus_) objectMenus_->Clear();
    if (missionRuntime_ && missionRuntime_->gate.scriptPaused) {
        client_.PopGamePause();missionRuntime_->gate.scriptPaused=0; // 미션이 보유한 자기 깊이를 반납한 뒤 참조를 해제한다.
    }
    if (missionRuntime_ && missionRuntime_->guided) { o::SetTotalMadeAllowAll(*missionRestrictions_,1);missionNumber_=0;missionRuntime_->gate.tutorialNumber=0;missionRuntime_->gate.stage=0;missionRuntime_->gate.stageDirty=0; } // 실제 Tutorial 소멸의 허용/번호/단계 해제를 전역 수명에도 유지한다.
    missionRuntime_.reset(); // 같은 Session/Script를 참조하는 Start를 대상보다 먼저 해제한다.
    userInput_.reset();
    const auto process=worldProcess_; // 해제 중의 현재 subscriber 조회가 파괴 중인 월드를 따라가지 않도록 번호만 보관한다.
    world_=nullptr; worldProcess_=0;
    if (process) client_.GetKernel().Remove(process);
    client_.WorldAudio().SetView({0,0,0,0,client_.GetScreen().Width(),client_.GetScreen().Height()});
}
// 현재 어댑터의 월드 수명으로 원본 번개 부모 조회 경계를 제공한다.
bool UberGump::HasWorld() const { return world_!=nullptr; }
// 별도 로그인 값 없이 현재 월드의 같은 Session 전역을 직접 조회한다.
std::uint32_t UberGump::CurrentSubscriber() const { return world_ ? world_->CurrentSubscriber() : 0U; }
// 기본 메뉴를 초기화한 다음 프레임에 미션을 시작한다.
void UberGump::StartMission(std::string name) { state_.Post({"MissionBegin",std::move(name)}); }
// 캠페인 이름의 설정 객체를 보존하여 officalN.title 같은 참조를 복원한다.
o::Config& UberGump::Script(std::string_view name) {
    const auto key = o::AsciiLower(name);
    if (const auto found = scripts_.find(key); found != scripts_.end()) return *found->second;
    auto script = std::make_unique<o::Config>(client_.Configuration().Registry(), key);
    const auto path = client_.Configuration().PathSpec("missionSpec", key);
    script->LoadBytes(client_.Files().Read(path)); script->Set("fileName", key);
    const auto [it, inserted] = scripts_.emplace(key, std::move(script)); (void)inserted;
    return *it->second;
}
// @/% 목록은 원본 파일 이름으로 정렬하고 '--' 제목은 선택할 수 없는 구분선으로 둔다.
void UberGump::ExpandMenus(DialogPage& page) {
    std::vector<DialogItem> items;
    // 일반 항목은 유지하고 파일 목록 항목만 확장한다.
    for (auto item : page.items) {
        if (item.menu && !item.label.empty() && (item.label[0] == '@' || item.label[0] == '%')) {
            // 빈 사용자 목록은 빈 메뉴다. 원본에 없는 캠페인을 만들지 않는다.
            for (const auto& path : client_.Files().Match(item.label.substr(1))) {
                const auto name = std::filesystem::path(path).stem().string(); auto& script = Script(name);
                const auto title = script.Get(name + ".title").value_or(name);
                auto entry = item; entry.label = title; entry.action.argument = Replace(item.action.argument, "{cur.file}", name);
                entry.action.parameter=scriptStrings_.Store(FontBytes(entry.action.argument)); // 각 실제 파일 행은 자기 이름의 새 인자 수명을 소유한다.
                entry.enabled = !title.starts_with("--") && Supported(entry.action.command); items.push_back(std::move(entry));
            }
        } else { item.enabled = item.enabled && Supported(item.action.command); items.push_back(std::move(item)); }
    }
    page.items = std::move(items);
}
// 목록을 전체 파싱 뒤 다시 찾지 않고 실제 메타 추출 순서에서 현재 파일/이름/열을 읽는다.
DialogMetadataHooks UberGump::MapMetadata() {
    DialogMetadataHooks metadata;
    metadata.appendMenu=[this](DialogPage& page,DialogItem item) {
        const auto current=mission_ ? mission_->FortName() : std::string(); // 현재 설치된 미션 Fort 이름과 별도 pending 이름의 수명을 구별한다.
        const auto begin=[&](auto flags) { page.menuColumnFlags.push_back(flags); }; // 표시 어댑터의 실제 열 생성 자료를 현재 페이지에 소유한다.
        const auto hooks=MakeTellMapMenuHooks(*mapCatalog_,mapFileSpec_,[&]() -> std::string_view { return current; },
            [this]() -> std::string_view { return pendingFortName_; },begin,[&](const auto& row,auto command) {
                if (command!=kReadFortCommand) throw std::logic_error("Tell 맵 행의 원본 ReadFort 명령 불일치");
                const auto decode=[](std::string_view bytes) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); }; // 원본 좁은 파일/라벨을 기존 페이지의UTF-8 경계로 옮긴다.
                page.items.push_back({decode(row.label),{"ReadFort",decode(row.name),kReadFortCommand,scriptStrings_.Store(row.name)},true,row.checked,row.enabled,false,page.menuColumns-1});
            });
        if (item.checkedDirective) {
            if (!page.menuColumns) { begin(kTellMenuFlags);++page.menuColumns; }
        } else if (AppendTellMapMenus(item.label,page.menuColumns,hooks)) return;
        item.menuColumn=page.menuColumns-1;page.items.push_back(std::move(item));
    };return metadata;
}
// 실제 부모가 반납되는 변경에서만 호출하며 Resize/Compose는 이 수명을 건드리지 않는다.
void UberGump::ClearPage() {
    if (pageAlive_) {
        retiredPageExits_.push_back(pageExit_); // OnExit0인 부모도 Cleanup까지005cab2c의 개수를 유지한다.
        rawButtons_->RetirePageControls();input_.SetControls({}); // 교체 Tell/명시 Kill 모두 같은 드레인에서 옛 부모 입력을 즉시 반납한다.
    }
    pageAlive_=false;pageExit_={};page_={}; // 같은 부모를 두 번 반납하지 않고 실제 예약은 입력 드레인 뒤로 남긴다.
}
// c79b0의 선택 예약→Kill 순서는 일반·Instant 버튼과 메뉴 종료·timeout에 공통이다.
void UberGump::ActivatePageAction(DialogAction action) {
    if (!pageAlive_ || !popup_.empty()) { state_.Post(std::move(action));return; } // 실제 HTML 외 임시 화면 전환의 기존 경계다.
    if (action.commandCode==0xffffffffU) throw std::logic_error("현재 HTML 선택의 메타 명령 해석 누락: "+action.command);
    FireDialogCommand(1,{action.commandCode,action.parameter},client_.Input(),[this](std::uint32_t) {
        ClearPage();popup_.clear();state_.phase=mission_ ? ClientPhase::Mission : ClientPhase::MainMenu;pageName_=mission_ ? "mission" : "main";
        if (!(missionRuntime_ && missionRuntime_->tutorial)) client_.Pause(false); // Guide 자기 정지는 원본7프레임 게이트가 반납한다.
        RebuildControls();
    });
}
// 원본의 부모 Kill과 소멸을 구별하여 OnExit가 현재 입력 루프 안에서 바로 재실행되지 않게 한다.
void UberGump::DisposeRetiredPages() {
    auto retired=std::exchange(retiredPageExits_,{}); // 예약 콜백 전에 현재 반납 표를 비운다.
    for (auto& exit:retired) DisposeDialogExitCommand(exit,client_.Input()); // 부모 반납 순서대로 같은40칸 큐에 한 번씩 쓴다.
}
// 명시 사용자 명령에서 잘못된 섹션은 기존 진단을 유지한다. 선택적 미션 사건은 TryTell을 사용한다.
void UberGump::Tell(std::string target, bool briefing) {
    bool created=false; // 섹션 존재와 실제 창 생성(serial 비제로)을 구별한다.
    if (!TryTell(target,briefing,&created)) throw std::runtime_error("Missing dialog section: "+target);
    if (created && missionRuntime_ && missionRuntime_->tutorial)
        missionRuntime_->tutorial->Gate().PauseForDialog(); // 명시 Tell도 같은 자기 깊이만 보유한다.
}
// 원본004cf730은 phase와 무관하게 현재 Mission의 설정에서 전체 target을 조회한다.
bool UberGump::TryTell(std::string target,bool briefing,bool* windowCreated) {
    if (windowCreated) *windowCreated=false; // 무본문/빈 Tell은 창 생성 성공으로 보고하지 않는다.
    if (o::AsciiLower(target) == "blank" || target.empty()) {
        if (mission_) { state_.phase = ClientPhase::Mission; pageName_ = "mission"; popup_.clear(); ClearPage(); client_.Pause(false); }
        else MainMenu();
        RebuildControls(); return true;
    }
    o::Config parameters(client_.Configuration().Registry(),"param"); // 원본지역param은즉시본문처리와두HTML순회가끝날때까지같은Registry에남는다.
    const auto original=ParseTellParameters(FontBytes(target),TellParameterHooks{
        [this](std::string_view value) { const auto decoded=o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});return FontBytes(client_.Configuration().SubstituteLocal(decoded)); },
        [&parameters](std::string_view line) { parameters.Append(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(line.data()),line.size()})); }}); // 원문치환과Append사이에인용escape를끼우지않는다.
    target=o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(original.data()),original.size()});
    std::optional<std::string> body;
    const auto split = target.find('.');
    if (split != std::string::npos && split + 1 < target.size()) {
        auto& script = Script(target.substr(0, split));
        if (const auto section = script.Section(target.substr(split + 1))) body = std::string(*section);
    }
    if (!body && mission_) body = mission_->Section(target);
    if (!body) {
        if (const auto section = Script("tell").Section(target)) body = std::string(*section);
    }
    if (!body) return false;
    const auto protectedBody = Replace(*body, "{cur.file}", "__CURRENT_FILE__");
    ScriptCommandHooks commands; // 현재복원된실제명령을원본ID로조회하고미등록은원본처럼진단뒤거부한다.
    commands.lookup=DialogScriptCode;
    commands.unknown=[](std::string_view name) { std::clog<<"스크립트 명령 미연결: "<<name<<'\n'; };
    HtmlTextParserState htmlState;HtmlTextParserHooks htmlHooks;std::string background; // 원본처럼새페이지는shown=1로시작하고두번의HTML순회에서같은상태를공유한다.
    htmlHooks.background=[&](HtmlTextParserState&,std::string_view name) { background=name; }; // 두HTML순회가선택한실제배경이름을현재페이지표시경로에보관한다.
    htmlHooks.command=[&](HtmlTextParserState&,std::string_view raw) { QueueScriptCommand(FontBytes(raw),client_.Input(),scriptStrings_,commands); }; // 원본1차HTML의선택시점에기존40칸입력큐로예약한다.
    BindHtmlConditionConfig(htmlHooks,client_.Configuration(),client_.Assets().Edition()); // 초기전체치환결과의조건토큰을그시점의현재Config에서다시치환한다.
    BindHtmlPictureText(htmlHooks); // 원본인용그림표기를실제Markup/SHP를읽는본문버퍼에전달한다.
    const auto expanded=Replace(client_.Configuration().SubstituteLocal(protectedBody),"__CURRENT_FILE__","{cur.file}"); // 원본004cf270의선행전체치환을HTML조건평가보다먼저유지한다.
    DialogExitCommandState parsedExit;auto metadata=MapMetadata();const auto exitHooks=MakeDialogExitResolveHooks(scriptStrings_,commands); // 새 부모의 메타 수명은 기존 표시 부모와 구별한다.
    metadata.resolveRuntimeAction=[&](DialogAction& action,std::string_view command,std::string_view argument) {
        DialogExitCommandState selected;ResolveDialogExitCommand(selected,FontBytes(command),FontBytes(argument),exitHooks); // 같은 생성 시점의 명령/숫자·고리 해석이다.
        action.commandCode=selected.command;action.parameter=selected.parameter;
    };
    metadata.resolveOnExit=[&](std::string_view command,std::string_view argument) { ResolveDialogExitCommand(parsedExit,FontBytes(command),FontBytes(argument),exitHooks); }; // 원문 캡처는 Trim/따옴표 해제 없이 원본 바이트로 전달한다.
    auto parsed=ParseDialogPage(expanded,htmlState,htmlGlobals_,htmlHooks,client_.Assets().Edition(),std::move(metadata)); // 현재 목록/열도1차HTML뒤 실제 메타 추출 시점에 처리한다.
    if (!parsed.hasContent) { if (parsedExit.command) throw std::logic_error("내용 없는 HTML 부모의 OnExit 메타 관문 위반");retiredPageExits_.push_back({});rebuild_=true;return true; } // 원본cf120→Kill 임시 부모는 Cleanup까지 세지만 기존 표시 부모를 닫지 않는다.
    const std::array<std::uint32_t,8> context{0,0,client_.GetScreen().Colors().basic[7],0,0,0,1,1}; // 실제기본TextGump의font0/normal/흰색/그림자오프셋문맥이다.
    dialogBody_->SetBody(FontBytes(parsed.htmlBody),context,htmlState,htmlGlobals_,std::move(htmlHooks));
    parsed.background=std::move(background); // 같은상태의2차HTML이선택한최종배경을실제표시에사용한다.
    ClearPage();page_=std::move(parsed);pageExit_=parsedExit;pageAlive_=true;
    ExpandMenus(page_);
    if (page_.items.empty()) page_.items.push_back({"OK", {"DoNothing", "0",kDialogDoNothing,0}});
    state_.phase = briefing ? ClientPhase::Briefing : ClientPhase::Dialog;
    pageName_ = target; popup_.clear(); opened_ = client_.Time().wall; RebuildControls();
    if (mission_ && !(missionRuntime_ && missionRuntime_->tutorial)) client_.Pause(true);
    if (windowCreated) *windowCreated=true;
    return true;
}
// 메인 메뉴로 돌아올 때 미션 설정 객체도 등록 목록에서 제거한다.
void UberGump::MainMenu() {
    client_.GetRenderer().SetScene({}); client_.GetRenderer().SetBackground(nullptr);
    client_.GetRenderer().SetOverlay(nullptr);
    ClearWorld(); mission_.reset(); fortObjects_ = 0; briefingSections_.clear(); briefingIndex_ = 0;
    state_.phase = ClientPhase::MainMenu; pageName_ = "main"; popup_.clear(); ClearPage(); input_.Cancel();
    client_.Pause(false); RefreshWindowTitle(); RebuildControls();
}
// 현재 미션의 원본 제목 설정을 적용한다. 데모 모드는 같은 미션 객체의 직접 기록을 읽는다.
void UberGump::RefreshWindowTitle() {
    WindowTitleState state; // 현재 메뉴는 요새 없음이며 데모 여부는 같은 실제 미션 필드다.
    if (mission_) { state.fortName=mission_->FortName();state.mission=true; }
    if (missionRuntime_) state.demo=missionRuntime_->gate.demo!=0;
    netstorm::client::RefreshWindowTitle(state,{
        [this](std::string_view key,std::optional<std::string_view> argument) { return client_.Configuration().PathSpec(key,argument); },
        [this] { return mission_ ? mission_->Get("title").value_or(std::string()) : std::string(); },
        [this](std::string_view title) { client_.Title(title); }});
}
// 원본 Begin의 현재 finish/release 뒤 새 빈 미션을 만든다. StartDemo의 직접 지정값도 설치 전에 공급한다.
void UberGump::BeginMission(std::string name,std::uint32_t demo,std::uint32_t mode) {
    auto effects=MissionTransitionEffects();effects.create=[this,demo,mode](std::string_view target) { CreateMission(std::string(target),demo,mode); };
    BeginGuideMission(name,effects);
}
// Guide 공장의 현재 Client 소비자다. 이전 미션 해제는 호출자가 원본 순서로 마친 상태여야 한다.
void UberGump::CreateMission(std::string name,std::uint32_t demo,std::uint32_t mode) {
    if (mission_ || missionRuntime_ || world_) throw std::logic_error("Guide 새 미션 공장 전에 기존 현재가 남아 있음");
    mission_ = std::make_unique<MissionScript>(client_.Configuration(), name);
    if (!mission_->Loaded()) throw std::runtime_error("Missing mission: " + name);
    const auto type = o::AsciiLower(mission_->MissionType());
    if (type != "tutorial" && type != "battle") throw std::runtime_error("Mission type pending: " + mission_->MissionType());
    if (nextMissionSaveToken_==0xffffffffU) throw std::overflow_error("Mission 저장 수명 토큰 소진");
    missionRuntime_=std::make_unique<MissionRuntime>(*mission_,demo,mode,++nextMissionSaveToken_);
    missionRuntime_->guided=missionRuntime_->guided && client_.Assets().Edition()==o::OriginalEdition::Patch1078; // 복원하지 않은 판본의 순번을 새 Guide에 분배하지 않는다.
    briefingSections_.clear();briefingIndex_=0;state_.phase=ClientPhase::LoadingMission;pageName_="loading";popup_.clear();ClearPage();client_.Pause(false);RebuildControls(); // 제목은 실제 단계1의 Header 적용/설치에서 갱신한다.
    Compose();client_.PresentScene(); // 동기 요새 적재 전에 같은 실제 안내를 먼저 출력하며 임의 최소 표시 시간은 두지 않는다.
    if (!missionRuntime_->guided) state_.Post({"LoadMissionWorld",{}}); // 다음 루프에 같은 로드를 수행하여 입력/Cancel을 한 차례 처리한다.
}
// Guide의 각 교체 함수가 같은 Runtime/Script/월드와 소유 정지 깊이를 직접 사용하게 한다.
GuideMissionTransitionHooks UberGump::MissionTransitionEffects() {
    GuideMissionTransitionHooks hooks;hooks.current=[this] { return missionRuntime_!=nullptr; };
    hooks.finish=[this] {
        if (!missionRuntime_) throw std::logic_error("현재 미션 없는 Guide 가상 종료");
        if (missionRuntime_->guided) RunTutorialMissionStage(missionRuntime_->state,6,{}); // 실제 Tutorial+1c의 빈 종료다. 다른 형식은 기존 teardown 어댑터를 유지한다.
    };
    hooks.release=[this] { ClearWorld();mission_.reset(); }; // 교체 도중 MainMenu/메뉴 음악을 끼워 넣지 않는다.
    hooks.create=[this](std::string_view name) { CreateMission(std::string(name)); };
    hooks.get=[this](std::string_view key) { if (!mission_) throw std::logic_error("현재 설정 없는 Guide Fork");return mission_->Get(key).value_or(""); };
    hooks.mode=[this](std::uint32_t value) { if (!missionRuntime_) throw std::logic_error("재시작 공장 뒤 모드 수신 누락");missionRuntime_->mode64=value; };
    hooks.demo=[this](std::uint32_t value) { if (!missionRuntime_) throw std::logic_error("재시작 공장 뒤 데모 수신 누락");missionRuntime_->gate.demo=value; };
    return hooks;
}
// 원본 Init은 현재 전역/Config를 쓰고 로드 요청 뒤 Header/flags를 수행한다. 실제 월드는 다음 루프에 설치한다.
void UberGump::InitializeMission() {
    if (!mission_ || !missionRuntime_ || !missionRuntime_->guided || world_ || worldProcess_) throw std::logic_error("Tutorial Init의 현재/빈 월드 계약 위반");
    TutorialInitHooks hooks;hooks.clearTemplates=[this] { if (world_ || worldProcess_ || rawButtons_->Production()) throw std::logic_error("이전 미션 월드/생산 UI가 아직 살아 있음");templateStorage_->ClearAll(); }; // Session보다 오래 보유한 실제9개 manager를 원본 Init 위치에서 반납한다.
    hooks.resetOptions=[this] { o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,*missionOptions_); };
    hooks.get=[this](std::string_view key,std::optional<std::string_view> fallback) { auto value=FontBytes(mission_->Get(key).value_or(""));if (const auto end=value.find('\0');end!=std::string::npos) value.resize(end);if (value.empty() && fallback) value=*fallback;return value; };
    hooks.set=[this](auto key,auto value) { const auto text=o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()});static_cast<void>(mission_->Set(key,text)); };
    hooks.requestFort=[this](auto name) {
        if (state_.Pending() || missionFortRequest_) throw std::logic_error("Init 요새 로드 요청 전 다른 전환/이미지가 남아 있음");
        auto request=std::make_shared<MissionFortLoadRequest>();MissionFortRequestHooks effects; // 원본 부모가 쓴 요청과 실제 복제 이미지를 같은 수명으로 소유한다.
        effects.selectFort=[request](auto value) { request->fort=o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(value.data()),value.size()}); };
        effects.duplicate=[request](auto& packet) { return CaptureMissionFortZone(*request,packet); };
        const std::array<std::uint8_t,9> address{'z','l','o','c','a','l',',','*',0};std::int32_t configuredPort=6799;client_.Configuration().ReadInt("gameServerPort",configuredPort);const auto port=static_cast<std::uint32_t>(configuredPort); // 원본00513870의9BYTE와 현재005318e8의 Client 설정 경계다.
        WriteMissionFortRequest(name,{request->next,request->sequence,address,port},effects);missionFortRequest_=std::move(request);state_.Post({"LoadMissionFort",missionFortRequest_->fort});
    };
    hooks.header=[this] { ApplyMissionTechnology(*mission_,client_.Assets().TypeTable(),std::span(*missionRestrictions_)); };
    InitializeTutorialMission(missionRuntime_->init,missionRuntime_->state.flags,missionRuntime_->gate.demo,{missionRuntime_->gate.tutorialNumber,missionNumber_,tutorialSeen_,initLoginSubscriber_,initialSubscriber_},hooks);
    rebuild_=true;
}
// Guide-only 조건 없이 실제 현재 Runtime을 읽으며 선택한 미션 수명이 바뀌면 물리 경계로 거부한다.
o::FortMissionSaveHooks UberGump::FortMissionSaveEffects() {
    o::FortMissionSaveHooks hooks; // write는 각 Snapshot이 소유한 실제 stream에 동기적으로 연결한다.
    hooks.current=[this] { return missionRuntime_ ? missionRuntime_->saveToken : 0U; };
    hooks.name=[this]()->std::optional<std::span<const std::uint8_t>> {
        if (!missionRuntime_) throw std::logic_error("현재 Mission 없는 이름 물리 접근");
        const auto& name=missionRuntime_->init.name; // 같은 Runtime의 NUL 포함 버퍼이며 저장 중 복제하지 않는다.
        return std::span(reinterpret_cast<const std::uint8_t*>(name.c_str()),name.size()+1);
    };
    hooks.field=[this](auto token,auto offset) {
        if (!missionRuntime_ || token!=missionRuntime_->saveToken) throw std::logic_error("선택 Mission의 저장 수명 만료");
        if (offset==0x58) return missionRuntime_->state.sequence;
        if (offset==0x54) return missionRuntime_->state.flags;
        throw std::out_of_range("Mission 저장의 실제 필드 범위");
    };
    hooks.assertion=[](auto) { throw std::logic_error("Mission 저장 원문 이름 진단"); };
    return hooks;
}
// 캡처한 요새 요청을 다음 State 자리에서 실제 설치한다. Start는 원래 Guide 단계3에 남긴다.
void UberGump::LoadMissionWorld(std::optional<std::string> requestedFort) {
    if (!mission_ || !missionRuntime_ || world_) throw std::logic_error("Guide 초기화의 현재 Script/Runtime/빈 월드 계약 위반");
    const auto type=o::AsciiLower(mission_->MissionType()); // 현재 미션의 실제 가상 형식에 맞는 기존 설치 경로다.
    if (!missionRuntime_->guided) { missionRuntime_->gate.tutorialNumber=std::bit_cast<std::uint32_t>(o::ConfigParseLong(mission_->Get("tutorialNumber").value_or("")));ApplyMissionDemo(mission_->Get("demo").value_or(""),missionRuntime_->gate.demo);o::ResetMissionBattleOptions(o::OriginalEdition::Patch1078,*missionOptions_);o::SetTotalMadeAllowAll(*missionRestrictions_,1);initLoginSubscriber_=1; }
    const auto selectedFort=requestedFort ? *requestedFort : mission_->FortName(); // Init의 지역256BYTE에서 캡처한 값을 이후 설정 재조회로 바꾸지 않는다.
    const auto fortPath=client_.Configuration().PathSpec("fortSpec",selectedFort);const auto fortImage=std::make_shared<const std::vector<std::uint8_t>>(client_.Files().Read(fortPath));auto fort = o::FortTemplate::Parse(*fortImage, client_.Assets().TypeTable());
    // 전체 요새 레코드를 세어 자동 검사에서 미션 선택과 파일 로드를 함께 확인한다.
    const auto count = [this](const o::FortChunkSection& section) { for (const auto& chunk : section.chunks) fortObjects_ += chunk.objects.size(); };
    fortObjects_ = 0; count(fort.chaff);
    // 플레이어 영역의 저장 오브젝트를 포함한다.
    for (const auto& section : fort.territories) count(section);
    // Starting Cash 옵션의 기본 인덱스 1은 6500 SP다(0052f5d0 옵션 표). 옵션 전체 복사는 후속이다.
    auto players=o::MissionPlayers::Load([this](std::string_view key) { return mission_->Get(key); },fort,client_.Assets().TypeTable(),6500);
    // 1-1/1-2도 원본 missionType은 Tutorial이다. 지원 두 종류 모두 같은 실제 raw 월드를 생성한다.
    auto services=MakeCampaignClientServices(client_,*rawButtons_,{
        [this] { RefreshWindowTitle(); },[this](std::string_view key) { Tell(std::string(key)); },
        [this](std::int32_t kind,std::int32_t player,std::int32_t parameter) { warnings_->Remove(kind,player,parameter);rebuild_=true; }}); // 현재 클라이언트의 설정·음향·입력·UI다.
    services.factoryHelp=[this](std::string_view section,std::uint32_t mode) { const auto token=scriptStrings_.Store(section);if (token>0x0fffffffU) throw std::overflow_error("Factory 도움말 문자열 별칭 토큰 소진");rawButtons_->Menus()->OpenHelp(0x70000000U|token,static_cast<std::int32_t>(mode),0); }; //기존Help와같은문자열고리/별칭과실제전용창을사용한다.
    services.missionOutcome.demo=[this] { return missionRuntime_ ? missionRuntime_->gate.demo : 0U; };
    services.missionOptions=missionOptions_;services.missionRestrictions=missionRestrictions_;services.missionSubscriber=initLoginSubscriber_;services.missionFortRequest=missionFortRequest_;services.templateStorage=templateStorage_;services.missionFortImage=fortImage; // 같은 Init 전역/요청/Template9와 실제 선택 파일 원문을 설치에 전달한다.
    services.fortBadges=client_.FortBadges();services.fortMission=FortMissionSaveEffects(); // 실제 Client 단일 장부와 현재 Runtime getter를 같은 Session에 빌린다.
    services.fortTypeConversion=client_.FortTypeConversion(); // 같은 Client의 현재 타입 변환표 소유자를 전달한다.
    services.missionOutcome.tellBlocked=[this] { return state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing ? 1U : 0U; };
    services.missionOutcome.section=[this](std::string_view section) { bool created=false;return mission_ && mission_->Section(section) && TryTell(std::string(section),false,&created) && created; };
    services.missionOutcome.closeTransient=[this] { rawButtons_->RequestClosePopups(); };
    services.missionOutcome.onSection=[this] {
        if (!missionRuntime_ || !missionRuntime_->tutorial) throw std::logic_error("현재 미션의 프레임 정지 연결 누락");
        missionRuntime_->tutorial->Gate().PauseForDialog();
    };
    tutorialSeen_=0; // 원본 State7의 세계 재설정4b2df0도 Init의5ca8f0을0으로 되돌린다. 사제 체력을 앞선1쓰기만으로 바꾸지 않는다.
    auto world=std::make_unique<GameWorld>(client_.Assets(),client_.Files(),std::move(fort),std::move(players),
        selectedFort,std::move(services),ScreenPoint{client_.GetScreen().Width(),client_.GetScreen().Height()},true);
    world->elapsed=[this]() { return client_.Time().delta; }; world->paused=[this]() { return client_.Paused(); };
    world->frameTime=[this]() { return client_.Time(); };
    world->BindHideBuildings([this] { return hideBuildings_; }); // 메뉴 체크·현재 Draw·선택 후보가 같은 원본 DWORD를 조회한다.
    // 카메라/해상도 변경 직후 위치 효과음의 기준을 갱신한다. 월드와 소리 묶음의 수신은 모두 주 스레드에서 실행한다.
    world->soundViewChanged=[this](SoundView view) { client_.WorldAudio().SetView(view); };
    // 장치 재생성 뒤에도 현재 Renderer로 raw 변경 영역을 전달한다.
    world->surfaceDisplay={
        [this]() { return client_.GetRenderer().FullRedrawPending(); },
        [this](o::SquidDisplayRect rect,std::uint32_t flags) { client_.GetRenderer().Invalidate({rect.left,rect.top,rect.right,rect.bottom},flags); }};
    world_=world.get(); worldProcess_=client_.GetKernel().Add(std::move(world));
    missionRuntime_->start=std::make_unique<RawCampaignMissionStartBindings>(world_->Session(),*mission_,missionRuntime_->gate,missionRuntime_->state,
        world_->Session().MissionStartSettings(),world_->Session().MissionAiSettings(),MakeCampaignMissionStartEffects(client_,*world_,[this] {
            // 원본0042e440은 현재00548740이0이면 아무 메시지도 보내지 않는다.
            if (commandGump_!=0) throw std::logic_error("미션 Start의 전용 명령 Gump0042e440 수신 미연결");
        }));
    world_->BeginLocalConnection(); //전역subscriber 조회가같은월드를보는상태에서Zone/로그인/월드로드를실행한다.
    if (type=="tutorial" && !missionRuntime_->guided) ApplyMissionTechnology(*mission_,client_.Assets().TypeTable(),world_->Session().TotalMade()); // guided Header는 이미 설치 전 같은 전역 표를 썼다.
    userInput_=std::make_unique<UserInput>(client_,*world_);
    // 브리핑으로 게임 시계가 멈추기 전에도 첫 카메라/표시 영역이 음악·효과음에 전달되게 한다.
    world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height());
    // 직전 미션의 날씨 팔레트가 유지된 재진입도 현재 화면 색을 사용한다.
    world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());
    // 같은 단일 커서에 현재 장치/트리·비용 표시·선택 해제·실제 로그인 전송을 공급한다.
    auto cursorEffects=MakeCampaignCursorClientEffects(client_,*rawButtons_);
    cursorEffects.send=[session=&world_->Session()](std::uint32_t target,std::span<const std::uint8_t> packet) { session->SendLocalPacket(std::bit_cast<std::int32_t>(target),packet); };
    BindCampaignUserInputServices(client_,*world_,cursorEffects.lifecycle.selectCursor);
    cursorEffects.lifecycle.highlight=[this](std::uint32_t mode) { world_->Highlight(mode); };
    cursorEffects.drop.refresh=[this](std::uint32_t sid,std::uint32_t force,std::uint32_t move) {
        // 원본Drop의004d5c90(0,0,1)을 같은 실제 선택으로 전달한다. Select 파사드는 둘째0/셋째1을 보존한다.
        if (force || move!=1) throw std::logic_error("현재 커서 선택 변경의 실제 인자 계약 오류");world_->Select(sid);
    };
    world_->Session().Cursor().SetEffects(std::move(cursorEffects));
    world_->Session().Cursor().Display().SetExternal([this](std::uint32_t token) { return rawButtons_->GumpDepth(token); },{});
    briefingSections_.clear(); briefingIndex_ = 0;
    if (type=="tutorial") {
        CampaignTutorialOneEffects frameEffects; // 원본 화면 프레임/입력이 함께 사용하는 현재 UI/시계다.
        frameEffects.frame.gameSeconds=[this] { return client_.Time().game; };
        frameEffects.frame.paused=[this] { return client_.Paused(); };
        frameEffects.frame.dialogsClear=[this] { return !pageAlive_; };
        frameEffects.frame.knowledgeOpen=[this] { return world_->Session().CursorDropSettings().lockDepth==0; };
        frameEffects.frame.guideOpen=[this] { return pageAlive_ || !retiredPageExits_.empty(); }; // 원본005cab2c는 Kill 뒤에도 실제 소멸까지 창 개수를 유지한다.
        frameEffects.frame.blockingGumpOpen=[this] { return rawButtons_->BlockingGumpOpen(); };
        frameEffects.frame.pause=[this] { client_.PushGamePause(); };
        frameEffects.frame.resume=[this] { client_.PopGamePause(); };
        frameEffects.frame.endDemo=[this] { client_.Input().Push(kMissionReloadCommand,0,0); }; // 원본452f90의 일반 큐 예약이다. 실행 중인 프레임 콜백 안에서 월드를 해제하지 않는다.
        frameEffects.tell=[this](std::string_view section) { bool created=false;return mission_->Section(section) && TryTell(std::string(section),true,&created) && created; };
        frameEffects.closeTransient=[this] { rawButtons_->RequestClosePopups(); };
        // 현재 월드의 선택 getter와 서로 다른 두 원본 선택 정리 몸체를 빌린다.
        frameEffects.selected=[this] { return world_->Selected(); };frameEffects.resetSelection=[this] { world_->ResetSelection(); };frameEffects.clearSelection=[this] { world_->ClearSelection(); };
        missionRuntime_->tutorial=std::make_unique<RawCampaignTutorialOneBindings>(world_->Session(),*mission_,*missionRuntime_->start,
            missionRuntime_->gate,missionRuntime_->state,std::move(frameEffects));
    }
    // 원본 초기 브리핑은 A.이며 A1. 등은 본문 Tell 명령이 넘긴다.
    if (missionRuntime_->gate.demo!=0) briefingSections_.push_back("Demo");
    else if (mission_->Section("A.")) briefingSections_.push_back("A.");
    state_.phase = ClientPhase::LoadingMission; pageName_ = "loading"; popup_.clear(); ClearPage(); RebuildControls();
    RefreshWindowTitle();
    if (missionRuntime_->tutorial) {
        state_.phase=ClientPhase::Mission;pageName_="mission";briefingSections_.clear();client_.Pause(false); // A/Demo 안내는 공통10호출 뒤 실제 TellStage가 연다.
    } else client_.Pause(true);
    // 원본 004b6dd0의 미션 시작 절차(전투 여부를 켜고 FUN_00469fc0): 난수로 고른 원소 곡으로 전투 음악을 시작한다.
    if (client_.Audio()) client_.Audio()->StartScene(true);
    // 원본004b6dd0의 음악 시작 뒤00463a40→가상+28은 Start가 아닌 자기 정지다.
    if (missionRuntime_->guided) {
        PauseMissionWorldEntry([this] { return missionRuntime_!=nullptr; },[this] { missionRuntime_->tutorial->Gate().PauseForDialog(); });
        // 다음 단계 flags는 원래 Init의 로드 요청/Header 뒤 이미 썼으며 여기서 다시 쓰지 않는다.
    }
}
// 현재 설치된 같은 Session/Script와 장기 순번으로 원본 단계3 Start를 한 번 실행한다.
void UberGump::StartMissionWorld() {
    if (!missionRuntime_ || !missionRuntime_->start || !world_) throw std::logic_error("Guide 단계3의 실제 설치/Start 연결 누락");
    missionRuntime_->start->Start();
}
// 첫 브리핑을 표시하거나 브리핑 종료 뒤 실제 월드 입력과 표시를 활성화한다.
void UberGump::AdvanceBriefing() {
    if (briefingIndex_ < briefingSections_.size()) { Tell(briefingSections_[briefingIndex_++], true); return; }
    state_.phase = ClientPhase::Mission; pageName_ = "mission"; ClearPage(); popup_.clear(); client_.Pause(false);
    world_->Resize(client_.GetScreen().Width(),client_.GetScreen().Height()); userInput_->Cancel(); RebuildControls();
}
// 저장한serial은루트등록칸이나단순인스턴스수로대체하지않는다.
bool UberGump::GameMenuOpen() const { return gameMenuState_ && rawButtons_->SerialGump(gameMenuState_->menuSerial)!=0; }
// 004d958c의serial조회와Kill/0쓰기/66710순서를유지한다.
void UberGump::ToggleGameMenu() {
    if (!world_) return;
    const auto token=gameMenuState_ ? rawButtons_->SerialGump(gameMenuState_->menuSerial) : 0U; // 현재비dead인동일serial만선택한다.
    if (!token) OpenGameMenu();
    else { rawButtons_->KillGump(token);gameMenuState_->menuSerial=0;rawButtons_->RequestClosePopups();rebuild_=true; }
}
// 실제현재Session의조회/검산과기존공통controlled메뉴를한수명안에서작성한다.
void UberGump::OpenGameMenu() {
    if (!world_ || !missionRuntime_ || state_.phase!=ClientPhase::Mission) throw std::logic_error("게임메뉴의현재미션없음");
    auto& session=world_->Session(); // 원본cc210은앞서열린다른팝업을임의로닫지않는다.
    if (!gameMenuState_) gameMenuState_=std::make_unique<GameMenuBarState>(session.GameMenuTrackingOffset());
    rawButtons_->Menus()->SetPalette(client_.GetScreen().Colors());
    rawButtons_->Menus()->OpenAttached([&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& menu) {
        GameMenusHooks hooks; // 실제Session외전역은각조회시점에같은현재Client에서읽는다.
        hooks.global=[this](std::uint32_t address)->std::uint32_t {
            if (const auto value=GameMenuCommandValue(address)) return *value;
            switch (address) {
            case 0x563e38:return rawButtons_->RootToken();
            case 0x531860:case 0x5c78f0:return static_cast<std::uint32_t>(client_.GetScreen().Width());
            case 0x531864:case 0x5c78f4:return static_cast<std::uint32_t>(client_.GetScreen().Height());
            case 0x5c78d4:return client_.GetScreen().Flags();
            case 0x5c7910:return 0; // 현재실제Screen은DIB전용이며DirectDraw장치를설치하지않는다.
            case 0x5c7b14:return client_.WorldAudio().DeviceReady() ? 1U : 0U;
            case 0x531900:return std::bit_cast<std::uint32_t>(client_.Configuration().GetInt("soundVolume"));
            case 0x531904:return std::bit_cast<std::uint32_t>(client_.Configuration().GetInt("musicVolume"));
            case 0x55b4b0:return client_.Paused() ? 1U : 0U; // 이소비자는원본깊이의양수여부만검사한다.
            case 0x59a8ac:return hideBuildings_;
            default:throw std::logic_error("게임메뉴현재전역미연결: "+std::to_string(address));
            }
        };
        hooks.config=[this](auto key) { return client_.Configuration().GetInt(key); };
        hooks.readInt=[this](auto key,std::int32_t& value) { client_.Configuration().ReadInt(key,value); };
        hooks.translate=[this](auto text) { return FontBytes(client_.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(text.data()),text.size()}))); };
        hooks.editingMission=[this] { return missionRuntime_->mode64; };hooks.demo=[this] { return missionRuntime_->gate.demo; };
        hooks.playerMenu=[&](auto subscriber) { return objectMenus_->BuildPlayerMenu(*world_,subscriber,widgets,controls,menu); };
        BindGameMenusControls(hooks,widgets,controls,menu,[this](auto text) { return scriptStrings_.Store(text); });
        hooks.node=[this](auto token) { return rawButtons_->Menus()->Node(token); };
        hooks=session.BindGameMenus(std::move(hooks));const auto globals=hooks.global;const auto translate=hooks.translate; // 바인더가교체한같은현재소유자의getter를빌린다.
        RawGameMenus children(std::move(hooks));GameMenuBarHooks bar;
        bar.prepare=[&session] { session.PrepareGameMenus(); };bar.translate=translate;
        bar.read=[this,globals](GameMenuBarValue value) {
            switch (value) {
            case GameMenuBarValue::Editor:return globals(0x5c85a4);
            case GameMenuBarValue::Battle:return globals(0x594fbc);
            case GameMenuBarValue::Mission:return globals(0x594fa4);
            case GameMenuBarValue::Cheat:return globals(0x5cab30);
            case GameMenuBarValue::Width:return static_cast<std::uint32_t>(client_.GetScreen().Width());
            case GameMenuBarValue::Left:return static_cast<std::uint32_t>(world_->MinimapView().viewport.left);
            case GameMenuBarValue::Root:return rawButtons_->RootToken();
            }
            throw std::logic_error("게임메뉴조회종류오류");
        };
        bar.about=[this] { const auto value=client_.Configuration().Configuration().Get("helpWord");if (!value) throw std::logic_error("게임메뉴현재helpWord포인터없음");return FontBytes(*value); };
        bar.technicalHelp=[this] { std::string exe,file;const auto first=client_.Configuration().ReadString("techHelpExe",exe),second=client_.Configuration().ReadString("techHelpFile",file);return first && second ? 1U : 0U; };
        bar.child=[&](GameMenuBarChild child) {
            switch (child) {
            case GameMenuBarChild::Game:return children.Game();case GameMenuBarChild::Edit:return children.Edit();
            case GameMenuBarChild::View:return children.View();case GameMenuBarChild::Options:return children.Options();
            case GameMenuBarChild::Players:return children.Players();
            case GameMenuBarChild::Cheat:throw std::logic_error("원본Cheat부모생성미연결");
            }
            throw std::logic_error("게임메뉴하위종류오류");
        };
        BindGameMenuBarControls(bar,widgets,controls,menu,[this](auto token) { return rawButtons_->Menus()->Node(token); },[this](auto text) { return scriptStrings_.Store(text); });
        return RawGameMenuBar(*gameMenuState_,std::move(bar)).Open();
    },false);
    rebuild_=true; // 실제메뉴열기는게임시계를정지하거나별도중앙팝업상태를만들지않는다.
}
// 현재 복원 범위에서 의미가 있는 원본 옵션을 구성한다. 소리 장치와 전체화면 항목은 후속이다.
// 공통 입력/Guide가 소비하지 않은 현재 메뉴 명령을 같은 Session·음향·설정·시계에 전달한다.
bool UberGump::GameMenuCommand(const InputEvent& event) {
    const auto code=event.code&0x03ffffffU;
    if (code==0x020000beU) {
        if (state_.phase==ClientPhase::Dialog) Tell("Blank"); // 원본 대화 부모의 MenuJustClosed→Kill에 대응하는 페이지 수명 정리다.
        rawButtons_->RequestClosePopups();rebuild_=true;return true;
    }
    if (code==0x0200006dU) {
        if (event.code&InputCode::kRelease) return true;
        if (event.y) {
            ApplyScriptConfig(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)),[this](auto key,auto value) {
                const auto decode=[](std::string_view bytes) { return o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()}); }; // 실제 설정의 내부 인코딩 경계만 변환한다.
                return client_.Configuration().Set(decode(key),decode(value));
            });
            client_.ApplyAudioOptions(); // 현재 복원된 옵션 장치에 실제 효과음·음악 값을 반영한다.
        }
        ++menuConfigRevision_;worldRedraw_=1;rawButtons_->RouteInput(event);Refresh();return true; // 원본 이 분기의 두 번째466200 호출을 보존한다.
    }
    if (!IsGameMenuCommand(code)) return false;
    auto binding=world_ ? world_->Session().BindGameMenus({}) : GameMenusHooks{}; // 종료/음량은 월드 설치 전후에도 같은 Client 명령이다.
    if (!world_) binding.global=[](std::uint32_t address)->std::uint32_t {
        switch (address) { // 메인 메뉴에서 해제된 미션/Player 모드는 원본 초기0이다.
        case 0x5c85a4:case 0x594fb8:case 0x594fc8:case 0x594fa4:case 0x595344:case 0x540c70:return 0;
        default:throw std::logic_error("월드 밖 메뉴의 다른 현재 전역 미연결");
        }
    };
    const auto session=[this]()->RawCampaignSession& { if (!world_) throw std::logic_error("현재 메뉴 명령의 Session 없음");return world_->Session(); }; // 필요한 분기에서만 현재 월드를 다시 조회한다.
    GameMenuActionHooks hooks;
    hooks.command=[](auto command) { const auto value=GameMenuCommandValue(static_cast<std::uint32_t>(command));if (!value) throw std::logic_error("메뉴 명령의 현재 원문 ID 없음");return *value; };
    hooks.queue=[this](auto command,auto first,auto second) { client_.Input().PushCommand(command,first,second); };
    hooks.tell=[this](auto name) { Tell(std::string(name)); };
    hooks.config=[this](auto assignment) { ApplyScriptConfig(assignment,client_.Configuration());client_.ApplyAudioOptions(); };
    hooks.sound=[this](auto name,auto a,auto b,auto c,auto d,auto e) { client_.WorldAudio().Sounds().PlayByName(name,a,b,c,d,e); };
    hooks.paused=[this] { return client_.Paused(); };
    hooks.status=[this](auto kind,auto owner,auto enabled,auto parameter,auto text) {
        if (kind!=11 || owner || parameter || !text.empty()) throw std::logic_error("다른 메뉴 상태 안내 분기 미연결");
        warnings_->SetView(client_.GetScreen().Width(),client_.GetScreen().Height(),client_.GetScreen().Colors());warnings_->PauseStatus(enabled!=0);
    };
    hooks.cancelCursor=[session] { session().Cursor().Cancel(); };
    hooks.pauseClock=[this] { client_.PushGamePause(); };hooks.resumeClock=[this] { client_.PopGamePause(); };
    hooks.muteSound=[this] { client_.WorldAudio().PushMute(); };hooks.resumeSound=[this] { client_.WorldAudio().PopMute(); };
    hooks.player=binding.player;hooks.centerObject=[this,session](auto sid) { static_cast<void>(session());world_->CenterOnObject(sid); };
    hooks.viewPriest=[this](auto owner) { if (world_) world_->ViewPriest(owner); }; // 일반 F5와 명시 메뉴 owner를 구별한다.
    hooks.reloadCount=[this,session](auto list) { return world_ ? session().GameMenuReloadCount(list==GameMenuReloadList::Second) : 0; };
    hooks.reloadSid=[session](auto list,auto index) { return session().GameMenuReloadSid(list==GameMenuReloadList::Second,index); };
    hooks.form=[session](auto sid)->std::span<const std::uint8_t> { if (sid>65535) throw std::out_of_range("메뉴 Form SID 폭");return session().Core().pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)}); };
    hooks.reloadAt=[session](auto x,auto y,auto mode) { session().ReloadTheamAt(x,y,mode); };
    hooks.assertion=[](auto line) { throw std::logic_error("원본 메뉴 입력 진단: "+std::to_string(line)); };
    const auto result=GameMenuActions({binding.global,hideBuildings_,worldRedraw_},std::move(hooks)).Handle(event.code,std::bit_cast<std::uint32_t>(event.x),std::bit_cast<std::uint32_t>(event.y));
    if (!result) return false;if (*result) quit_=true;Refresh();return true;
}
void UberGump::OpenMenu(std::string name) {
    if (o::AsciiLower(name)=="game" && world_) { OpenGameMenu();return; } // 미션의실제수평부모는중앙임시메뉴와구별한다.
    popup_ = o::AsciiLower(name); ClearPage(); auto& config = client_.Configuration();
    // 목록 행에는 돌 버튼과 달리 누름 즉시 실행 규칙을 적용한다.
    const auto add = [this](std::string label, std::string command, std::string argument, bool checked = false, bool enabled = true) {
        page_.items.push_back({std::move(label), {std::move(command), std::move(argument)}, true, checked, enabled});
    };
    if (popup_ == "options") {
        add("Direct Draw / Full Screen", "", "", false, false);
        add("Resolution >", "Menu", "resolution");
        add("Sound On", "Toggle", "sound", config.GetInt("sound") != 0);
        add("Play Music", "Toggle", "music", config.GetInt("music") != 0);
        add("Wind Noise", "Toggle", "playWind", config.GetInt("playWind") != 0);
        add("Speaker Swap L/R", "Toggle", "swapLeftRightSpeakers", config.GetInt("swapLeftRightSpeakers") != 0);
        add("Sound Effect Volume >", "Menu", "soundvolume"); add("Music Volume >", "Menu", "musicvolume");
        add("Edge Scroll in Fullscreen", "Toggle", "edgeScroll", config.GetInt("edgeScroll") != 0);
        add("Auto-Demo", "Toggle", "autoDemo", config.GetInt("autoDemo") != 0);
        add("Tell Tips at Startup", "Toggle", "tellTips", config.GetInt("tellTips") != 0);
        add("Pass Server Diagnostic", "", "", false, false);
    } else if (popup_ == "resolution") {
        add("640 by 480", "Resolution", "640", client_.GetScreen().Width() == 640);
        add("800 by 600", "Resolution", "800", client_.GetScreen().Width() == 800);
        add("1024 by 768", "Resolution", "1024", client_.GetScreen().Width() == 1024);
    } else if (popup_ == "soundvolume" || popup_ == "musicvolume") {
        const std::string key = popup_ == "soundvolume" ? "soundVolume" : "musicVolume";
        // 원본 음량 단계는 1~5다. 장치 연결 전에 설정 저장을 검사할 수 있다.
        for (int i = 1; i <= 5; ++i) add("Volume " + std::to_string(i), "Volume", key + "=" + std::to_string(i), config.GetInt(key) == i);
    } else if (popup_ == "help") {
        add("About NetStorm", "Tell", "About");
    } else if (popup_ == "game") {
        add("Leave Battle", "Tell", "LeaveNormal");
    } else throw std::runtime_error("Unknown popup: " + name);
    if (world_) { client_.Pause(true); userInput_->Cancel(); }
    opened_ = client_.Time().wall; RebuildControls();
}
// 원본 파생 열기/선택/작성/위치/실제 Form 부착을 현재 사건으로 실행한다.
void UberGump::OpenObjectMenu(o::SquidId id,const InputEvent& event) {
    if (!world_ || !id || !world_->Object(id)) return;
    objectMenus_->Open(*world_,o::Sid{static_cast<std::uint16_t>(id)},event);userInput_->Cancel();rebuild_=true;
}
// 같은 실제 큐의 원본 정수 명령과 물리 사건을 분배한다.
void UberGump::Event(InputEvent event) {
    const auto transitionCode=event.code&0x03ffffffU; // 원본 Guide 제어 사건은 일반 입력/프레임보다 먼저 소비한다.
    if (transitionCode==kMissionBeginCommand || transitionCode==kMissionReloadCommand || transitionCode==kMissionForkCommand || transitionCode==kMissionEndCommand) {
        GuideMissionCommandHooks hooks;
        hooks.begin=[this](std::uint32_t token) { const std::string name(scriptStrings_.Resolve(token));BeginMission(name); }; // 고리가 해제되기 전에 실제 수명을 확인하고 이름을 소유 복사한다.
        hooks.restart=[this] {
            if (!mission_ || !missionRuntime_) throw std::logic_error("현재 미션 없는 Restart의 원본 선행 조건 위반");
            const auto request=CaptureMissionReload(mission_->Name(),missionRuntime_->mode64,missionRuntime_->gate.demo);RestartGuideMission(request,MissionTransitionEffects());
        };
        hooks.fork=[this](std::uint32_t parameter) { ForkGuideMission(parameter,MissionTransitionEffects()); };
        hooks.end=[this] { EndGuideMission(MissionTransitionEffects());if (!missionRuntime_) { MainMenu();if (client_.Audio()) client_.Audio()->StartScene(false); } };
        hooks.invalidBegin=[](std::uint32_t) { throw std::logic_error("원본 Guide Begin의 signed param2>100 문자열 주소/토큰 관문 위반"); };
        if (DispatchGuideMissionCommand(event,hooks)) return;
    }
    if (DispatchMissionAbort(event,client_.Input(),[this](std::string_view section) { Tell(std::string(section)); })) return;
    if (missionRuntime_ && missionRuntime_->tutorial) {
        auto missionEvent=event; // 실제 카메라 F4 입력과 단계 판정이 같은 홈 명령을 보게 한다.
        if ((event.code&0x83ffffffU)==(0x73U<<InputCode::kVirtualKeyShift)) missionEvent.code=0x020000aeU;
        if (missionRuntime_->tutorial->Event(missionEvent)) return;
        if (QueueMissionFirstKey(event,client_.Input(),true)) return; // 모든 연결된 Tutorial 형식은 큐 끝의 First가 현재 안내/자기 정지를 다시 사용한다.
    }
    if (pageAlive_ && popup_.empty() && transitionCode==0x02000038U) {
        const auto first=std::find_if(page_.items.begin(),page_.items.end(),[](const DialogItem& item) { return !item.menu; }); // 원본 첫 버튼 슬롯이며 메뉴 행은 포함하지 않는다.
        if (first!=page_.items.end()) ActivatePageAction(first->action);
        return; // c79b0은 부모에 전달된 Enter 사건의 release를 별도로 거부하지 않는다.
    }
    bool worldUiRouted=false; // 미션사건은같은공통Gump에서먼저한번만소비를판정한다.
    if (world_ && state_.phase==ClientPhase::Mission && popup_.empty()) {
        const auto old=rawButtons_->Pressed();const auto routed=rawButtons_->RouteInput(event);worldUiRouted=true; // nativeGuide뒤466200의현재성공토큰이다.
        if (old!=rawButtons_->Pressed()) rebuild_=true;
        if (routed.activated) {
            if (*routed.activated<0 || static_cast<std::size_t>(*routed.activated)>=actions_.size()) throw std::logic_error("현재UI부모click명령범위오류");
            ActivatePageAction(actions_[static_cast<std::size_t>(*routed.activated)]);rebuild_=true;return;
        }
        if (routed.consumed) { rebuild_=true;return; }
        if (!(event.code&InputCode::kRelease) && (transitionCode==InputCode::kLeftButton || transitionCode==InputCode::kRightButton)) rawButtons_->RequestClosePopups(); // UI밖누름은팝업Kill뒤같은사건으로월드에진행한다.
    }
    if (transitionCode==0x02000017U || transitionCode==0x02000057U) {
        if (!(event.code&InputCode::kRelease)) rawButtons_->Menus()->OpenHelp(std::bit_cast<std::uint32_t>(event.y ? event.y : event.x),transitionCode==0x02000057U ? 0 : 1,0);
        rebuild_=true;return; // Tutorial 소비 뒤 원본 Help/요청의 현재 주제/TYPE 인자를 보존한다.
    }
    if (transitionCode==0x02000095U) { if (!(event.code&InputCode::kRelease)) { const auto name=std::string(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)));static_cast<void>(TryTell(name)); }return; } // Help Back의 스크립트 복귀다.
    if (transitionCode==kDialogDoNothing) return; // 원본 명령 자체는 효과가 없으며 HTML 부모는 발화 시 이미 Kill되었다.
    if (transitionCode==kStartDemo) {
        if (!(event.code&InputCode::kRelease)) BeginMission(std::string(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y))),1);
        return; // 원본005429f4의 실제 세 번째 인자를 동일 공장으로 전달한다.
    }
    if (transitionCode==kCheatAllTechnologyCommand) {
        if (!(event.code&InputCode::kRelease)) { if (!world_) throw std::logic_error("현재 월드 없는 원소 기술 지급");ApplyThemeTechnology(world_->Session(),event.x);rebuild_=true; }
        return; // 일반 입력의 key-up 거부 뒤 원문 첫 인자를 사용하며 창 수명 효과를 넣지 않는다.
    }
    if (!(event.code&InputCode::kRelease) && transitionCode==(0x70U<<InputCode::kVirtualKeyShift)) { client_.Input().PushCommand(0x02000017U,0,0x61000003);return; } // 원본4d8c47의 F1Help 문자열이며 Control/Shift를 별도 거부하지 않는다.
    if (rawButtons_->Menus()->HelpOpen()) { if (!worldUiRouted) static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; } // 현재 공통 모달/스크롤/태그/돌 버튼을 실제 GumpInput에 전달한다.
    if (transitionCode==kProductionBridgeCommand) {
        if (!worldUiRouted) static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; // Guide/현재 모달 뒤 같은 Gump 트리로 전달한다. 정지/활성 커서 관문을 추가하지 않는다.
    }
    if (transitionCode==kShowTechnologyCommand) {
        if (!(event.code&InputCode::kRelease) && world_) rawButtons_->Menus()->OpenKnowledge(world_->Session(),[this](std::string_view key) { return FontBytes(client_.Translate(key)); },static_cast<std::uint32_t>(client_.LanguageNumber()));
        rebuild_=true;return; // 원본 일반 입력의 뗌 거부와 PlayerKnown/메타맵 관문은 실제 메뉴 연결에서 유지한다.
    }
    if (world_ && state_.phase==ClientPhase::Mission && !(event.code&(InputCode::kRelease|InputCode::kControl|InputCode::kShift)) && transitionCode==(0x75U<<InputCode::kVirtualKeyShift)) {
        client_.Input().PushCommand(kShowTechnologyCommand,0,0);return; // 사제 메뉴/Review 버튼과 같은 명령 큐로 F6을 예약한다.
    }
    if (!worldUiRouted && rawButtons_->Menus()->KnowledgeOpen()) {
        if ((event.code&0xffffU)==0x1b && !(event.code&InputCode::kRelease)) { rawButtons_->Menus()->RequestClosePopups();rebuild_=true;return; }
        if (transitionCode==InputCode::kLeftButton || transitionCode==InputCode::kRightButton || transitionCode==InputCode::kMiddleButton) { static_cast<void>(rawButtons_->Event(event));rebuild_=true;return; }
    }
    if ((event.code&0x03ffffffU)==kLeaveBattleCommand) {
        if (event.code&InputCode::kRelease) return; // Guide가 소비한 Abort와 달리 실제 Leave는 원본 일반 입력의 key-up 거부 뒤에 실행한다.
        MainMenu();if (client_.Audio()) client_.Audio()->StartScene(false);return; // 기존 단일 플레이 종료·메뉴 음악 경로를 원본 큐 수신에 연결한다.
    }
    if (objectMenus_ && objectMenus_->Command(event)) { rebuild_=true;return; }
    const auto commandCode=event.code&0x03ffffffU; // 원본명령분배와같이최상위입력플래그만제거한다.
    if (commandCode==kScriptWarning) {
        warnings_->SetView(client_.GetScreen().Width(),client_.GetScreen().Height(),client_.GetScreen().Colors());
        warnings_->Add(scriptStrings_.Resolve(std::bit_cast<std::uint32_t>(event.y)));rebuild_=true;return;
    }
    if (GameMenuCommand(event)) return;
    if (IsGameMenuCommand(commandCode)) throw std::logic_error("현재 메뉴 선택 명령의 실제 장치 미연결: "+std::to_string(commandCode));
    // 원본 ESC 글자 사건. 초기 브리핑은 확인 버튼으로만 닫는다.
    if ((event.code & 0xffffu) == 0x1b && (event.code & InputCode::kRelease) == 0) {
        if (state_.phase == ClientPhase::Briefing || state_.phase == ClientPhase::LoadingMission) return;
        if (world_ && userInput_ && state_.phase==ClientPhase::Mission && popup_.empty()) {
            if (!world_->Session().CursorDropSettings().lockDepth) { userInput_->Event(event);rawButtons_->RequestClosePopups();rebuild_=true; }
            else ToggleGameMenu(); // 현재커서가없으면같은serial조회로막대를열거나닫는다.
            return;
        }
        if (!popup_.empty()) state_.Post({"Menu", "close"});
        else if (mission_ && state_.phase == ClientPhase::Mission) state_.Post({"Menu", "game"});
        else if (state_.phase == ClientPhase::Dialog) state_.Post({"Tell", "Blank"});
        return;
    }
    if (!popup_.empty() && (event.code & 0x03ffffffu) == InputCode::kLeftButton &&
        (event.code & InputCode::kRelease) == 0 && !rawButtons_->Menus()->Contains({event.x,event.y})) {
        state_.Post({"Menu", "close"}); return;
    }
    if (!worldUiRouted && objectMenus_ && objectMenus_->Active() && rawButtons_->Menus()->Contains({event.x,event.y})) {
        static_cast<void>(rawButtons_->Event(event));rebuild_=true;return;
    }
    if (world_ && userInput_ && state_.phase==ClientPhase::Mission && popup_.empty()) {
        // 가상키도 상위16비트에 있으므로 버튼과 정확히 일치할 때만 좌표로 UI를 가린다.
        const auto mouse=event.code&0x03ffffffu;
        const bool mouseButton=mouse==InputCode::kLeftButton || mouse==InputCode::kRightButton || mouse==InputCode::kMiddleButton;
        const bool ui=mouseButton && (rawButtons_->PickGump({event.x,event.y})!=0 || std::any_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) { return GumpControlInput::Contains(control.rect,{event.x,event.y}); }));
        if (!ui && !rawButtons_->HasCapture()) { if (const auto id=userInput_->Event(event)) { if (*id) OpenObjectMenu(*id,event); else world_->Select(0); } return; }
    }
    if (popup_.empty() && (state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing)) {
        const auto mouse=event.code&0x03ffffffU; // 모달 안내의 현재 실제 버튼 영역만 마우스 입력을 받는다.
        const bool mouseButton=mouse==InputCode::kLeftButton || mouse==InputCode::kRightButton || mouse==InputCode::kMiddleButton; // 뗌도 같은 대상에 전달한다.
        if (mouseButton && !rawButtons_->HasCapture() && std::none_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) {
            return GumpControlInput::Contains(control.rect,{event.x,event.y});
        })) return; // 보존한 생산 패널을 안내창 뒤에서 조작하지 않는다.
    }
    const auto old=rawButtons_->Pressed();auto activated=worldUiRouted ? std::optional<int>{} : rawButtons_->Event(event);
    // 메뉴 행은 현재 어댑터에서 누름 즉시 실행한다. 돌 버튼 명령은 실제 부모 click 통지만 사용한다.
    if (popup_.empty() && !activated && std::any_of(input_.Controls().begin(),input_.Controls().end(),[&](const GumpControl& control) {
        return control.menu && GumpControlInput::Contains(control.rect,{event.x,event.y}); })) activated=input_.Event(event);
    if (old!=rawButtons_->Pressed()) rebuild_=true;
    if (activated && *activated >= 0 && static_cast<std::size_t>(*activated) < actions_.size()) {
        // 원본 004cebf0은 메인 메뉴 버튼 사건을 받자마자 시작 표시를 지운다.
        if (state_.phase == ClientPhase::MainMenu && popup_.empty()) client_.ClearFullScreenState();
        ActivatePageAction(actions_[static_cast<std::size_t>(*activated)]);
    }
}
// 네이티브 입력 큐의 버튼 사건과 폴링 좌표를 같은 Gump 판정기에 전달한다.
bool UberGump::Input() {
    // 사건 순서를 유지한다. 첫 상태 전환만 다음 프레임에 예약된다.
    const auto mouse=PrepareGumpPointerBoundary(pointerBoundary_,static_cast<std::uint32_t>(client_.LanguageNumber()),
        [this](auto code) { return client_.Poll(code); },client_.Input()); // Alt/언어3 Ctrl·재진입 release를 같은 원본40칸 큐에 연결한다.
    if (missionRuntime_ && missionRuntime_->tutorial) static_cast<void>(missionRuntime_->tutorial->Event(mouse)); //4d65a7의Guide 폴링 통지는 반환을 검사하지 않는다. 일반 큐 Event의 소비 관문은 별개다.
    // 현재 카메라 폴링 어댑터는 원본처럼 Overlay/일반 큐보다 앞에서 실행한다. Tick 내부 UpdateCursor와 기존 직접 호출을 중복하지 않는다.
    if (userInput_ && state_.phase==ClientPhase::Mission && popup_.empty()) userInput_->Tick();
    else if (userInput_) userInput_->Cancel();
    const auto mode=world_ ? world_->InputMode() : 0U; //005cae90의 현재 강조 모드이며 페이지 phase로 대체하지 않는다.
    const bool cursorActive=world_ && world_->Session().CursorDropSettings().lockDepth==0; //445330은5325c8==0이며 Session의 같은 깊이를 읽는다.
    const bool logicalModal=state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing || !popup_.empty(); // 아직 공통 modal 부모가 아닌 기존 HTML/임시 팝업 뒤의 overlay만 막는 호스트 경계다.
    const auto pointer=rawButtons_->NotifyPointer(mouse,mode,cursorActive,!logicalModal); // 원본4d69f4는 일반 큐 Pop보다 앞이다.
    if (pointer.overlay && *pointer.overlay==0) client_.GetCursor().Set(1); // 실제466150 반환0의4d68cc→4d69d3 fallback이며 미호출과 구별한다.
    if (pointer.changed) rebuild_=true;
    if (pointer.activated) {
        if (*pointer.activated<0 || static_cast<std::size_t>(*pointer.activated)>=actions_.size()) throw std::logic_error("현재 포인터 부모 click 명령 범위 오류");
        if (state_.phase==ClientPhase::MainMenu && popup_.empty()) client_.ClearFullScreenState();
        ActivatePageAction(actions_[static_cast<std::size_t>(*pointer.activated)]);rebuild_=true;
    }
    while (!client_.Input().Empty()) Event(client_.Input().Pop(false));
    if (input_.Move({mouse.x, mouse.y}, (mouse.code & InputCode::kOutside) == 0)) rebuild_ = true;
    if (world_ && world_->TakeChanged()) rebuild_=true;
    if (rebuild_) Compose();
    return quit_;
}
// 예약 명령은 메시지/입력보다 앞서 실행한다(원본 004b88c0 자리).
void UberGump::Tick() {
    const bool loading = state_.phase == ClientPhase::LoadingMission;
    if (const auto action = state_.Take()) Execute(*action);
    else if (loading && !(missionRuntime_ && missionRuntime_->guided)) AdvanceBriefing();
    if (page_.timeoutSeconds > 0 && client_.Time().wall - opened_ >= page_.timeoutSeconds) {
        const auto action=page_.timeout;page_.timeoutSeconds=0;ActivatePageAction(action); // 같은 부모의 timeout을 두 번 발화하지 않는다.
    }
    if (rebuild_) Compose();
}
// 원본 루프8번에서 현재 Guide의 상태/창 관문과0..6 단계를 한 번 실행한다.
void UberGump::MissionFrame() {
    if (guide_) guide_->Run();
}
// 원본의미니맵프레임은게임정지와별개로커널보다먼저실행한다.
void UberGump::RefreshMinimap() { if (rawButtons_->Minimap() && rawButtons_->Minimap()->Refresh()) rebuild_=true; }
// Client 정상 갱신의 원본4662b0 위치에서만 논리 페이지의 지연 소멸을 실행한다.
void UberGump::CleanupDialogs() { DisposeRetiredPages(); }
// 단일 전역 큐를 아직 공유하지 않는 Warning/논리 HTML은 명시 순서 어댑터다. raw 부모의 실제 정리는 같은 공통 큐 한 번뿐이다.
void UberGump::CleanupGumps() {
    if (rawButtons_->CleanupGumps()) rebuild_=true;
    if (warnings_->CleanupGumps()) rebuild_=true;
    CleanupDialogs();
}
// 원본 일반/영상잠금 양쪽 가지에서 같은 idle을 한 번 실행하고 표시 변화는 뒤쪽 Frame 합성으로 넘긴다.
void UberGump::IdleGumps() {
    if (rawButtons_->IdleGumps()) rebuild_=true;
    if (warnings_->IdleGumps()) rebuild_=true;
}
// 커널의 이동·날씨와 비동기 Tell을 같은 프레임에 반영한다. 페이지가 같으면 입력 영역/누름 상태를 유지한다.
void UberGump::Frame() {
    if (world_ && world_->TakeChanged()) rebuild_=true;
    if (rebuild_) Compose();
}
// UI 합성을 예약하고 이미 제출된 정적 픽셀도 다시 그리게 한다.
void UberGump::Refresh() { rebuild_=true;client_.GetRenderer().InvalidateAll(); }
// 새 RGB를 월드에 공급한 뒤 UI의 장식·글자·선택 색도 다시 합성한다.
void UberGump::PaletteChanged() {
    if (world_) world_->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());
    rawButtons_->SetPalette(ButtonColors(client_.GetScreen().Colors()));
    if (rawButtons_->Production()) rawButtons_->Production()->SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());
    if (rawButtons_->Minimap()) rawButtons_->Minimap()->SetPalette(client_.GetScreen().Colors());
    Refresh();
}
// 선택한 원본 명령의 작은 부분집합. 지원하지 않는 항목은 활성화하지 않는다.
void UberGump::Execute(const DialogAction& action) {
    if (o::AsciiLower(action.command)=="loadmissionworld") { LoadMissionWorld();StartMissionWorld();return; } // 비 Tutorial도 표시/입력 뒤 같은 원본 월드 설치를 수행한다.
    if (o::AsciiLower(action.command)=="loadmissionfort") { if (!missionRuntime_ || !missionRuntime_->guided) throw std::logic_error("현재 Tutorial 없는 지연 요새 로드");LoadMissionWorld(action.argument);return; } // 같은 전환 큐의 다음 루프에서만 설치한다.
    if (QueueMissionControlAction(action.command,action.argument,client_.Input(),scriptStrings_)) return;
    if (GameMenuScriptCode(action.command)!=0xffffffffU) { QueueScriptCommand(action.command+','+action.argument,client_.Input(),scriptStrings_,{GameMenuScriptCode,{}});return; } // 원본확인창/메뉴닫기효과도같은정수명령큐를사용한다.
    if (KnowledgeScriptCode(action.command)!=0xffffffffU) { QueueScriptCommand(action.command+','+action.argument,client_.Input(),scriptStrings_,{KnowledgeScriptCode,{}});return; }
    const auto command = o::AsciiLower(action.command);
    if (command == "quit") { quit_ = true; return; }
    if (command == "tell") { Tell(action.argument, state_.phase == ClientPhase::Briefing); return; }
    if (command == "startdemo") { BeginMission(action.argument,1);return; }
    if (command == "donothing" || command == "resume") {
        if (state_.phase == ClientPhase::Briefing) AdvanceBriefing(); else Tell("Blank"); return;
    }
    if (command == "menu") {
        if (action.argument == "close") { popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); }
        else OpenMenu(action.argument);
        return;
    }
    if (command == "toggle") {
        auto& config = client_.Configuration(); config.SetInt(action.argument, config.GetInt(action.argument) == 0 ? 1 : 0);
        // 소리·음악·좌우 바꿈은 원본 Interpret Options처럼 장치와 현재 곡에 바로 반영한다.
        if (action.argument == "sound" || action.argument == "music" || action.argument == "swapLeftRightSpeakers") client_.ApplyAudioOptions();
        popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); return;
    }
    if (command == "volume") {
        const auto equal = action.argument.find('='); client_.Configuration().SetInt(action.argument.substr(0, equal), o::ConfigParseLong(action.argument.substr(equal + 1)));
        client_.ApplyAudioOptions(); // 효과음·음악 음량 단계를 장치에 바로 적용한다.
        popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); return;
    }
    if (command == "resolution") {
        const int width = o::ConfigParseLong(action.argument), height = width == 640 ? 480 : width == 800 ? 600 : 768;
        client_.ChangeResolution(width, height); popup_.clear(); ClearPage(); if (world_ && state_.phase==ClientPhase::Mission) client_.Pause(false); RebuildControls(); return;
    }
    throw std::runtime_error("Command pending: " + action.command);
}
// Tell의 호출 단계와 무관하게 다음 합성에서 이전 입력 트리를 폐기하도록 예약한다.
void UberGump::RebuildControls() { controlsChanged_=true;rebuild_=true; }
// 배경 합성과 글자 목록 제출은 입력 판정과 분리한다. 페이지 교체 때만 이전 캡처/등록을 정리한다.
void UberGump::Compose() {
    const bool controls=controlsChanged_; // Tick/Input/Kernel 뒤 Frame 중 어느 합성이든 같은 교체 예약을 소비한다.
    const int width = client_.GetScreen().Width(), height = client_.GetScreen().Height();
    const ScreenRect all{0, 0, width, height};
    auto canvas = std::make_shared<IndexedImage>(); canvas->width = width; canvas->height = height;
    canvas->indices.resize(static_cast<std::size_t>(width) * height); canvas->opacity.assign(canvas->indices.size(), 255);
    Tile(*canvas, clouds_, all);
    std::vector<RenderText> text; std::vector<GumpControl> regions;
    const auto& font = client_.Fonts().Get(0);
    const auto palette = client_.GetScreen().Palette();
    const auto light = Color(palette, 191, 178, 139), dark = Color(palette, 49, 44, 36);
    // 마지막 파일 로딩의 실제 버튼 색/문맥을 사용한다. 일시 번개 팔레트에서 색 번호를 다시 찾지 않는다.
    const auto buttonColors=ButtonColors(client_.GetScreen().Colors());
    const auto buttonContexts=ButtonTextContexts(buttonColors);
    warnings_->SetView(width,height,client_.GetScreen().Colors());
    // TextGump의 실제 기준선/내부 클립을 UI 글자 목록 끝에 제출한다.
    const auto appendWarnings=[&] { auto messages=warnings_->Text();text.insert(text.end(),std::make_move_iterator(messages.begin()),std::make_move_iterator(messages.end())); };
    std::vector<RenderSprite> worldSprites,dialogSprites; // 월드와본문그림은같은실제자산을읽으며UI본문목록은해당패널에만제출한다.
    client_.GetRenderer().SetOverlay(nullptr);client_.GetRenderer().SetOverlayProcedure({});
    if (world_ && state_.phase!=ClientPhase::LoadingMission) {
        world_->Resize(width,height);worldSprites=world_->Sprites();
        auto& cursorDisplay=world_->Session().Cursor().Display();cursorDisplay.Settings().gridOption=client_.GetScreen().Flags();
        std::int32_t themeMode=1; // 원본 설정 철자와 누락 기본값을 같은 소유자 변경 경계와 맞춘다.
        client_.Configuration().ReadInt("theammode",themeMode);cursorDisplay.SetThemeMode(static_cast<std::uint32_t>(themeMode));
        cursorDisplay.SetText(client_.Fonts(),[this](std::string_view key) { const auto bytes=o::EncodeOriginalText(client_.Translate(key));return std::string(bytes.begin(),bytes.end()); });
        if (!client_.GetRenderer().HasExtraCollection()) client_.GetRenderer().SetExtraCollection([this](ScreenRect clip,std::uint32_t flags) {
            if (!world_ || state_.phase!=ClientPhase::Mission || !popup_.empty()) return std::vector<RenderSprite>{};
            return world_->Session().Cursor().Display().Sprites(clip,flags);
        });
        // 정지한 대화상자만 월드를 배경에 합성하고 실제 조작 화면은 살아 있는 스프라이트를 제출한다.
        if (state_.phase!=ClientPhase::Mission || !popup_.empty()) {
            client_.GetRenderer().SetBackground(canvas); client_.GetRenderer().SetScene(worldSprites);
            canvas = std::make_shared<IndexedImage>(client_.GetRenderer().SceneImage());
        }
    }
    if (world_ && popup_.empty() && (state_.phase==ClientPhase::Dialog || state_.phase==ClientPhase::Briefing) && rawButtons_->Production()) {
        // 같은 살아 있는 패널을 먼저 그린 뒤 아래에서 모달 본문/테두리를 합성한다.
        rawButtons_->Menus()->Draw(canvas->indices,width,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps());
    }
    if (controls) { labels_.clear(); actions_.clear(); input_.Cancel();rawButtons_->Cancel(); }
    // 같은 순서의 영역/라벨/명령을 제출한다. 그림만 갱신할 때도 번호가 유지된다.
    const auto control = [&](const DialogItem& item, ScreenRect r) {
        const int id = static_cast<int>(regions.size()); regions.push_back({id, r, item.menu, item.enabled, item.instant});
        if (controls) { labels_.push_back(item.label); actions_.push_back(item.action); }
        return id;
    };
    // 돌 버튼은 누르는 동안만 명암과 글자 위치를 바꾼다.
    const auto button = [&](const DialogItem& item, int x, int y, int w) {
        const int id = control(item, {x, y, x + w, y + kButtonHeight + 1});
        const bool drawRaw=!controls && (popup_.empty() || state_.phase==ClientPhase::MainMenu); // 팝업 아래 메인 버튼도 같은 실제 노드를 유지한다.
        const auto label=drawRaw ? rawButtons_->Label(id) : FontBytes(item.label);
        const ScreenRect r{x, y, x + w, y + kButtonHeight};
        // 실제 raw 자식의 상대 (1,1) 부착/크기/눌림을 사용한다. 첫 준비 합성은 다음 raw 합성으로 대체된다.
        const auto background=!drawRaw ? PlanGumpBackground({r.left+1,r.top+1,r.right-1,r.bottom-1},all,
            buttonTextureMetrics_.width,buttonTextureMetrics_.height,GumpBackgroundFlags::kButton) : rawButtons_->BackgroundPlan(id,all);
        DrawGumpBackground(*canvas,decorations_.at("A01"),buttonTextureOffset_,background,client_.GetScreen().ShadeMaps());
        const auto plan=!drawRaw ? PlanButtonDraw({r,font.Measure(label),font.Height(),0,0,item.enabled ? 0U : 1U},buttonContexts,buttonColors.edge) :
            rawButtons_->DrawPlan(id,font.Measure(label),font.Height());
        DrawGumpLines(*canvas,plan.lines);
        text.push_back({&font,label,x+plan.textOffset.x,y+plan.textOffset.y,static_cast<std::uint8_t>(plan.text.color),
            static_cast<std::uint8_t>(plan.text.shadowColor),{plan.text.shadowX,plan.text.shadowY},(plan.text.flags&2)!=0,(plan.text.flags&4)!=0});
    };
    if (state_.phase == ClientPhase::MainMenu || (!mission_ && !popup_.empty())) {
        Paste(*canvas, title_, (width - 640) / 2, (height - 480) / 2, all);
        const int x = width / 2 - 156, y = height / 2 - 73;
        const std::string campaign = Script("tell").Section("UCampaign") ? "UCampaign" : "Campaign";
        // 원본 10.78의 8개 버튼. 미복원 기능도 원본 자료의 창을 통해 상태를 알린다.
        const std::array<DialogItem, 8> buttons{{
            {"Campaign", {"Tell", campaign}}, {"Multiplayer", {"Tell", "NotImplemented"}}, {"Demo", {"Tell", "DEMOVILLE"}}, {"Help", {"Menu", "help"}},
            {"Edit", {"Tell", "NotImplemented"}}, {"Credits", {"Tell", "Credits"}}, {"Options", {"Menu", "options"}}, {"Quit", {"Quit", "0"}}}};
        // 펼침 메뉴 동안 밑에 보이는 버튼은 입력 영역에 넣지 않는다.
        for (std::size_t i = 0; i < buttons.size(); ++i) button(buttons[i], x + static_cast<int>(i % 4) * kButtonPitch, y + static_cast<int>(i / 4) * 23, kButtonWidth);
    }
    if (state_.phase == ClientPhase::Mission && popup_.empty()) {
        // 현재 연결한 Game 메뉴와 실제 로컬 Storm Power를 월드 위에 표시한다.
        button({"Game", {"Menu", "game"}}, 84, 4, kButtonWidth);

        appendWarnings();client_.GetRenderer().SetBackground(canvas); client_.GetRenderer().SetScene(std::move(worldSprites), std::move(text));
        if (world_) client_.GetRenderer().SetOverlay(world_->SelectionImage());
        if (rawButtons_->Menus()->HasOverlay()) {
            client_.GetRenderer().SetOverlayProcedure(RenderProcedure{all,[this](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
                rawButtons_->Menus()->Draw(pixels,pitch,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps(),clip);
            }});
        }
    } else {
        if (!popup_.empty()) {
            if (controls) {
                regions.clear();labels_.clear();actions_.clear();
                // 첫 합성에서는 호스트 DialogAction 번호만 준비한다. 실제 경계는 아래 원본 메뉴 생성 뒤 읽는다.
                for (const auto& item:page_.items) control(item,{0,0,1,1});
            } else {
                regions=rawButtons_->Menus()->Controls();
                // 먼저 기존 바탕 글자를 실제 픽셀로 합성하고 그 위에 같은 원본 메뉴 Draw를 실행한다.
                RasterMenuBackdropText(*canvas,text);text.clear();
                rawButtons_->Menus()->Draw(canvas->indices,width,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps());
            }
        } else if (state_.phase == ClientPhase::Dialog || state_.phase == ClientPhase::Briefing) {
            if (!page_.background.empty()) {
                auto it = decorations_.find(page_.background);
                if (it == decorations_.end()) {
                    // 원본 00419ec0도 파일이 없으면 배경을 만들지 않는다(nsstart 등은 배포 자료에 없을 수 있다).
                    if (const auto bytes = client_.Files().TryRead(client_.Configuration().PathSpec("BackgroundGifSpec", page_.background)))
                        it = decorations_.emplace(page_.background, DecodeGif(*bytes)).first;
                }
                if (it != decorations_.end()) Paste(*canvas, it->second, (width - static_cast<int>(it->second.width)) / 2, (height - static_cast<int>(it->second.height)) / 2, all);
            }
            const int contentWidth = std::min(480, width - 64), panelWidth = contentWidth + 32;
            int contentHeight = 0, menuRows = 0, totalButtonWidth = 0, buttons = 0; // 본문과 가장 긴 현재 열/하단 버튼의 높이를 합친다.
            const auto columnCount=std::max(1U,page_.menuColumns);std::vector<int> rowsByColumn(columnCount); // 단일 열 페이지도 같은 계산을 사용한다.
            const auto measured=dialogBody_->Layout(contentWidth,{0,0},{0,0,contentWidth,height}); // 실제원본버퍼의전체줄높이를구하고inline서식은별도문단으로쪼개지않는다.
            contentHeight=measured.height;
            // 목록과 하단 버튼의 실제 높이를 더한다.
            for (const auto& item : page_.items) {
                if (item.menu) {
                    if (item.menuColumn>=columnCount) throw std::out_of_range("Tell 표시의 현재 메뉴 열 범위 오류");
                    menuRows=std::max(menuRows,++rowsByColumn[item.menuColumn]);
                } else { ++buttons; totalButtonWidth += std::max(kButtonWidth, font.Measure(FontBytes(item.label)) + 16) + 4; }
            }
            const int panelHeight = std::min(height - 32, contentHeight + menuRows * kRowHeight + (buttons ? 27 : 0) + 32);
            const int left = (width - panelWidth) / 2, top = (height - panelHeight) / 2;
            const ScreenRect panel{left, top, left + panelWidth, top + panelHeight}; Tile(*canvas, decorations_.at("A00"), panel); Bevel(*canvas, panel, light, dark);
            const int footerTop = top + panelHeight - 16 - (buttons ? 27 : 0) - menuRows * kRowHeight;
            const auto bodyLayout=dialogBody_->Layout(contentWidth,{left+16,top+16},{left+16,top+16,left+16+contentWidth,footerTop}); // 같은표시복제를현재본문내부영역에서만출력한다.
            text.insert(text.end(),bodyLayout.text.begin(),bodyLayout.text.end());
            dialogSprites=bodyLayout.sprites;
            std::fill(rowsByColumn.begin(),rowsByColumn.end(),footerTop); // 각 열의 첫 행은 같은 footer 위쪽 기준에서 시작한다.
            // 캠페인 제목/완료 체크/잠금 조건은 원본 스크립트 값을 그대로 사용한다.
            for (const auto& item : page_.items) if (item.menu) {
                auto& row=rowsByColumn[item.menuColumn];const auto column=static_cast<int>(item.menuColumn),columns=static_cast<int>(columnCount);
                const ScreenRect rect{left+16+contentWidth*column/columns,row,left+16+contentWidth*(column+1)/columns,row+kRowHeight};
                const int id = control(item,rect);
                if (input_.HoveredMenu() == id) Fill(*canvas,rect,dark);
                if (item.checked) Paste(*canvas, decorations_.at("J02"),rect.left+1,row+3,panel);
                text.push_back({&font,FontBytes(item.label),rect.left+16,row+1,item.enabled ? static_cast<std::uint8_t>(255) : dark,0,{1,1},true,false});text.back().clip=rect;row+=kRowHeight;
            }
            int x = (width - totalButtonWidth + 4) / 2;
            // 하단 버튼은 라벨 폭에 맞춰 나란히 놓는다.
            for (const auto& item : page_.items) if (!item.menu) { const int w = std::max(kButtonWidth, font.Measure(FontBytes(item.label)) + 16); button(item, x, top + panelHeight - 16 - kButtonHeight, w); x += w + 4; }
            // 원본 장식 모서리 네 프레임을 확대하지 않고 붙인다.
            for (int corner = 0; corner < 4; ++corner) {
                const auto& image = decorations_.at("I0" + std::to_string(corner));
                Paste(*canvas, image, corner % 2 ? panel.right - static_cast<int>(image.width) : panel.left, corner / 2 ? panel.bottom - static_cast<int>(image.height) : panel.top, panel);
            }
        } else if (state_.phase == ClientPhase::LoadingMission && !controls) {
            rawButtons_->Menus()->Draw(canvas->indices,width,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps());
        }
        appendWarnings();client_.GetRenderer().SetBackground(canvas); client_.GetRenderer().SetScene(std::move(dialogSprites), std::move(text));
    }
    if (controls) {
        if (state_.phase!=ClientPhase::LoadingMission) rawButtons_->Menus()->CloseMissionLoading();
        std::vector<std::string> originalLabels;originalLabels.reserve(labels_.size());
        // 원본 돌 버튼의 63바이트 라벨은 UTF-8가 아닌 기존 게임 글꼴 바이트를 사용한다.
        for (const auto& label:labels_) originalLabels.push_back(FontBytes(label));
        if (popup_.empty()) rawButtons_->SetControls(regions,originalLabels,width,height,{buttonTextureType_,buttonTextureFrame_,
            static_cast<std::int16_t>(buttonTextureMetrics_.width),static_cast<std::int16_t>(buttonTextureMetrics_.height),buttonColors},world_!=nullptr);
        if (state_.phase==ClientPhase::LoadingMission) {
            rawButtons_->Menus()->SetPalette(client_.GetScreen().Colors());
            // 원본 기본 HUD의 수직 구분선 오른쪽 지도 사각형이다. 전체 dock 드래그 연결 전의 초기값85를 사용한다.
            rawButtons_->Menus()->ShowMissionLoading(FontBytes(client_.Translate("Starting Mission...")),FontBytes(client_.Translate("Cancel")),{85,0,width,height});
        }
        if (popup_.empty() && world_) {
            if (!rawButtons_->Production()) rawButtons_->EnableProduction(world_->Session(),client_.Assets(),RawProductionHooks{
                [this] { return client_.WallSeconds(); },[this](std::string_view name) { client_.WorldAudio().Sounds().PlayByName(name,0,0,0,0,0); },
                [this](std::string_view key) { state_.Post({"Tell",std::string(key)}); },
                [this](std::string_view key,std::int32_t fallback) { client_.Configuration().ReadInt(key,fallback);return fallback; },
                [this](std::string_view key) { return FontBytes(client_.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(key.data()),key.size()}))); },
                [this](const InventoryItemNode& item,ScreenPoint point,const std::function<bool(o::Sid)>& affordable) { if (!world_ || !objectMenus_) throw std::logic_error("생산 정보의 현재 월드 없음");return objectMenus_->OpenProduction(*world_,item,point,affordable); }});
            auto& production=*rawButtons_->Production(); // 같은 미션의 안내/복귀는 예산/등록ID를 재생성하지 않는다.
            production.Resize(height);production.SetPalette(client_.GetScreen().Palette(),client_.ColorSortInput()->Logical());
            if (!rawButtons_->Minimap()) rawButtons_->EnableMinimap(world_->Session(),RawMinimapHooks{
                [this] { return world_->MinimapView(); },[this](auto x,auto y) { world_->CenterCamera(x,y); },
                [this](auto x,auto y) { world_->MoveFromMinimap(x,y); },[this] { return world_->InputMode(); },[this] { return client_.Poll(0xffff); }});
            rawButtons_->Minimap()->Resize(height);rawButtons_->Minimap()->SetPalette(client_.GetScreen().Colors());
        }
        if (!popup_.empty()) {
            auto& menus=*rawButtons_->Menus();menus.SetPalette(client_.GetScreen().Colors());
            menus.Open({width/2+2,height/2-27},[&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump&) {
                widgets.BeginVertical();
                // 기존 팝업 동작의 호스트 인덱스만 연결하며 실제 게임 명령의 원본 토큰은 이 어댑터와 구분한다.
                for (std::size_t index=0;index<page_.items.size();++index) {
                    const auto& item=page_.items[index];controls.AddChecked(item.checked,FontBytes(item.label),0x01ffffffU,static_cast<std::uint32_t>(index+1),true,item.enabled);
                }
            });
            regions=menus.Controls();
        }
        input_.SetControls(std::move(regions));
        if (world_) world_->Session().Cursor().GumpsChanged(); // 고정 마우스에서도 교체 전 UI 토큰의 깊이를 읽지 않는다.
        // 새 페이지는 같은 raw 노드로 한 번 다시 합성하며 중간 어댑터 그림은 화면에 제출하지 않는다.
        controlsChanged_=false;Compose();return;
    }
    if (rawButtons_->Menus()->KnowledgeOpen() || rawButtons_->Menus()->HelpOpen()) client_.GetRenderer().SetOverlayProcedure(RenderProcedure{{0,0,width,height},[this](std::span<std::uint8_t> pixels,int pitch,int height,ScreenRect clip) {
        rawButtons_->Menus()->Draw(pixels,pitch,height,client_.GetScreen().Colors(),client_.GetScreen().ShadeMaps(),clip);
    }}); // 원래 브리핑을 유지한 상태에서도 같은 실제 지식 카드를 가장 위에 그린다.
    if (world_) world_->TakeChanged();
    rebuild_ = false;
}
// 자동 검사는 라벨 좌표를 읽고 실제 버튼 입력을 보낸다. 명령을 직접 호출하지 않는다.
std::optional<ScreenPoint> UberGump::ControlPoint(std::string_view label) const {
    if (rawButtons_->Minimap() && label.starts_with("minimap:")) return rawButtons_->Minimap()->Point(label);
    if (rawButtons_->Production() && label.starts_with("production:")) return rawButtons_->Production()->Point(static_cast<std::uint32_t>(std::stoul(std::string(label.substr(11)))));
    if (world_ && (label.starts_with("world:") || label.starts_with("cell:") || label.starts_with("placement:") || label.starts_with("placement-near:") || label.starts_with("placement-reach:") || label.starts_with("placement-fire:"))) return world_->Point(label);
    if (rawButtons_->Menus()->HasMenus() || state_.phase==ClientPhase::LoadingMission) if (const auto point=rawButtons_->Menus()->Point(FontBytes(label))) return point;
    // 동일 라벨의 첫 영역을 선택한다.
    for (const auto& control : input_.Controls()) if (labels_.at(static_cast<std::size_t>(control.id)) == label)
        return ScreenPoint{(control.rect.left + control.rect.right) / 2, (control.rect.top + control.rect.bottom) / 2};
    return {};
}
// 실제 메뉴 상태·활성 조건·원본 미션 로드 결과를 한 파일에 기록한다.
std::string UberGump::Report() const {
    std::ostringstream out;
    out << "phase\t" << static_cast<int>(state_.phase) << "\npage\t" << Field(pageName_) << "\npopup\t" << Field(popup_) << "\n";
    out << "size\t" << client_.GetScreen().Width() << '\t' << client_.GetScreen().Height() << "\n";
    out << "paused\t" << client_.Paused() << "\nsound\t" << client_.Configuration().GetInt("sound") << "\n";
    out<<"map_menu\t"<<page_.menuColumns<<'\t'<<mapCatalog_->Cached().count<<'\t'<<mapCatalog_->Cached().dirty<<'\t'<<mapFileSpec_.Current()<<'\n'; // 실제 표시/캐시 상태만 관찰하며 선택/로딩 성공을 합성하지 않는다.
    out << "thunder_processes\t" << client_.ThunderFlashCount() << '\n';
    out<<"game_menu_bar\t"<<(gameMenuState_ ? gameMenuState_->menuSerial : 0U)<<'\t'<<GameMenuOpen()<<'\t'<<(gameMenuState_ ? gameMenuState_->checksum : 0)<<'\n'; // 같은serial조회와기존SP검산을관찰한다.
    out<<rawButtons_->Report()<<'\n';
    out<<rawButtons_->Menus()->Report();
    out<<warnings_->Report();
    out.precision(12); out<<"time\t"<<client_.Time().game<<'\t'<<client_.Time().wall<<"\n";
    out << "fullscreenMarker\t" << client_.Files().TryRead("fullscreenStateFile.dat").has_value() << "\n";
    out << "musicVolume\t" << client_.Configuration().GetInt("musicVolume") << "\nsoundVolume\t" << client_.Configuration().GetInt("soundVolume") << "\n";
    if (mission_) {
        out << "mission\t" << Field(mission_->Name()) << '\t' << Field(mission_->MissionType()) << '\t' << fortObjects_ << "\n";
        out<<"mission_progress\t"<<client_.Configuration().GetInt("Done"+mission_->Name())<<'\n'; // 실제안내Config명령과같은저장값을검사보고에서읽는다.
        if (missionRuntime_) {
            const auto& gate=missionRuntime_->gate;const auto& state=missionRuntime_->state; // 실제 Start의 최종 표식과 중간 쓰기를 읽는다.
            out<<"mission_init\t"<<missionRuntime_->init.namePrefix<<'\t'<<gate.tutorialNumber<<'\t'<<missionNumber_<<'\t'<<tutorialSeen_<<'\t'<<initLoginSubscriber_<<'\t'<<state_.Pending()<<'\n'; // 로드 전/후의 실제 전역과 요청을 읽는다.
            if (missionFortRequest_) out<<"mission_fort_request\t"<<missionFortRequest_->next[0]<<'\t'<<missionFortRequest_->sequence<<'\t'<<missionFortRequest_->next[31]<<'\t'<<missionFortRequest_->zone.size()<<'\t'<<missionFortRequest_->consumed<<'\n'; // 작성한 요청 캡처와 실제 복제 이미지의 소비를 구별한다.
            out<<"mission_start\t"<<gate.started<<'\t'<<state.sequence<<'\t'<<gate.demo<<'\t'<<gate.startSeconds<<'\t'<<gate.demoEndSeconds<<'\n';
            out<<"mission_gate\t"<<gate.tutorialNumber<<'\t'<<gate.stage<<'\t'<<gate.lockFrames<<'\t'<<gate.scriptPaused<<'\t'<<gate.clearFrames<<'\t'
                <<(missionRuntime_->tutorial ? 1 : 0)<<'\t'<<gate.nextStageSeconds<<'\t'<<gate.tellAtSeconds<<'\n'; // 동일 현재 상태의 관측만 제공한다.
            if (world_) {
                out<<"mission_bridges\t"<<world_->Session().TotalMade().BuiltForGenus(4)<<'\n'; // 단계 판정과 같은 누적 장부를 관찰하며 현재 살아 있는 다리 수와 구별한다.
                out<<"mission_options\t"<<unsigned(world_->Session().BattleOption(2))<<'\t'<<unsigned(world_->Session().BattleOption(4))<<'\t'<<world_->Session().EnergySettings().radiusSquared<<'\n'; // 같은 현재 옵션/공급 범위를 읽기만 한다.
                const auto& settings=world_->Session().MissionStartSettings(); // 원본 게임 명령이 공유할 같은 제한 전역이다.
                out<<"mission_rules\t"<<settings.denySalvage<<'\t'<<settings.denyAscend<<'\t'<<settings.allowAnyCapture<<'\t'<<settings.ordinaryGeysers<<'\n';
            }
        }
    }
    if (world_) out<<world_->Report();
    // 화면/월드와 같은 프레임에 위치 효과음이 실제로 읽는 여섯 값을 기록한다. 소리 없는 실행에는 항목을 만들지 않는다.
    if (client_.Audio()) {
        const auto& view=client_.Audio()->View();
        out<<"audio_view\t"<<view.cameraX<<'\t'<<view.cameraY<<'\t'<<view.left<<'\t'<<view.top<<'\t'<<view.right<<'\t'<<view.bottom<<'\n';
        const auto& scene=client_.Audio()->Scene();
        out<<"weather\t"<<scene.index<<'\t'<<scene.tint<<'\t'<<scene.paletteDirty;
        // 현재 팔레트에서 계산한 네 원소 색도 같은 관찰에 기록한다.
        for (const auto tint:scene.tints) out<<'\t'<<tint;
        out<<'\n';
    }
    // 그리기와 같은 판정 표를 기록한다.
    for (const auto& control : input_.Controls()) {
        const auto& label = labels_.at(static_cast<std::size_t>(control.id));
        const auto item = std::find_if(page_.items.begin(),page_.items.end(),[&label](const DialogItem& value) { return value.label == label; });
        const bool checked = item != page_.items.end() && item->checked;
        out << "control\t" << Field(label) << '\t' << control.enabled << '\t' << control.menu << '\t'
            << control.rect.left << '\t' << control.rect.top << '\t' << control.rect.right << '\t' << control.rect.bottom << '\t' << checked << "\n";
    }
    return out.str();
}
}
