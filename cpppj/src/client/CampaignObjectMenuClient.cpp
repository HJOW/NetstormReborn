// 메뉴의 게임 판단은 기존 원본 모듈에 맡기고 실제 Client의 화면/번역/입력 수명만 연결한다.
#include "client/CampaignObjectMenuClient.h"
#include "client/InventoryAboutMenu.h"
#include "client/ClientMain.h"
#include "client/ClientAudio.h"
#include "client/GameWorld.h"
#include "client/RawButtonControls.h"
#include "client/RawMenuControls.h"
#include "client/RawObjectMenuSlots.h"
#include "o/OriginalText.h"
#include "o/RawMissionStart.h"
#include <algorithm>
#include <bit>
#include <iostream>
#include <map>
#include <stdexcept>
#include <utility>
#include <vector>

namespace netstorm::client {
namespace {
// 원본 메뉴의 실제 고정%s/%d/%%만 처리한다. 번역은 호출자의 원본 조회 위치에서 수행한다.
std::string PrintMenu(std::string_view pattern,std::span<const PriestMenuArgument> values) {
    std::string result;std::size_t argument{};
    // 각 원본 형식과 인자를 순서대로 소비하고 정의되지 않은 서식은 거부한다.
    for (std::size_t index=0;index<pattern.size();++index) {
        if (pattern[index]!='%') { result+=pattern[index];continue; }
        if (++index>=pattern.size()) throw std::logic_error("실제 객체 메뉴 서식 끝 오류");
        if (pattern[index]=='%') { result+='%';continue; }
        if (argument>=values.size()) throw std::logic_error("실제 객체 메뉴 서식 인자 부족");
        if (pattern[index]=='s') result+=std::get<std::string>(values[argument++]);
        else if (pattern[index]=='d') result+=std::to_string(std::get<std::int32_t>(values[argument++]));
        else throw std::logic_error("실제 객체 메뉴 미복원 서식");
    }
    if (argument!=values.size() || result.size()>=256) throw std::length_error("실제 객체 메뉴 원본256BYTE 서식 계약 오류");
    return result;
}
}
// 클라이언트 수명의 정적 문자열/Chat 보조 상태와 현재 메뉴의 실제 Session 참조다.
struct CampaignObjectMenuClient::Impl {
    Client& client; // 현재 설정/실제 입력/원본 번역의 유일한 Client다.
    RawButtonControls& buttons; // 실제 등록/부착/Form/지연 삭제 트리다.
    std::function<void(std::string_view)> tell; // 현재 입력 callback 이후 실제 Tell을 예약한다.
    GameWorld* world{}; // 메뉴 체인과 객체 부착 Form이 빌리는 현재 월드다.
    std::unique_ptr<RawCampaignObjectMenus> chain; // 실제 사제/캐리어/회수/Player/정보 몸체를 함께 소유한다.
    std::unique_ptr<RawObjectMenuOpen> opener; // 같은 현재 객체의 공통 열기와 위치 계산이다.
    std::uint32_t root{},staticSerial{0x60000000U},localAddress{}; // 실제 루트/호스트 정적 문자열 범위/5953fc의 초기0 주소다.
    std::map<std::uint32_t,std::string> staticStrings; // 페이지/80칸 Script 고리와 독립인 정적 메뉴 문자열 수명이다.
    std::vector<std::uint32_t> ignored; // 원본548750/548754의 현재 subscriber 목록이다. 초기 빈 목록을 유지한다.
    std::string chatInput; // 원본 Chat 입력 텍스트의 현재 초기 빈 바이트다. 편집 Gump 연결은 후속이다.
    // 현재 실제 UI 대상을 빌리기만 하며 메뉴를 만들거나 게임을 진행하지 않는다.
    Impl(Client& source,RawButtonControls& tree,std::function<void(std::string_view)> notify):client(source),buttons(tree),tell(std::move(notify)) {
        if (!tell || !buttons.Menus()) throw std::invalid_argument("실제 객체 메뉴 Client 경계 누락");
    }
    // 현재 번역을 원본 글꼴/서식이 쓰는 Windows-1252 바이트로 돌린다.
    std::string Translate(std::string_view key) const {
        const auto bytes=o::EncodeOriginalText(client.Translate(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(key.data()),key.size()})));
        return {bytes.begin(),bytes.end()};
    }
    // 원본 정적 문자열은 값별로 안정된 호스트 토큰을 보유하며 메뉴를 닫아도 반납하지 않는다.
    std::uint32_t Static(std::string_view text) {
        // 기존 토큰은 같은 값을 요청할 때 계속 사용한다.
        for (const auto& [token,value]:staticStrings) if (value==text) return token;
        if (staticSerial==0xffffffffU) throw std::overflow_error("실제 객체 메뉴 정적 문자열 토큰 소진");
        const auto token=++staticSerial;staticStrings.emplace(token,text);return token;
    }
    // 원본42f640의 현재 Identity 조회와42e9e0의 콜론 없는 입력 경로를 사용한다.
    std::uint8_t Whispered(std::uint32_t subscriber) const {
        o::PlayerIdentityRecord record{};o::RawPlayerIdentity::Initialize(record,0);
        // 원본은 검색 전 subscriber DWORD만 기록한다.
        for (std::size_t index=0;index<4;++index) record[4+index]=static_cast<std::uint8_t>(subscriber>>(index*8));
        if (!chain->ResolveIdentity(record)) return 0;
        if (chatInput.find(':')!=std::string::npos) throw std::logic_error("객체 메뉴의 현재 Chat 접두 이름 해석0042dc70 미연결");
        // 콜론이 없으면42e9e0은 첫 바이트를NUL로 바꾼 뒤 strstr를 실행한다. 빈 이름만 일치한다.
        return record[8]==0 ? 1U : 0U;
    }
    // 같은 실제 작성기/현재 게임/번역/정적 수명에 모든 UI 경계를 연결한다.
    CampaignObjectMenuUi Ui(const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& raw) {
        CampaignObjectMenuUi ui;ui.chat=client.ChatRecords();
        std::string help; // NUL/빈 문자열의 차이를 실제 현재 설정에서 유지한다.
        if (client.Configuration().ReadString("helpWord",help)) { const auto bytes=o::EncodeOriginalText(help);ui.objectState.helpWord=std::string(bytes.begin(),bytes.end()); }
        ui.objectState.denySalvageRead=[this] { return std::bit_cast<std::uint32_t>(world->Session().MissionStartSettings().denySalvage); };
        ui.identity.address=[this] { return localAddress; };ui.identity.unknownName=[] { return std::string_view("SOMEONE"); };
        ui.priest.format=ui.carrier.format=ui.object.format=[this](std::string_view key,std::span<const PriestMenuArgument> arguments) { return PrintMenu(Translate(key),arguments); };
        ui.priest.description=ui.carrier.description=[this](std::uint32_t type) { const auto types=world->Session().Core().types;if (type>=types.size()) throw std::out_of_range("현재 객체 메뉴 설명 TYPE 범위");return Translate(types[type].description); };
        ui.priest.tell=[this](std::string_view key) { tell(key); };
        ui.factory.format=ui.priest.format;ui.factory.description=ui.priest.description;
        ui.factory.helpWord=[this] { std::string word;if (!client.Configuration().ReadString("helpWord",word)) throw std::logic_error("Factory 실제 도움말 원문 미설치");const auto bytes=o::EncodeOriginalText(word);return std::string(bytes.begin(),bytes.end()); };
        ui.player.translate=[this](std::string_view key) { return Translate(key); };ui.player.print=PrintMenu;
        ui.player.allowSend=[this] { return world->Session().BattleOption(13)!=0; };ui.player.allowOffer=[this] { return world->Session().BattleOption(11)!=0; };ui.player.allowBreak=[this] { return world->Session().BattleOption(12)!=0; };
        ui.player.ignored=[this](std::uint32_t subscriber) { return std::find(ignored.begin(),ignored.end(),subscriber)!=ignored.end(); };
        ui.player.whispered=[this](std::uint32_t subscriber) { return Whispered(subscriber); };
        ui.player.parseAddress=[](std::string_view,std::uint32_t) { throw std::logic_error("원격 Player 메뉴 주소 해석 미연결"); };
        ui.player.reliability=[](std::uint32_t)->std::uint32_t { throw std::logic_error("메타맵 Player 메뉴 신뢰도 미연결"); };
        ui.player.assertion=[](PlayerMenuAssertion assertion) { std::clog<<"객체 Player 메뉴 원본 진단 "<<static_cast<int>(assertion)<<'\n'; };
        ui.info.translate=ui.player.translate;ui.info.description=ui.priest.description;ui.info.print=PrintMenu;
        ui.info.substitute=[this](std::string_view source) { const auto text=client.Configuration().SubstituteLocal(o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(source.data()),source.size()}));const auto bytes=o::EncodeOriginalText(text);return std::string(bytes.begin(),bytes.end()); };
        BindPriestMenuControls(ui.priest,widgets,controls,raw);BindCarrierMenuControls(ui.carrier,controls);BindPlayerMenuControls(ui.player,widgets,controls,raw);BindObjectMenuInfoControls(ui.info,controls);
        BindFactoryMenuControls(ui.factory,widgets,controls,raw,[this](std::string_view text) { return Static(text); });
        BindObjectMenuControls(ui.object,controls,raw,client.Configuration(),[this](std::string_view text) { return Static(text); });
        return ui;
    }
    // 같은 원본+a8 주소에 이미 복원된 실제 부모 체인만 분배한다. 나머지 파생은 명시 거부한다.
    void Populate(o::Sid sid) const {
        const auto raw=world->Session().Core().pool.Slot(sid);std::uint32_t vt{};
        // 실제 슬롯의 비정렬 DWORD를 그대로 읽는다.
        for (std::size_t index=0;index<4;++index) vt|=std::uint32_t(raw[index])<<(index*8);
        switch (ObjectMenuSlots(vt).populate) {
        case 0x454c00:chain->OpenFactory(sid);break;
        case 0x494870:chain->OpenPriest(sid);break;
        case 0x4288f0:chain->OpenCarrier(sid);break;
        case 0x44cbd0:chain->OpenDamageable(sid);break;
        case 0x4b1270:chain->OpenObject(sid);break;
        default:throw std::logic_error("현재 객체 메뉴의 실제 파생+a8 미연결");
        }
    }
    // 같은 실제 선택을 먼저 실행하는 운반체/사제/아처와 공통+ac를 구분한다.
    std::uint32_t Dispatch(o::Sid sid,const InputEvent& event) const {
        const auto bytes=world->Session().Core().pool.Slot(sid);std::uint32_t vt{};
        // 생성자 VT와 현재 슬롯 VT를 혼동하지 않는다.
        for (std::size_t index=0;index<4;++index) vt|=std::uint32_t(bytes[index])<<(index*8);
        const auto common=[this](o::Sid object,const InputEvent& input) { return opener->Open(object,input); };
        switch (ObjectMenuSlots(vt).open) {
        case 0x4754b0:return opener->OpenFactory(sid,event);
        case 0x4adb30:return common(sid,event);
        case 0x4268c0:case 0x492080:return world->OpenSelectionMenu(sid,event,false,common);
        case 0x41e660:return world->OpenSelectionMenu(sid,event,true,common);
        default:throw std::logic_error("현재 객체 메뉴의 실제 파생+ac 미연결");
        }
    }
};
// 같은 Client와 메뉴 트리의 수명에만 연결한다.
CampaignObjectMenuClient::CampaignObjectMenuClient(Client& client,RawButtonControls& buttons,std::function<void(std::string_view)> tell):impl_(std::make_unique<Impl>(client,buttons,std::move(tell))) {}
// 대상 Session은 호출자가 Clear까지 유지한다.
CampaignObjectMenuClient::~CampaignObjectMenuClient() { Clear(); }
// 실제 객체 메뉴는 게임 시계를 멈추지 않고 원본 controlled 생성/부착을 사용한다.
void CampaignObjectMenuClient::Open(GameWorld& world,o::Sid sid,const InputEvent& event) {
    Clear();auto& s=*impl_;s.world=&world;
    s.buttons.Menus()->OpenAttached([&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& raw) {
        s.chain=world.Session().MakeObjectMenus(s.Ui(widgets,controls,raw));ObjectMenuOpenHooks hooks;
        hooks.delegate=[&s](o::Sid object,const InputEvent& input) { return s.Dispatch(object,input); };
        hooks.beginFancy=[&] { widgets.BeginFancy(); };hooks.summary=[&s](o::Sid object) { s.chain->Summary(object); };hooks.populate=[&s](o::Sid object) { s.Populate(object); };
        hooks.menuWidth=[&] { return s.buttons.Menus()->Node(raw.Current())->baseWidth; };hooks.screenWidth=[&s] { return s.client.GetScreen().Width(); };
        hooks.attach=[&](o::Sid object,std::int32_t x,std::int32_t y) { s.root=raw.Current();raw.AttachObjectCurrent(object.value,x,y); };
        s.opener=world.Session().MakeObjectMenuOpen(std::move(hooks));const auto accepted=s.Dispatch(sid,event);return accepted ? s.root : 0U;
    });
}
// 같은 fancy/설명 작성기와 현재 Session을 사용하며 원본처럼 게임 시계를 멈추지 않는다.
std::uint32_t CampaignObjectMenuClient::OpenProduction(GameWorld& world,const InventoryItemNode& item,ScreenPoint point,const std::function<bool(o::Sid)>& affordable) {
    Clear();auto& s=*impl_;s.world=&world;std::uint32_t accepted{};
    s.buttons.Menus()->OpenAttached([&](const MenuGumpWidgets& widgets,const MenuGumpControls& controls,const MenuGump& raw) {
        s.chain=world.Session().MakeObjectMenus(s.Ui(widgets,controls,raw));InventoryAboutState state;InventoryAboutHooks h;
        h.begin=[&] { s.root=widgets.BeginFancy();return s.root; };h.root=[&s] { return s.buttons.RegisteredGump(0); };h.templateOwner=[&] { return world.Session().TemplateOwner(); };h.editor=[&] { return world.Session().Editor()!=0; };
        h.cost=[&](auto kind) { return world.Session().HitPoints().TypeCostUnits(kind); };h.affordable=affordable;
        h.helpWord=[&s]()->std::optional<std::string> { std::string word;if (!s.client.Configuration().ReadString("helpWord",word)) return std::nullopt;const auto bytes=o::EncodeOriginalText(word);return std::string(bytes.begin(),bytes.end()); };
        h.idle=[&](auto token,bool enabled) { widgets.SetIdle(token,enabled); };h.reference=[&](auto token,auto sid) { widgets.Reference(token,sid); };
        h.describe=[&s](auto kind,auto count,auto owner,auto sid) { s.chain->Describe(kind,count,owner,sid); };h.translate=[&s](auto key) { return s.Translate(key); };
        h.print=[](auto pattern,std::int32_t value) { const std::array<PriestMenuArgument,1> arguments{value};return PrintMenu(pattern,arguments); };h.header=[&](auto text) { controls.AddHeader(text); };
        h.entry=[&](std::optional<std::string_view> text,auto command,auto parameter,bool include,bool enabled,bool force) { if (!text) throw std::logic_error("생산 정보 메뉴 helpWord NULL의 실제 CRT 문자열 소비 미복원");controls.AddText(*text,command,parameter,include,enabled,force); };
        h.attach=[&](auto p,auto parent) { raw.AttachCurrent(p.x,p.y,parent); };h.sound=[&s](auto name) { s.client.WorldAudio().Sounds().PlayByName(name,0,0,0,0,0); };
        InventoryAboutMenu menu(world.Session().Core().pool,state,std::move(h));accepted=menu.Open(item,point);return s.root;
    });return accepted;
}
// 팝업의 실제 Form이 먼저 반납된 뒤 빌린 세션/서식 callback을 해제한다.
void CampaignObjectMenuClient::Clear() {
    auto& s=*impl_;if (s.world) s.buttons.Menus()->Close();s.root=0;s.opener.reset();s.chain.reset();s.world=nullptr;
}
// 원본 등록/지연 삭제 상태를 읽으며 별도 활성 bool을 만들지 않는다.
bool CampaignObjectMenuClient::Active() const {
    const auto& s=*impl_;const auto* node=s.root ? s.buttons.Menus()->Node(s.root) : nullptr;
    return node && (node->flags&1) && !(node->flags&0x10000);
}
// 메뉴가이미닫히도록예약했어도같은세계의현재Factory입력/건설/회수요청을실행한다.
bool CampaignObjectMenuClient::Command(const InputEvent& event) {
    const auto command=event.code&0x03ffffffU;
    const bool common=command==0x030000afU || command==0x030000e2U || command==0x03000015U; // 공통 메뉴가 실제로 작성하는 소유자/피해/삭제 명령이다.
    if (!common && command!=0x02000060U && command!=0x03000080U && command!=0x03000032U && command!=0x03000063U && command!=0x0200005dU && command!=0x0300011aU && command!=0x03000076U) return false;
    auto& s=*impl_;if (!s.world || !s.chain) throw std::logic_error("실제 사제 건설 메뉴의 현재 월드 수명 없음");
    const auto parameter=std::bit_cast<std::uint32_t>(event.y);
    if (common) return s.world->Session().CommandObject(o::Sid{static_cast<std::uint16_t>(parameter)},event.code)!=0;
    // 원본42aa50은 이벤트의y를 내용물검색부모로 읽는다. 현재 메뉴의 부모도 같은 SID다.
    if (command==0x0300011aU || command==0x03000076U) { const o::Sid parent{static_cast<std::uint16_t>(parameter)};static_cast<void>(s.world->Session().CommandCarrierDrop(parent,event.code,parent));return true; }
    if (command==0x0200005dU) { s.world->Session().RequestSalvage(parameter);return true; }
    if (command==0x03000080U || command==0x03000032U || command==0x03000063U) { static_cast<void>(s.world->Session().CommandFactory(o::Sid{static_cast<std::uint16_t>(parameter)},event.code,parameter));return true; }
    static_cast<void>(s.chain->SelectBuild(parameter>>16,o::Sid{static_cast<std::uint16_t>(parameter)}));return true;
}
}
