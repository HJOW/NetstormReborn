// 누락된 FPU/레지스터 지역값은 같은 호스트의 전체 PE 기계어로 보완한다.
#include "client/HudNsInfoMeta.h"
#include "client/RawCameraView.h"
#include "o/RawBattleOptionText.h" //실제선택문자열의번역플래그와포함최댓값을공유한다.
#include "o/RawBattleOptionSlots.h" //표시/입력의같은24DWORD 랭크변환과실제슬롯필드를공유한다.
#include <algorithm>
#include <bit>
#include <cstdio>
#include <stdexcept>

namespace netstorm::client { // 실제 CS/Gump의부분효과를실패뒤되돌리지않는다.
namespace { // 원문비정렬필드·문자열/float저장과현재UI하위호출을구별한다.
template<class F> const F& Need(const F& value) { if (!value) throw std::logic_error("NSInfo 메타의 실제 선택 장치 미연결");return value; } //미선택장치를빈성공으로요구하지않는다.
// 지정한실제폭만읽고원문필드밖물리는그접근위치에서거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("NSInfo 메타의 실제 CS 필드 부족");std::uint32_t value{};
    // 낮은BYTE부터원문폭만조립한다.
    for (std::size_t byte=0;byte<width;++byte) value|=std::uint32_t(bytes[offset+byte])<<(8*byte);return value;
}
// 현재레코드의실제첫NUL을찾으며임의길이로이름을잘라성공시키지않는다.
std::string_view Name(std::span<const std::uint8_t> record) {
    const auto text=record.subspan(0x26);const auto end=std::find(text.begin(),text.end(),std::uint8_t{});if (end==text.end()) throw std::out_of_range("NSInfo 이름의 실제 NUL 없음");return {reinterpret_cast<const char*>(text.data()),static_cast<std::size_t>(end-text.begin())};
}
// 원문현재자식의기본폭을쓴뒤같은배율의float결과만저장한다.
void Width(std::uint32_t token,double width,const HudNsInfoMetaHooks& h) { auto& node=Need(h.node)(token);node.baseWidth=static_cast<float>(width);node.width=static_cast<float>(width*node.scale); }
void Height(std::uint32_t token,double height,const HudNsInfoMetaHooks& h) { auto& node=Need(h.node)(token);node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(height*node.scale); } //FST32뒤에도남은원문계산값으로배율을곱하며저장높이를재조회하지않는다.
void Visible(std::uint32_t token,bool visible,const HudNsInfoMetaHooks& h) { auto& node=Need(h.node)(token);if (visible) node.flags|=1U;else node.flags&=~1U; } //다른Gump플래그는보존한다.
// 바뀐대상또는0대상에서만+b8을지우며고정PlayerInfo의현재+b0을그대로쓴다.
void Select(std::uint32_t token,std::uint32_t participant,const HudNsInfoMetaHooks& h) { auto& node=Need(h.player)(token);if (node.player!=participant || !participant) node.reserved=0;node.player=participant; }
// 실제현재도움말제목/키를각각빈문자열교체뒤에쓴다.
void Help(std::uint32_t token,bool battle,const HudNsInfoMetaHooks& h) {
    Need(h.helpText)(token,"");const auto title=Need(h.translate)(battle ? "Battle Status Window":"Player List Window");h.helpText(token,title);Need(h.helpWord)(token,"");h.helpWord(token,battle ? "battleStatusView":"playerListView");
}
// 원문256BYTE서식은현재숫자의signed해석을그대로쓰고물리범위밖출력만거부한다.
template<class... Args> std::string Format(const char* format,Args... arguments) {
    std::array<char,256> bytes;const auto size=std::snprintf(bytes.data(),bytes.size(),format,arguments...);if (size<0 || static_cast<std::size_t>(size)>=bytes.size()) throw std::out_of_range("PlayerInfo 실제256BYTE 서식 범위");return {bytes.data(),static_cast<std::size_t>(size)};
}
// +40에계산값을저장해도배율곱에는원문FST뒤의잔류계산값을사용한다.
void PanelHeight(std::uint32_t token,double height,const HudNsInfoMetaHooks& h) { auto& node=Need(h.node)(token);node.baseHeight=static_cast<float>(height);node.height=static_cast<float>(height*node.scale); }
// 0047b430은원문범위에맞는고정그림문자열을반환하며현재언어번역을추가하지않는다.
const char* Category(std::int32_t option) { if (option>0 && option<5) return "~[I93.110]";if (option>4 && option<11) return "~[I93.117]";if (option>10 && option<14) return "~[I93.116]";if (option>=14 && option<=21) return "~[I93.115]";return "~[I93.112]"; }
// 파생+b4..163의실제현재DWORD를주소순서가아닌원문쓰기순서로지정한다.
void Setting(HudBattleInfoNode& node,std::size_t offset,std::uint32_t value) { const auto relative=offset-0xb4;node.settings.at(relative/8).at(relative%8/4)=value; }
}
// 원문은정렬되지않은유효표를표준sort가아닌전체39×39쌍교환으로재배치한다.
void RefreshNsInfoRoster(std::uint32_t token,HudNsInfoMetaGlobals g,const HudNsInfoMetaHooks& h) {
    if (!g.metamap || !g.known) return;const auto shade=Need(h.read)(0x5c78d0),yellow=h.read(0x59afd8);const std::array<std::uint32_t,8> ownContext{2,0,yellow,0,0,shade,1,1};
    auto& root=Need(h.root)(token);Need(h.replace)(root.roster,"");Need(h.wrap)(root.roster,0);
    // 0번을보존하며005930b4..314f의39DWORD만초기순서로쓴다.
    for (std::uint32_t index=1;index<40;++index) g.info.rosterOrder[index]=index;
    // 바깥현재후보는교환직후새참가자로재조회한다.
    for (std::uint32_t outer=1;outer<40;++outer) {
        auto current=g.challenge.Participant(g.info.rosterOrder[outer]);if (!(Read(current,0x64,1)&1U)) continue;
        // 유효한상대의레벨BYTE를먼저비교하고동률때만상대→현재순서로기술합계를읽는다.
        for (std::uint32_t inner=1;inner<40;++inner) {
            const auto other=g.challenge.Participant(g.info.rosterOrder[inner]);if (!(Read(other,0x64,1)&1U)) continue;const auto level=Read(current,0x58,1),otherLevel=Read(other,0x58,1);bool exchange=level<otherLevel;
            if (level==otherLevel) { const auto otherWeight=Need(h.weight)(other.subspan(0x54,10));const auto currentWeight=h.weight(current.subspan(0x54,10));exchange=currentWeight<otherWeight; }
            if (exchange) { std::swap(g.info.rosterOrder[outer],g.info.rosterOrder[inner]);current=g.challenge.Participant(g.info.rosterOrder[outer]); }
        }
    }
    // 유효행만추가하되metadata는실제참가자번호가아닌정렬배열의1..39행번호다.
    for (std::uint32_t row=1;row<40;++row) {
        const auto record=g.challenge.Participant(g.info.rosterOrder[row]);if (!(Read(record,0x64,1)&1U)) continue;const bool own=Need(h.objectOwner)(Read(record,0xb8))==g.owner;const auto rank=Need(h.weight)(record.subspan(0x54,10));
        std::array<char,256> prefix;const auto count=std::snprintf(prefix.data(),prefix.size()," %d.%d ",static_cast<int>(Read(record,0x58,1)+1U),static_cast<int>(rank));if (count<0 || static_cast<std::size_t>(count)>=prefix.size()) throw std::out_of_range("NSInfo 숫자행의 실제256BYTE 범위");
        std::string text(prefix.data(),static_cast<std::size_t>(count));text+=Name(record);text+='\n';if (text.size()>=120) throw std::out_of_range("NSInfo 참가자행의 실제120BYTE 범위");
        const auto minor=Read(record,0xb2,2);std::array<std::uint32_t,8> context{0,0,minor==78 ? 0xc4U:0xdfU,0,0,0,1,1};if (minor>78) context={0,0,0x9dU,0,0,0,1,1};context[0]|=2U;context[5]=Need(h.read)(0x5c78d0);context[6]=1;context[7]=1;
        const auto& selected=own && h.read(0x5c85c8) ? ownContext:context;Need(h.append)(root.roster,text,selected,row);
    }
}
// 일반부착/모드관문은HudNsInfo가실행했다.여기서는그뒤원문큰본문만수행한다.
void LayoutMetaNsInfo(std::uint32_t token,HudNsInfoMetaGlobals g,const HudNsInfoMetaHooks& h) {
    auto& root=Need(h.root)(token);const auto own=g.challenge.Participant(g.owner);std::uint32_t used{};
    if (!Read(own,0xa4)) {
        Need(h.battle)(root.battle).bytes[0]=0;Need(h.push)(root.title);Visible(root.title,true,h);Need(h.replace)(root.title,"");const auto header=Need(h.header)(false);Need(h.parse)(root.title,header);Width(root.title,root.baseWidth,h);Need(h.fitHeight)(root.title);
        Need(h.push)(root.upperDivider);Visible(root.upperDivider,true,h);Width(root.upperDivider,root.baseWidth,h);Need(h.push)(root.roster);Visible(root.roster,true,h);Width(root.roster,root.baseWidth,h);
        Height(root.roster,static_cast<double>(root.baseHeight)-Need(h.node)(root.title).baseHeight-h.node(root.upperDivider).baseHeight,h);RefreshNsInfoRoster(token,g,h);
        Need(h.attach)(root.title,0,0,root.background,0);h.attach(root.upperDivider,0,Need(h.node)(root.title).baseHeight,root.background,0);h.attach(root.roster,0,static_cast<float>(static_cast<double>(h.node(root.title).baseHeight)+h.node(root.upperDivider).baseHeight),root.background,0);
        Need(h.push)(root.lowerDivider);Visible(root.lowerDivider,false,h);Width(root.lowerDivider,root.baseWidth,h);h.attach(root.lowerDivider,0,static_cast<float>(static_cast<double>(h.node(root.background).baseHeight)*0.5F),root.background,0);
        Need(h.push)(root.battle);Visible(root.battle,false,h);auto& battle=Need(h.battle)(root.battle);battle.battle=Read(own,0xa4);battle.changed=0;Width(root.battle,root.baseWidth,h);h.attach(root.battle,0,static_cast<float>(static_cast<double>(h.node(root.background).baseHeight)*0.5F+h.node(root.lowerDivider).baseHeight),root.background,0);Help(token,false,h);
    } else {
        Visible(root.roster,false,h);Need(h.push)(root.title);Need(h.replace)(root.title,"");const auto header=Need(h.header)(true);Need(h.parse)(root.title,header);Visible(root.title,true,h);Width(root.title,root.baseWidth,h);Need(h.fitHeight)(root.title);
        Need(h.push)(root.upperDivider);Visible(root.upperDivider,true,h);Width(root.upperDivider,root.baseWidth,h);Need(h.attach)(root.title,0,0,root.background,0);h.attach(root.upperDivider,0,Need(h.node)(root.title).baseHeight,root.background,0);
        const auto battle=g.challenge.Battle(Read(own,0xa4));auto y=CameraFtolLow(static_cast<double>(Need(h.node)(root.title).baseHeight)+h.node(root.upperDivider).baseHeight);bool master{};
        // 같은전투8슬롯에서첫마스터하나만별도0번패널에놓는다.마스터가없으면used0을유지한다.
        for (std::uint32_t slot=0;slot<8;++slot) {
            const auto participant=Read(battle,0x38+slot*4U);if (!participant || !(Read(g.challenge.Participant(participant),0x64,1)&4U)) continue;
            Need(h.push)(root.players[0]);Width(root.players[0],root.baseWidth,h);Select(root.players[0],participant,h);Visible(root.players[0],true,h);h.attach(root.players[0],3,static_cast<float>(static_cast<double>(h.node(root.title).baseHeight)+h.node(root.upperDivider).baseHeight+3.0F),root.background,0);
            y=CameraFtolLow(static_cast<double>(h.node(root.players[0]).y)+h.node(root.players[0]).baseHeight+2.0F);used=1;master=true;break;
        }
        // 원문슬롯순서로비마스터만추가하고다음Y는저장된현재정수Y+실제후행높이+2를ftol한다.
        for (std::uint32_t slot=0;slot<8;++slot) {
            const auto participant=Read(battle,0x38+slot*4U);if (!participant || (Read(g.challenge.Participant(participant),0x64,1)&4U)) continue;
            if (used>=root.players.size()) throw std::out_of_range("NSInfo PlayerInfo 실제8패널 범위");const auto panel=root.players[used++];Need(h.push)(panel);Width(panel,root.baseWidth,h);Select(panel,participant,h);Visible(panel,true,h);const auto point=static_cast<float>(y);h.attach(panel,3,point,root.background,0);y=CameraFtolLow(static_cast<double>(point)+h.node(panel).baseHeight+2.0F);
        }
        Need(h.push)(root.lowerDivider);Visible(root.lowerDivider,master,h);Width(root.lowerDivider,root.baseWidth,h);h.attach(root.lowerDivider,3,static_cast<float>(static_cast<double>(h.node(root.background).baseHeight)*0.5F),root.background,0);
        Need(h.push)(root.battle);Visible(root.battle,master,h);auto& info=Need(h.battle)(root.battle);info.battle=Read(own,0xa4);info.changed=Read(own,0x64)&4U;Width(root.battle,root.baseWidth,h);Height(root.battle,static_cast<double>(h.node(root.background).baseHeight)*0.5F-h.node(root.lowerDivider).baseHeight,h);h.attach(root.battle,3,static_cast<float>(static_cast<double>(h.node(root.background).baseHeight)*0.5F+h.node(root.lowerDivider).baseHeight),root.background,0);Help(token,true,h);
    }
    // 남은패널도숨긴뒤실제Attach를호출한다.미복원자식본문을빈성공으로우회하지않는다.
    for (std::uint32_t index=used;index<8;++index) { const auto panel=root.players[index];Need(h.push)(panel);Width(panel,root.baseWidth,h);auto& player=Need(h.player)(panel);player.reserved=0;player.player=0;Visible(panel,false,h);Need(h.attach)(panel,0,0,root.background,0); }
}
// 일반Attach/현재meta/known관문은HudNsInfoCreate가먼저수행한다.
void LayoutMetaPlayerInfo(std::uint32_t token,HudNsInfoMetaGlobals g,const HudNsInfoMetaHooks& h) {
    auto& panel=Need(h.player)(token);Need(h.push)(panel.text);h.push(panel.button);h.push(panel.start);h.push(panel.master);
    auto& textFlags=Need(h.textFlags)(panel.text);if (Read(g.challenge.Participant(g.owner),0x64,1)&4U) textFlags|=8U;else textFlags&=~8U;
    const auto white=Need(h.read)(0x5bec2c),yellow=h.read(0x59afd8);static_cast<void>(white);const std::array<std::uint32_t,8> emptyContext{0,0,yellow,0,0,0,1,1};float extraHeight{}; //원문초기흰문맥의색은후행이름색으로덮어쓰지만선행getter조회는유지한다.
    if (!panel.player) {
        Need(h.replace)(panel.text,"");Need(h.append)(panel.text,"Slot empty\n",emptyContext,0);Need(h.enabled)(panel.button,false);Need(h.value)(panel.button,0);h.enabled(panel.start,false);h.value(panel.start,0);
    } else {
        const auto participant=g.challenge.Participant(panel.player);const auto options=g.challenge.Battle(Read(participant,0xa4)).subspan(0x59);const bool ownMaster=(Read(g.challenge.Participant(g.owner),0x64)&4U)!=0;Need(h.textFlags)(panel.text)&=~0x10U;
        Need(h.enabled)(panel.start,false);Visible(panel.start,false,h);Visible(panel.master,false,h);std::uint32_t metadata{};h.enabled(panel.button,true);Visible(panel.button,true,h);const auto targetMaster=Read(participant,0x64)&4U;
        if (targetMaster && !ownMaster) { Visible(panel.master,true,h);extraHeight=Need(h.node)(panel.master).baseHeight; }
        else if (ownMaster) {
            if (targetMaster) {
                Visible(panel.start,true,h);h.enabled(panel.start,true);const auto battle=g.challenge.Battle(Read(participant,0xa4));
                // 실제8슬롯을원문순서로읽고첫미준비참가자에서만시작버튼을끈다.
                for (std::uint32_t slot=0;slot<8;++slot) { const auto other=Read(battle,0x38+slot*4);if (!other) continue;const auto flags=Read(g.challenge.Participant(other),0x64);if (!(flags&2U) || !(flags&0x2000U)) { h.enabled(panel.start,false);Visible(panel.start,false,h);break; } }
                metadata=panel.player;extraHeight=h.node(panel.start).baseHeight;
            } else metadata=panel.player;
            Need(h.textFlags)(panel.text)|=0x10U;
        }
        if (!(Read(participant,0x64,1)&2U)) { Need(h.readyButton)(panel.button).entry=0;Need(h.value)(panel.button,0); }
        else { Need(h.value)(panel.button,1);Need(h.readyButton)(panel.button).entry=(Read(participant,0x64)&0x2000U) ? 1U:0U; }
        if (panel.player!=g.owner) Need(h.enabled)(panel.button,false);
        Need(h.replace)(panel.text,"");const auto slot=Read(participant,0xa8);const auto colorAddress=0x5c78b0U+slot*4U;const auto nameColor=Need(h.read)(colorAddress);const std::array<std::uint32_t,8> iconContext{2,0,h.read(0x5b5e58),1,0,h.read(0x5bec2c),1,1};
        Need(h.readyButton)(panel.button).untouchedC0=(Read(participant,0x64)&0x2000U) ? h.read(0x5accfcU+slot*4U):h.read(0x5bec3c);
        const bool special=Read(options,0x3f+Read(participant,0xa8),1)!=0;auto saved=Need(h.node)(panel.text).textContext;h.node(panel.text).textContext=iconContext;
        Need(h.parse)(panel.text,Format("~[T%d]",std::bit_cast<std::int32_t>(panel.player+100U)));h.parse(panel.text,special ? "~[IfortGump.L10]":"~[IfortGump.L9]");h.parse(panel.text,"~[T0]");
        if (!special) { h.parse(panel.text,Format("~[T%d]",std::bit_cast<std::int32_t>(panel.player+200U)));h.parse(panel.text,Format("~[IfortGump.L%d]",static_cast<int>(Read(options,0x47+Read(participant,0xa8),1)+11U)));h.parse(panel.text,"~[T0]"); }
        h.node(panel.text).textContext=saved;
        // 두상태아이콘은각각현재문맥을새로보관/대입하고정상파싱뒤에만복원한다.
        for (const auto flag:{0x800U,0x200U}) { if (!(Read(participant,0x64)&flag)) continue;saved=h.node(panel.text).textContext;h.node(panel.text).textContext=iconContext;h.parse(panel.text,flag==0x800U ? "~[IfortGump.L1]":"~[IfortGump.L0]");h.node(panel.text).textContext=saved; }
        if (!special) {
            h.parse(panel.text,Format("~[T%d]",std::bit_cast<std::int32_t>(panel.player+300U)));const auto rank=o::BattleSlotRank(options,std::bit_cast<std::int32_t>(Read(participant,0xa8)));const auto pictures=Need(h.rankPicture)(rank);h.parse(panel.text,Format("%s",pictures.c_str()));h.parse(panel.text,Format("~[T0]"));
        }
        std::array<std::uint32_t,8> nameContext{2,0,(Read(participant,0x64)&0x2000U) ? nameColor:h.read(0x5bec3c),0,0,0,1,1};
        if (Read(participant,0x64,1)&4U) Need(h.append)(panel.text,Name(participant),nameContext,metadata);
        else { const std::string name(Name(participant));h.parse(panel.text,Format("~[T%d]~B~E~[C.%d]%s~[T0]",std::bit_cast<std::int32_t>(panel.player+400U),std::bit_cast<std::int32_t>(nameContext[2]),name.c_str())); }
        Need(h.appendDefault)(panel.text,"\n");
    }
    Need(h.attach)(panel.button,0,0,0,0);Need(h.requestTop)(panel.text);Need(h.fitContent)(panel.text);const auto width=static_cast<double>(panel.baseWidth)-Need(h.node)(panel.button).baseWidth-2.0F;Width(panel.text,width,h);
    h.attach(panel.text,static_cast<float>(static_cast<double>(h.node(panel.button).baseWidth)+2.0F),0,0,0);
    h.attach(panel.start,static_cast<float>((static_cast<double>(panel.baseWidth)-h.node(panel.start).baseWidth)*0.5F),static_cast<float>(static_cast<double>(h.node(panel.text).baseHeight)+2.0F),0,0);
    h.attach(panel.master,0,static_cast<float>(static_cast<double>(h.node(panel.text).baseHeight)+2.0F),0,0);PanelHeight(token,static_cast<double>(extraHeight)+h.node(panel.text).baseHeight+2.0F,h);
}
// 일부현재설정쌍을보존하는원문분기를전체초기화로대체하지않는다.
void PrepareMetaBattleSettings(HudBattleInfoNode& node,std::uint32_t gameType) {
    switch (gameType) {
    case 1: Setting(node,0xd4,2);Setting(node,0xe4,1);Setting(node,0x138,0);Setting(node,0x144,1);break;
    case 2: Setting(node,0xd4,3);Setting(node,0xe0,1);Setting(node,0xe4,2);Setting(node,0x100,1);Setting(node,0x128,1);Setting(node,0x138,1);Setting(node,0x140,1);Setting(node,0x144,2);Setting(node,0x148,1);goto extra;
    case 3: Setting(node,0xd4,3);Setting(node,0xe0,0);Setting(node,0xe4,2);Setting(node,0x100,0);Setting(node,0x128,0);Setting(node,0x138,0);Setting(node,0x140,0);Setting(node,0x144,0);Setting(node,0x148,0);Setting(node,0x154,2);Setting(node,0x158,1);Setting(node,0x15c,1);Setting(node,0x160,2);goto lower;
    case 4: Setting(node,0xd4,3);Setting(node,0xe0,1);Setting(node,0xe4,2);Setting(node,0x100,1);Setting(node,0x128,1);Setting(node,0x138,1);Setting(node,0x140,1);Setting(node,0x144,2);Setting(node,0x148,1);Setting(node,0x150,1);Setting(node,0x154,0);Setting(node,0x158,1);Setting(node,0x15c,0);Setting(node,0x160,1);goto common;
    default: Setting(node,0xd4,3);Setting(node,0xe4,2);Setting(node,0x138,1);Setting(node,0x144,2);break;
    }
    Setting(node,0xe0,0);Setting(node,0x100,0);Setting(node,0x128,0);Setting(node,0x140,0);Setting(node,0x148,0);
extra: Setting(node,0x154,0);Setting(node,0x158,0);Setting(node,0x15c,0);Setting(node,0x160,0);
lower: Setting(node,0x150,0);
common: Setting(node,0xbc,2);Setting(node,0xc4,2);Setting(node,0x12c,2);Setting(node,0x134,2);Setting(node,0x14c,2);Setting(node,0xc0,1);Setting(node,0xc8,1);Setting(node,0xcc,0);Setting(node,0xd0,0);Setting(node,0xd8,1);Setting(node,0xdc,0);Setting(node,0xe8,1);Setting(node,0xec,1);Setting(node,0xf0,1);Setting(node,0xf4,1);Setting(node,0xf8,1);Setting(node,0xfc,0);Setting(node,0x104,0);Setting(node,0x108,0);Setting(node,0x10c,1);Setting(node,0x110,1);Setting(node,0x114,0);Setting(node,0x118,1);Setting(node,0x11c,1);Setting(node,0x120,1);Setting(node,0x124,1);Setting(node,0x130,1);Setting(node,0x13c,1);
}
// 일반부착/두현재관문뒤에는비활성전투도공통스크롤/자식배치를수행한다.
void LayoutMetaBattleInfo(std::uint32_t token,HudNsInfoMetaGlobals g,const HudNsInfoMetaHooks& h) {
    auto& panel=Need(h.battle)(token);Need(h.push)(panel.title);h.push(panel.body);h.push(panel.summary);Need(h.replace)(panel.title,"");Need(h.textFlags)(panel.title)&=~8U;h.textFlags(panel.title)&=~0x10U;Need(h.scrollable)(panel.title,1);Need(h.slider)(panel.title,1);h.replace(panel.body,"");h.textFlags(panel.body)&=~8U;h.textFlags(panel.body)&=~0x10U;
    if (panel.battle) {
        const auto battle=g.challenge.Battle(panel.battle);
        if (Read(battle,0,1)&1U) {
            h.textFlags(panel.title)|=8U;const auto options=battle.subspan(0x59);PrepareMetaBattleSettings(panel,Read(options,0,1));Need(h.parse)(panel.title,"~y~E~[T1]Game Type~[T0]\n");const auto type=o::BattleOptionValueText(options,0,h.translate);h.parse(panel.title,Format("~a~[T%u]~w~E%s~[T0]\n",1U,type.c_str()));h.parse(panel.title,Format("~y~E~[T%i]Game Options~[T0]~w~[T%i]  (~sSort By~w)~[T0]\n",23,24));
            // mode2는활성전체값/편집태그를쓰고그외는현재값이기준과다른활성행만쓴다.
            for (std::uint32_t option=1;option<22;++option) {
                if (panel.mode==2) { if (!panel.settings[option][1]) continue;const auto value=o::BattleOptionValueText(options,option,h.translate);const auto name=o::BattleOptionNameText(option,h.translate);h.parse(panel.title,Format("%s~[T%i]~w~E%s: ~[T0]~[T%i]~i%s~[T0]\n",Category(option),static_cast<int>(option+1),name.c_str(),static_cast<int>(option+27),value.c_str())); }
                else { if (Read(options,option,1)==panel.settings[option][0] || !panel.settings[option][1]) continue;const auto value=o::BattleOptionValueText(options,option,h.translate);const auto name=o::BattleOptionNameText(option,h.translate);h.parse(panel.title,Format("%s~[T%i]~w~E%s: ~[T0]~i%s~[T0]\n",Category(option),static_cast<int>(option+1),name.c_str(),value.c_str())); }
            }
            if (panel.mode==1) h.parse(panel.title,Format("~w~E~[T%i]Click To Add~[T0]\n",23));h.parse(panel.title,"~y~E~[T1000]Excluded:~[T0]\n");std::uint32_t excluded{};
            // 원문은count를매회다시읽는다.첫행번호1과TYPE+122태그를사용하고실제배열밖은명시거부한다.
            while (excluded<Read(options,0x16,1)) { const auto typeNumber=Read(options,0x17+excluded,1);const auto name=Need(h.description)(typeNumber);const auto tag=Read(options,0x17+excluded,1)+122U;h.parse(panel.title,Format("~j~E%d) ~[T%u]~N~w%s~[T0]\n",static_cast<int>(excluded+1),tag,name.c_str()));++excluded; }
            if (excluded+122U<162U) { const auto add=Need(h.translate)("Click To Add");h.parse(panel.title,Format("~j~E%d) ~[T1000]~N~w%s~[T0]\n",static_cast<int>(excluded+1),add.c_str())); }
        }
    }
    Width(panel.summary,panel.baseWidth,h);Width(panel.title,panel.baseWidth,h);PanelHeight(panel.title,panel.baseHeight,h);
    if (!panel.bytes[0]) { Need(h.requestTop)(panel.title);h.requestTop(panel.body);panel.bytes[0]=1; }
    Need(h.attach)(panel.title,0,0,token,0);h.attach(panel.summary,static_cast<float>((static_cast<double>(panel.baseWidth)-Need(h.node)(panel.summary).baseWidth)*0.5F),static_cast<float>(static_cast<double>(h.node(panel.title).baseHeight)+2.0F),token,0);h.attach(panel.body,0,static_cast<float>(static_cast<double>(h.node(panel.summary).y)+h.node(panel.summary).baseHeight+2.0F),token,0);
}
}
