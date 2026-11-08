//원문 마우스의 후보 재조회·부분 효과·현재 폴링 좌표와 사건 좌표 구별을 보존한다.
#include "client/ClientMetaPointerInput.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientChallengeGlobals.h" //실제 현재 참가자/전투·영역 TYPE의 완전한 형식을 사용한다.
#include "client/ClientLocalStateGlobals.h"
#include "client/ClientDiagnostics.h"
#include "client/ClientAudio.h"
#include "client/RawCampaignSession.h"
#include "client/RawCampaignFactoryDisplay.h"
#include "client/RawMenuControls.h"
#include "client/RawUserPick.h"
#include "client/RawUserSelection.h"
#include "client/RawUserHighlight.h"
#include "client/RawCarrierInput.h"
#include "client/RawCarrierDropPoint.h"
#include "client/RawCursorPosition.h"
#include "client/RawCampaignCursor.h"
#include "client/ClientMetaObjectInput.h"
#include "client/SpecialPointerTrace.h"
#include "client/ObjectOwnerCommands.h"
#include "client/SelectionKeyInput.h" //메타도 같은 H/N/P/R/F4/F5/F6 부모와 일반 View 큐를 공유한다.
#include "client/NextUnitSelection.h" //다음 유닛은 현재 같은 Pool/선택과 실제 카메라를 사용한다.
#include "client/PriestSelectionShortcut.h" //P/R의 실제 참여 조회와 Client 장기 누적 카운터를 빌린다.
#include "client/ProductionShortcuts.h" //원문 공통 키 처리 순서에서 실제 생산 단축 명령만 예약한다.
#include "client/GameMenuCommands.h" //현재 View/지식 명령의 원문 주소표를 공유한다.
#include "o/RawSquidFinder.h"
#include "o/RawObjectTraceMain.h"
#include "o/OriginalText.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::client { //메타도 실제 현재 게임 전역을 읽고 별도 입력/선택 사본을 만들지 않는다.
namespace { //물리 필드·원문 FTOL 하위 DWORD만 지역화한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at,std::size_t width=4) { if (at>bytes.size() || width>bytes.size()-at) throw std::out_of_range("메타 포인터의 실제 필드 읽기 범위");std::uint32_t value{};for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[at+byte])<<(8*byte);return value; } //선택한 폭만 낮은 BYTE부터 조립한다.
std::uint32_t Ftol(float value) { if (!std::isfinite(value) || static_cast<double>(value)<-0x1p63 || static_cast<double>(value)>=0x1p63) return 0;return static_cast<std::uint32_t>(static_cast<std::int64_t>(value)); } //4e49c0 int64 절삭의 하위 DWORD이며 indefinite의 하위는0이다.
std::uint32_t Pack(std::array<float,2> point) { return (Ftol(point[0])<<16)|(Ftol(point[1])&65535U); } //사건 좌표 역투영의 X상위/Y하위 WORD를 순서대로 포장한다.
}
struct ClientMetaPointerInput::Impl { //같은 실제 저장소의 원본 처리기와 현재 화면 callback 수명이다.
    Client& client;RawCampaignSession& session;RawCampaignFactoryDisplay& display;RawMenuControls& menus; //메타 Runtime보다 오래 사는 현재 owner를 빌린다.
    std::function<ScreenRect()> view;std::function<std::uint32_t()> hide;std::function<void(std::string_view)> tell; //현재 viewport/숨김/실제 Tell은 선택 위치에서 조회한다.
    std::unique_ptr<RawCarrierInput> carrier;std::unique_ptr<RawUserHighlight> highlight;std::unique_ptr<RawUserSelection> selection; //공통 MetaMap 장치를 사용하며 마지막에 만든 선택부터 먼저 반납한다.
    Impl(Client& c,RawCampaignSession& s,RawCampaignFactoryDisplay& d,RawMenuControls& m,std::function<ScreenRect()> viewport,std::function<std::uint32_t()> hidden,std::function<void(std::string_view)> query):client(c),session(s),display(d),menus(m),view(std::move(viewport)),hide(std::move(hidden)),tell(std::move(query)) { //실제 준비된 메타 Session에서만 한 번 연결한다.
        if (!view || !hide || !tell || !session.Core().ownerMode.challenge) throw std::invalid_argument("메타 포인터의 실제 현재 모드/장치 계약");
        UserHighlightHooks marks;marks.selectCursor=[this](auto code) { client.GetCursor().Set(code); };marks.project=[this](auto x,auto y) { const auto p=Project({x,y});return std::array<std::uint32_t,2>{std::bit_cast<std::uint32_t>(p[0]),std::bit_cast<std::uint32_t>(p[1])}; };marks.move=[this](auto sid,auto x,auto y,auto force) { carrier->Highlight(sid,{std::bit_cast<float>(x),std::bit_cast<float>(y)},force); };
        highlight=std::make_unique<RawUserHighlight>(Game().selection.HighlightState(),std::move(marks));CarrierInputHooks movement;movement.selectCursor=[this](auto code) { client.GetCursor().Set(code); };movement.highlight=[this](auto mode) { highlight->Highlight(mode); };movement.multiple=[this] { return Game().selection.multipleCount>0 ? 1U:0U; };
        movement.assertion=[this](auto line) { client.SurfaceStorage()->AssertDisplay("sid","\\Ns\\O\\Carrier.cpp",line); };movement.showSacrifice=[this](auto target) { session.ShowSacrificeMenu(target,tell); };movement.earlyLocal=[] { throw std::logic_error("원본4cf960 조기 희생 안내 UI 미연결"); };movement.earlyRemote=[](auto) { throw std::logic_error("메타 운반체의 원격 조기 희생 UI 경계 미연결"); };movement.marker=[](auto,auto) { throw std::logic_error("메타 이동 진단 표시의 실제 marker 경계 미연결"); };movement.refreshCursor=[] { throw std::logic_error("메타 이동 진단의 실제 커서 갱신 경계 미연결"); };
        carrier=session.MakeCarrierInput(std::move(movement));UserSelectionHooks select;select.paused=[this] { return client.Paused() ? 1U:0U; };select.enabled=[this] { return Game().selection.enabled; };select.highlight=[this](auto mode) { highlight->Highlight(mode); };select.dump=[this](auto sid) { client.TraceObject(sid.value); };
        select.assertion=[this] { client.SurfaceStorage()->AssertDisplay("selectedSid",".\\Userinput.cpp",0xf01); };select.previewAssertion=[] { throw std::logic_error("원본 Carrier.cpp !s.isForm 진단"); };select.appendMultiple=[this](auto sid) { Game().selection.AppendMultiple(sid); };select.removeMultiple=[this](auto sid) { Game().selection.RemoveMultiple(sid); };select.writeEnabled=[this](auto value) { Game().selection.enabled=value; };
        selection=session.MakeUserSelection(Game().selection.SelectionState(),std::move(select));
    }
    ClientGameplayGlobals& Game() const { return *client.GameplayGlobals(); } //선택/좌표·special/editor는 매번 같은 현재 Client owner다.
    std::array<float,2> Project(ScreenPoint point) const { return RawCursorPosition::ScreenToWorld(point.x,point.y,Game().Camera().Snapshot().worldBits); } //4971e0의 현재 snapshot을 사용한다.
    bool Inside() const { const auto area=view();const auto point=Game().selection.mouse;return area.left<=point.x && point.x<area.right && area.top<=point.y && point.y<area.bottom; } //239c0은 사건이 아닌 앞선 폴링 좌표를 읽는다.
    std::span<const std::uint8_t> Raw(std::uint32_t sid) const { const auto core=session.Core();if (sid>65535U || sid>=core.pool.Capacity()) throw std::out_of_range("메타 포인터의 실제 SID WORD/Pool 범위");return core.pool.Slot(o::Sid{static_cast<std::uint16_t>(sid)}); } //상태/free/active 필터를 추가하지 않는다.
    bool Valid(std::uint32_t sid) const { return sid>0 && sid<session.Core().pool.Capacity(); } //40ea40의 실제 물리 범위만 판정한다.
    std::uint32_t Pick(InputEvent event,std::uint32_t force) const { //기존4d5840의 동일 finder/투명 픽셀/현재 메타 배율을 사용한다.
        const auto core=session.Core();o::RawSquidFinder finder(core.pool,core.hash,core.types);UserPickHooks hooks;hooks.paused=[this] { return client.Paused() ? 1U:0U; };hooks.localOwner=[this] { return Game().player.battleOwner; };hooks.metamap=[&core] { return core.ownerMode.challenge ? 1U:0U; };hooks.hideBuildings=hide;hooks.project=[this](auto x,auto y) { return Project({x,y}); };
        hooks.begin=[&finder](auto at,auto rx,auto ry) { const auto p=RawCarrierDropPoint::Snap(at);const auto x=static_cast<std::int32_t>(p[0]),y=static_cast<std::int32_t>(p[1]);return finder.Begin({x-rx,y-ry,x+rx,y+ry},0).value; };hooks.next=[&finder] { return finder.Next().value; };hooks.center=[&core](auto sid) { return core.neighbors.Center(sid); };
        hooks.hit=[this](auto sid,auto x,auto y) { return display.Hit(sid,x,y,Game().Camera().Snapshot().pixels,Game().player.battleOwner,[this](auto object,auto px,auto py) { session.DrawBridgeAdditional(object,px,py); }); };return RawUserPick(core.pool,core.types,std::move(hooks)).Pick(event.x,event.y,force).value;
    }
    void Open(std::uint32_t sid,InputEvent event) { if (InputClientMetaObject(client,menus,sid,event,tell)) client.WorldAudio().Sounds().PlayByName("opengump.wav",0,0,0,0,0); } //현재 실제+ac가 비영이면 메뉴 없이 입력만 예약해도 원문 열기 음향을 재생한다.
    void Queue(std::uint32_t code,std::uint32_t value) { client.Input().PushCommand(code,0,std::bit_cast<std::int32_t>(value)); } //물리 입력이나 상태 직접 변경 대신 같은 실제 InputFIFO만 예약한다.
    void Primary(std::uint32_t sid,std::uint32_t force,bool control,InputEvent event) { //일반 선택 뒤 현재 special 지점 진단·편집 후처리를 원문 순서로 실행한다.
        static_cast<void>(selection->Select(sid,force,1));const auto core=session.Core();o::RawSquidFinder finder(core.pool,core.hash,core.types);SpecialPointerTraceHooks trace;trace.project=[this](auto x,auto y) { return Project({x,y}); };trace.begin=[&finder](auto point) { const auto p=RawCursorPosition::TruncatePoint(point);const auto x=static_cast<std::int32_t>(p[0]),y=static_cast<std::int32_t>(p[1]);return finder.Begin({x,y,x,y},0).value; };trace.next=[&finder] { return finder.Next().value; };
        trace.trace=[this,&core](auto number) { o::TraceRawObjectVirtual(core.pool,core.types,client.DiagnosticGlobals().suppress,number,[this](auto level,auto text,auto) { std::size_t prefix{};while (prefix+1<text.size() && text[prefix]=='~') prefix+=2;client.WriteDiagnosticLog(level,text,prefix); }); }; //직접 가상70의 색 접두만 로그에서 구별하며 Form 재귀로 바꾸지 않는다.
        TraceSpecialPointer(control,Game().player.special,event,trace);EditorPrimarySelectionHooks edit;edit.editor=[this] { return Game().world.editor; };edit.selected=[this] { return Game().selection.selected; };edit.localOwner=[this] { return Game().player.battleOwner; };edit.readOwner=[this](auto number) { return Read(Raw(number),34,1); };edit.flags2=[this](auto number) { const auto core=session.Core();const auto type=Read(Raw(number),10,1);if (type>=core.types.size()) throw std::out_of_range("메타 편집 후처리의 현재 TYPE 범위");return core.types[type].flags2; };edit.assignOwner=[this](auto number,auto owner) { session.SetObjectOwner(o::Sid{static_cast<std::uint16_t>(number)},owner); };edit.cycleBridge=[this](auto number) { session.CycleBridgeQuality(number); };edit.follow=[this](auto number) { session.FollowObjectOwner(number); };FinishEditorPrimarySelection(control,edit);
    }
    void CenterOn(std::uint32_t sid) { CameraHomeState state;state.width=client.GetScreen().Width();state.height=client.GetScreen().Height();session.CenterCameraOn(sid,state,[this,&state](auto,auto) { session.CommitRequestedCamera(state.width,state.height); }); } //같은 현재 화면/Session의 실제 즉시 카메라 적용을 사용한다.
    void ViewPriest(std::uint32_t owner) { CameraHomeState state;state.width=client.GetScreen().Width();state.height=client.GetScreen().Height();session.ViewYourPriest(owner,state,[this,&state](auto,auto) { session.CommitRequestedCamera(state.width,state.height); }); } //실제 참여 조회에서0이면 CenterOn도 실행하지 않는다.
    bool Event(InputEvent event) { //공통 GUI/모달 이후의 현재 메타 키와 포인터를 원문 순서로 소비한다.
        const auto code=event.code&0x03ffffffU;const bool mouse=code==InputCode::kLeftButton || code==InputCode::kRightButton;if ((event.code&InputCode::kRelease) || client.Paused()) return mouse;
        std::int32_t snap=0,cannot=0;client.Configuration().ReadInt("cursorSnapToGrid",snap);client.Configuration().ReadInt("cursorShowCannotPlace",cannot);auto& cursor=Game().CursorGlobals();cursor.SnapToGrid()=std::bit_cast<std::uint32_t>(snap);cursor.Placement().placementFlags=std::bit_cast<std::uint32_t>(cannot);
        if (!session.CursorDropSettings().lockDepth && session.Cursor().HandleInput(event,cursor.RotationMode())) return true;
        if (QueueProductionShortcut(event,client.Paused(),{[] { return kProductionBridgeCommand; },[this](auto command,auto slot,auto zero) { client.Input().PushCommand(command,slot,zero); },[] { return kProductionRepeatCommand; }})) return true;
        SelectionKeyInputHooks keys;keys.paused=[this] { return client.Paused(); };keys.command=[](auto address) { return GameMenuCommandValue(address).value(); };keys.localOwner=[this] { return session.LocalOwner(); };keys.queue=[this](auto command,auto first,auto second) { client.Input().PushCommand(command,std::bit_cast<std::int32_t>(first),std::bit_cast<std::int32_t>(second)); };
        keys.next=[this](auto add) { const auto core=session.Core();NextUnitSelectionHooks hooks;hooks.localOwner=[this] { return session.LocalOwner(); };hooks.paused=[this] { return client.Paused() ? 1U:0U; };hooks.select=[this](auto sid,auto force,auto activate) { static_cast<void>(selection->Select(sid,force,activate)); };hooks.center=[this](auto sid) { CenterOn(sid); };hooks.multiple=[this](auto sid,auto value) { selection->ToggleMultiple(sid,value); };SelectNextMovingUnit(core.pool,core.types,Game().selection.selected,add,hooks); }; //원문은 후보 선택 뒤 현재 정지를 다시 읽으며 Shift 비트 전체를 보존한다.
        keys.priest=[this,queue=keys.queue] { PriestSelectionShortcutHooks hooks;hooks.localOwner=[this] { return session.LocalOwner(); };hooks.find=[this](auto owner) { return session.FindPriestParticipation(owner); };hooks.select=[this](auto sid,auto force,auto activate) { static_cast<void>(selection->Select(sid,force,activate)); };hooks.viewCommand=[] { return GameMenuCommandValue(0x005429d4U).value(); };hooks.queue=queue;SelectPriestByKey(Game().selection.selected,Game().selection.priestSelections,hooks); }; //조회 전 SID와 장기 누적 카운터는 같은 현재 Client 주소다.
        if (DispatchSelectionKeyInput(event,keys)) return true;if (!mouse) return false;const bool control=(event.code&InputCode::kControl)!=0;
        if (code==InputCode::kRightButton) {
            if (Game().selection.mode) { highlight->Highlight(0);static_cast<void>(selection->Select(0,0,0));return true; }const auto sid=Pick(event,control && (Game().player.special || Game().world.editor) ? 1U:0U);
            if (Valid(sid)) { if (Game().world.editor) { static_cast<void>(selection->Select(sid,0,0));if (Game().world.editor) session.FollowObjectOwner(sid); }Open(sid,event);return true; }
            if (!session.Core().ownerMode.challenge || !Game().player.playerKnown) static_cast<void>(selection->Select(0,0,0));
            else if (!Read(client.ChallengeGlobals()->Participant(Game().player.battleOwner),0xa4)) { const auto parameter=Pack(Project({event.x,event.y}));menus.OpenMetaSailMenu(parameter,{event.x,event.y},[this](auto key) { const auto bytes=o::EncodeDisplayText(client.Translate(key));return std::string(bytes.begin(),bytes.end()); }); }return true;
        }
        const auto mode=Game().selection.mode;
        if (mode==2 || mode==5) { const auto force=control && Game().player.special ? 1U:0U;const auto sid=Pick(event,force);if (event.code&InputCode::kShift) selection->ExtendSelection(sid,force);else { const auto p=RawCursorPosition::TruncatePoint(Project({event.x,event.y}));client.Input().PushCommand(kWorldDestinationSelectedCommand,std::bit_cast<std::int32_t>(Ftol(p[0])),std::bit_cast<std::int32_t>(Ftol(p[1])));selection->MoveMultiple(std::bit_cast<std::uint32_t>(p[0]),std::bit_cast<std::uint32_t>(p[1]),[this](auto number,auto x,auto y) { carrier->Commit(number,{std::bit_cast<float>(x),std::bit_cast<float>(y)}); });carrier->Commit(Game().selection.selected,{p[0],p[1]}); }return true; }
        if (mode==1) { const auto sid=Pick(event,0);if (sid) { session.Cursor().TakeObject(sid,0);client.WorldAudio().Sounds().PlayByName("selectTab.wav",0,0,0,0,0); }highlight->Highlight(0);return true; }
        if (mode==6) { if (Inside()) { Queue(0x0200006cU,Pack(Project({event.x,event.y})));highlight->Highlight(0); }return true; }
        if (session.Core().ownerMode.challenge) { const auto sid=Pick(event,control && (Game().player.special || Game().world.editor) ? 1U:0U);if (Valid(sid)) { const auto type=Read(Raw(sid),10,1);if (type==client.ChallengeGlobals()->BattleType()) { Open(sid,event);return true; }if (type==client.ChallengeGlobals()->ZoneGlyphType()) { const auto at=Read(Raw(sid),0xc,2)*144U;Queue(0x020000e8U,Read(client.LocalStateGlobals()->MetaZoneRecords(),at));return true; } }
            if (Inside() && Game().player.playerKnown) { const auto own=client.ChallengeGlobals()->Participant(Game().player.battleOwner);const auto value=Pack(Project({event.x,event.y}));if (!Read(own,0xa4)) Queue(0x0200006cU,value);else tell("QueryLeaveChalBattle("+std::to_string(std::bit_cast<std::int32_t>(value))+")"); }
        }
        const auto force=control && Game().player.special ? 1U:0U;Primary(Pick(event,force),force,control,event);return true;
    }
    void Poll(InputEvent pointer) { if (pointer.code&InputCode::kOutside) return;if (!session.CursorDropSettings().lockDepth) { session.Cursor().Update(pointer.x,pointer.y);return; }if (Game().selection.mode==2 || Game().selection.mode==5) { const auto p=RawCursorPosition::TruncatePoint(Project(Game().selection.mouse));carrier->Highlight(Game().selection.selected,{p[0],p[1]},0); } } //4d6b46과 이동 강조는 같은 현재 폴링 좌표를 사용한다.
};
ClientMetaPointerInput::ClientMetaPointerInput(Client& client,RawCampaignSession& session,RawCampaignFactoryDisplay& display,RawMenuControls& menus,std::function<ScreenRect()> view,std::function<std::uint32_t()> hide,std::function<void(std::string_view)> tell):impl_(std::make_unique<Impl>(client,session,display,menus,std::move(view),std::move(hide),std::move(tell))) {} //같은 현재 소유자의 원본 몸체를 한 번 조립한다.
ClientMetaPointerInput::~ClientMetaPointerInput()=default; //비소유 Session 참조는 바깥 owner가 먼저 해제하지 않아야 한다.
bool ClientMetaPointerInput::Event(InputEvent event) { return impl_->Event(event); } //실제 동일 입력 몸체에 위임한다.
void ClientMetaPointerInput::Poll(InputEvent pointer) { impl_->Poll(pointer); } //별도 프레임/통신 진행 없이 현재 포인터만 위임한다.
void ClientMetaPointerInput::ResetSelection() { static_cast<void>(impl_->selection->Select(0,0,0)); } //정지 중에도 원문의 enabled/demo·정리·가상 통지 순서를 유지한다.
void ClientMetaPointerInput::CenterOn(std::uint32_t sid) { impl_->CenterOn(sid); } //카메라 저장소를 복사하거나 별도 월드를 만들지 않는다.
void ClientMetaPointerInput::ViewPriest(std::uint32_t owner) { impl_->ViewPriest(owner); } //원문 명령의 owner DWORD를 축소하지 않는다.
}
