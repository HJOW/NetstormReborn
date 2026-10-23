// NSInfo와항상생성되는PlayerInfo/BattleInfo/특수버튼의실제부모생성자를복원한다.
#pragma once
#include "client/HudNsInfo.h"
#include "client/ButtonGump.h"

namespace netstorm::client {
// PlayerInfo원문0xbc의미작성필드도같은저장소에서보존한다.
struct HudPlayerInfoNode : GumpCreateNode {
    std::uint32_t text{},master{},button{},start{}; // +a0/+a4/+a8/+ac의실제네자식이다.
    std::uint32_t player{},buttonType{},reserved{}; // +b0/+b4/+b8의현재메타플레이어/TYPE/상태다.
};
// BattleInfo원문0x16c의선택값과22개의현재설정쌍이다.
struct HudBattleInfoNode : GumpCreateNode {
    std::uint32_t title{},summary{},body{},battle{},changed{}; // +a0/+a4/+a8/+ac/+b0이다.
    std::array<std::array<std::uint32_t,2>,22> settings{}; // +b4..+163의각값/활성쌍이다.
    std::array<std::uint8_t,4> bytes{}; // +164..167에서생성자는첫BYTE만0으로쓴다.
    std::uint32_t mode{}; // +168의초기값은2다.
};
// 004256d0은기본버튼을생성한뒤VT와+c4만바꾸는실제파생버튼이다.
struct HudPlayerButtonNode : ButtonGumpNode {
    std::uint32_t untouchedC0{},entry{}; // +c0는보존하고+c4만0으로쓴다.
};
// 실제하위가메타맵관문을통과할때호출할후속배치종류다.
enum class HudNsInfoChild { Player,Battle };
// 모든토큰/노드는같은실제Gump등록표에속한다.하위생성훅은실제기존생성자를호출한다.
struct HudNsInfoCreateHooks {
    std::function<HudNsInfoNode*(std::uint32_t)> node; // NSInfo고정this의현재파생저장소다.
    std::function<HudPlayerInfoNode*(std::uint32_t)> player; // 각PlayerInfo의고정주소다.
    std::function<HudBattleInfoNode*(std::uint32_t)> battle; // BattleInfo고정주소다.
    std::function<HudPlayerButtonNode*(std::uint32_t)> button; // 실제특수버튼저장소다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 같은등록표의현재기본노드다.
    std::function<std::uint32_t&(std::uint32_t)> textFlags,buttonFlags; // 현재Scrolling+d0/현재Button+ac를직접빌린다.
    std::function<std::uint32_t(std::uint32_t)> read; // 원문종류BYTE/배경TYPE·frame/zone/색/모드를현재조회한다.
    std::function<std::uint32_t(std::size_t)> allocate; // 원문new의요청폭과실제파생저장소토큰이다.
    std::function<void(std::uint32_t,std::uint32_t,float,float,std::uint32_t)> baseConstruct; // 실제GumpCreate::Construct다.
    std::function<std::uint32_t(std::uint32_t,float,float,std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> background; // 실제GumpBackgroundCreate::Construct다.
    std::function<std::uint32_t(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t)> scroll,text; // 실제Scrolling/Text생성자의4인자다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t)> buttonBase; // 실제ButtonGump::ConstructBase다.
    std::function<std::uint32_t(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t,std::uint32_t)> stone; // 실제Stone생성자의문자열/색/문맥토큰을유지한다.
    std::function<void(std::uint32_t,std::uint32_t)> scrollable,slider,wrap,parseScroll,parseText; // 실제scroll설정/본문파싱/초기Refresh를호출한다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> fitText; // 실제004bb310의두DWORD인자다.
    std::function<void(std::uint32_t)> fitStone; // 실제00424f90의문자측정/크기갱신이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach,baseAttach; // 현재가상Attach와공통00466750을구별한다.
    std::function<std::uint32_t(std::uint32_t)> translate,type; // 실제번역/TYPE조회이며반환포인터토큰을복사문자열로대체하지않는다.
    std::function<std::uint32_t()> serverName; // 인자없는0047c430의현재문자열토큰이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::int32_t)> formatZone; // 캡처한format/server/zone으로실제문자열을만든다.
    std::function<void(std::uint32_t)> assertion; // Metadisplay.cpp line1912의실제진단이며복귀하면계속한다.
    std::function<void(HudNsInfoChild,std::uint32_t)> metamapLayout; // 일반관문이후전체메타맵배치다.미연결진입을거부한다.
};
// 생성은항상전체자식을만들며일반/메타모드관문은각가상Attach에만있다.
class HudNsInfoCreate {
public:
    // current는00593150의장기소유DWORD이고훅/파생저장소도콜백보다오래살아야한다.
    HudNsInfoCreate(std::uint32_t& current,HudNsInfoCreateHooks hooks);
    // 0047ca50: new0xd8→전체Construct→current저장→반환된노드visible끄기다.
    std::uint32_t CreateHidden() const;
    // 각각0047c4b0/00479d40/0047afa0/004256d0의부모본체다.
    std::uint32_t Construct(std::uint32_t token) const;
    std::uint32_t ConstructPlayer(std::uint32_t token) const;
    std::uint32_t ConstructBattle(std::uint32_t token) const;
    std::uint32_t ConstructButton(std::uint32_t token,std::int32_t width,std::int32_t height) const;
    // 기본Attach후현재meta/playerKnown관문을검사한다.메타전체본문은후속효과다.
    void AttachPlayer(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
    void AttachBattle(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const;
private:
    GumpCreateNode& Base(std::uint32_t token) const; // null/다른저장소를실제접근위치에서거부한다.
    std::uint32_t NewBackground(std::uint32_t flags) const; // 현재frame→TYPE를읽고실제배경을생성한다.
    std::uint32_t NewText(bool scrolling) const; // 같은4개의0인자로실제Text또는Scrolling을생성한다.
    void Attach(std::uint32_t token,std::uint32_t parent) const; // 현재자식의가상Attach(0,0,parent,0)다.
    void AttachChild(HudNsInfoChild kind,std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 두일반관문몸체다.
    std::uint32_t& current_; // 원본현재NSInfo전역의같은수명이다.
    HudNsInfoCreateHooks hooks_; // 실제하위처리기/현재문자열/TYPE를빌린다.
};
}
