// 부모생성자의현재포인터재조회/형제생성순서/초기파싱·크기갱신을보존한다.
#include "client/HudNsInfoCreate.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kNsVtable=0x0050d074,kPlayerVtable=0x0050cd64,kBattleVtable=0x0050ce00,kButtonVtable=0x005038e8; // 보호PE의실제종류표다.
constexpr std::uint32_t kMasterText=0x0050cd3c,kStartText=0x0050cd2c; // 번역입력의원문문자열토큰이다.
// 선택한원문효과를빈성공으로대체하지않는다.
template<class T> const T& NsNeed(const T& hook) { if (!hook) throw std::logic_error("NSInfo의실제생성/자식효과미연결");return hook; }
// 안정된같은실제파생주소를원문접근위치에서요구한다.
template<class T> T& NsNode(const std::function<T*(std::uint32_t)>& hook,std::uint32_t token) {
    auto* node=token ? NsNeed(hook)(token) : nullptr;if (!node) throw std::runtime_error("NSInfo의실제파생저장소없음");return *node;
}
// 종류전역은signedBYTE를DWORD로부호확장한다.
std::uint32_t NsKind(std::uint32_t value) { return std::bit_cast<std::uint32_t>(std::int32_t(std::bit_cast<std::int8_t>(std::uint8_t(value)))); }
}
// 실제현재전역과순수조회만보관하고선택하지않은하위효과는요구하지않는다.
HudNsInfoCreate::HudNsInfoCreate(std::uint32_t& current,HudNsInfoCreateHooks hooks):current_(current),hooks_(std::move(hooks)) {}
// 부모/child기본필드는같은파생저장소의일부다.
GumpCreateNode& HudNsInfoCreate::Base(std::uint32_t token) const { return NsNode(hooks_.base,token); }
// 생성중callback이배경전역을바꾼경우에도각원문new뒤에다시읽는다.
std::uint32_t HudNsInfoCreate::NewBackground(std::uint32_t flags) const {
    const auto token=NsNeed(hooks_.allocate)(0xb8);if (!token) return 0;
    const auto frame=NsNeed(hooks_.read)(0x005ca924),type=hooks_.read(0x005ca920);
    return NsNeed(hooks_.background)(token,1,1,flags,1,type,frame);
}
// 네생성인자는항상0이고할당실패시하위생성자는실행하지않는다.
std::uint32_t HudNsInfoCreate::NewText(bool scrolling) const {
    const auto token=NsNeed(hooks_.allocate)(scrolling ? 0xd8 : 0xa4);if (!token) return 0;
    return scrolling ? NsNeed(hooks_.scroll)(token,0,0,0,0) : NsNeed(hooks_.text)(token,0,0,0,0);
}
// 가상분배기의동일자식토큰과현재부모를그대로전달한다.
void HudNsInfoCreate::Attach(std::uint32_t token,std::uint32_t parent) const { Base(token);NsNeed(hooks_.attach)(token,0,0,parent,0); }
// wrapper는숨김앞에서도전체자식/번역/초기갱신을생략하지않는다.
std::uint32_t HudNsInfoCreate::CreateHidden() const {
    const auto token=NsNeed(hooks_.allocate)(0xd8);const auto result=token ? Construct(token) : 0U;current_=result;
    if (result) Base(result).flags&=~1U;return result;
}
// NSInfo의배경직계자식1개와그배경아래필수13개자식을항상실제로생성한다.
std::uint32_t HudNsInfoCreate::Construct(std::uint32_t token) const {
    auto& node=NsNode(hooks_.node,token);NsNeed(hooks_.baseConstruct)(token,0,0,0,1);node.vtable=kNsVtable;node.command=NsKind(NsNeed(hooks_.read)(0x00540421));
    node.background=NewBackground(0x02000000);Attach(node.background,token);
    node.roster=NewText(true);Base(node.roster).flags&=~1U;NsNeed(hooks_.textFlags)(node.roster)|=8U;
    NsNeed(hooks_.scrollable)(node.roster,1);NsNeed(hooks_.slider)(node.roster,1);Attach(node.roster,node.background);
    node.title=NewText(true);const auto format=NsNeed(hooks_.translate)(NsNeed(hooks_.read)(0x00540bac));
    const auto zone=std::bit_cast<std::int32_t>(hooks_.read(0x005467b8));const auto server=NsNeed(hooks_.serverName)();
    const auto text=NsNeed(hooks_.formatZone)(format,server,zone);NsNeed(hooks_.parseScroll)(node.title,text);Base(node.title).flags&=~1U;Attach(node.title,node.background);
    node.upperDivider=NewBackground(0x02a00000);Base(node.upperDivider).flags&=~1U;Attach(node.upperDivider,node.background);
    node.lowerDivider=NewBackground(0x02a00000);Base(node.lowerDivider).flags&=~1U;Attach(node.lowerDivider,node.background);
    const auto battle=NsNeed(hooks_.allocate)(0x16c);node.battle=battle ? ConstructBattle(battle) : 0U;Base(node.battle).flags&=~1U;Attach(node.battle,node.background);
    for (auto& player:node.players) { // 원문배열의8개주소는고정하지만각부착의배경은현재값을다시읽는다.
        const auto next=NsNeed(hooks_.allocate)(0xbc);player=next ? ConstructPlayer(next) : 0U;Attach(player,node.background);
    }
    node.flags|=0x800U;return token;
}
// PlayerInfo의스크롤/실제파생버튼/마스터문자/Start돌버튼을항상생성한다.
std::uint32_t HudNsInfoCreate::ConstructPlayer(std::uint32_t token) const {
    auto& node=NsNode(hooks_.player,token);NsNeed(hooks_.baseConstruct)(token,0,0,0,1);node.vtable=kPlayerVtable;node.reserved=0;
    node.command=NsKind(NsNeed(hooks_.read)(0x00540440));node.player=0;
    node.text=NewText(true);NsNeed(hooks_.scrollable)(node.text,0);
    node.buttonType=NsNeed(hooks_.type)(hooks_.read(0x005411c8));if (!node.buttonType) NsNeed(hooks_.assertion)(1912);
    const auto button=NsNeed(hooks_.allocate)(0xc8);node.button=button ? ConstructButton(button,16,15) : 0U;NsNeed(hooks_.buttonFlags)(node.button)|=6U;
    node.master=NewText(false);const auto master=NsNeed(hooks_.translate)(kMasterText);NsNeed(hooks_.parseText)(node.master,master);NsNeed(hooks_.fitText)(node.master,0,0);
    const auto stone=NsNeed(hooks_.allocate)(0x170);
    if (stone) { const auto label=hooks_.translate(kStartText);const auto color=hooks_.read(0x005bec3c);node.start=NsNeed(hooks_.stone)(stone,1,1,label,color,0); }else node.start=0;
    NsNeed(hooks_.fitStone)(node.start);Attach(node.text,token);Attach(node.button,token);Attach(node.start,token);Attach(node.master,token);return token;
}
// BattleInfo의22개설정쌍과실제텍스트자식세개를초기화한다.
std::uint32_t HudNsInfoCreate::ConstructBattle(std::uint32_t token) const {
    auto& node=NsNode(hooks_.battle,token);NsNeed(hooks_.baseConstruct)(token,0,0,0,1);node.vtable=kBattleVtable;
    for (auto& setting:node.settings) { setting[1]=1;setting[0]=0; } // 원문은각쌍의뒤값1→앞값0순서로쓴다.
    node.mode=2;node.command=NsKind(NsNeed(hooks_.read)(0x00540457));node.bytes[0]=0;
    node.title=NewText(true);NsNeed(hooks_.scrollable)(node.title,0);NsNeed(hooks_.wrap)(node.title,0);Attach(node.title,token);
    node.summary=NewText(false);NsNeed(hooks_.fitText)(node.summary,0,0);Attach(node.summary,token);
    node.body=NewText(true);NsNeed(hooks_.scrollable)(node.body,0);NsNeed(hooks_.wrap)(node.body,0);Attach(node.body,token);
    node.flags|=0x800U;node.battle=0;node.changed=0;return token;
}
// 특수버튼은원문기본Button생성자를그대로사용하고+c0등미작성필드는보존한다.
std::uint32_t HudNsInfoCreate::ConstructButton(std::uint32_t token,std::int32_t width,std::int32_t height) const {
    auto& node=NsNode(hooks_.button,token);NsNeed(hooks_.buttonBase)(token,width,height,0);node.vtable=kButtonVtable;node.entry=0;return token;
}
// 00479fb0/0047b480의일반부착은공통Attach뒤에만모드를읽는다.
void HudNsInfoCreate::AttachChild(HudNsInfoChild kind,std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const {
    NsNeed(hooks_.baseAttach)(token,x,y,parent,mode);if (!NsNeed(hooks_.read)(0x00594fc8) || !hooks_.read(0x00595344)) return;
    NsNeed(hooks_.metamapLayout)(kind,token);
}
// PlayerInfo의메타맵전체후속은원문일반관문과별도경계로유지한다.
void HudNsInfoCreate::AttachPlayer(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { AttachChild(HudNsInfoChild::Player,token,x,y,parent,mode); }
// BattleInfo도현재메타모드/현재playerKnown두값을분리하여조회한다.
void HudNsInfoCreate::AttachBattle(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const { AttachChild(HudNsInfoChild::Battle,token,x,y,parent,mode); }
}
