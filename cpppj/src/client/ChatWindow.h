// 원본 ChatWindow의 네 실제 자식 생성·공장·입력과 helper 메뉴를 공통 Gump/텍스트 계층에 연결한다.
#pragma once
#include "client/HudChatGeometry.h"
#include "client/TextEntryGump.h"
#include "client/TextGump.h"
#include "o/RawPlayerIdentity.h"
#include <vector>

namespace netstorm::client {
//00548740/44와0054da91 및 세 자동완성 벡터는 Client보다 짧게 살면 안 된다.
struct ChatWindowState {
    std::uint32_t current{},registered{};std::uint8_t completionReady{}; // 현재 root/clear·quit 등록 여부/문자열 목록 초기화 여부다.
    std::vector<std::string> colors,backgrounds,icons; //0054de88/0054dea8/0054de98의 현재 문자열 목록이다.
};
// 실제 자식/버퍼를 조회하며 큰 공통 생성·메뉴·Chatmanager 장치만 외부에 둔다.
struct ChatWindowHooks {
    std::function<HudChatNode*(std::uint32_t)> chat; // 같은0x13c 파생 root다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // root와 네 자식의 같은 기본 주소다.
    std::function<ConsoleGumpNode*(std::uint32_t)> console; // 같은 실제0x128 입력 자식이다.
    std::function<ScrollingTextGumpNode*(std::uint32_t)> text; // 같은 실제0xd8 로그 자식이다.
    std::function<TextEntryBuffer*(std::uint32_t)> buffer; // Console+108의 실제0x28 버퍼다.
    const TextEntryGump* entry{}; // 동일 노드/문자 버퍼에 설치된 실제 편집 본체다.
    const HudChatGeometry* geometry{}; // 동결된 실제 Attach/표시 전환 본체다.
    std::function<std::uint32_t(std::uint32_t)> global; // 현재 사건/모드/색/종류/포인터 전역을 각 사용 지점에서 읽는다.
    std::function<std::uint32_t(std::size_t)> allocate; // 요청0x13c/170/b8/128/d8에 맞는 실제 소유 저장소다.
    std::function<void(std::uint32_t,float,float)> constructBase; //4642c0(this,0,width,height,1)이다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> constructHelper; //4254b0(this,0,0,label,unusedBrown,null)이다.
    std::function<void(std::uint32_t)> fitHelper; //424f90의 현재 실제 버튼 글자 크기 맞춤이다.
    std::function<void(std::uint32_t,std::string_view)> title,help; //464ed0/464f80의 소유 문자열 설치다.
    std::function<std::string(std::string_view)> translate; //4de9a0의 현재 실제 번역이다.
    std::function<std::array<std::int32_t,2>(std::uint32_t,std::string_view)> measure; //465020의 현재 root 문맥 측정이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t)> constructBackground; //4655d0의flags1/type0/frame0 배경이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t)> constructScrolling; //4bc060의 실제 생성·자기 버퍼·Slider다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> replaceBuffer; //4bc3b0의 이전 소유 버퍼 반납→현재 공유 버퍼 빌림이다.
    std::function<std::uint32_t()> sharedBuffer; //00548758 객체와 같은 Client 수명의 실제 StyleTextBuffer 토큰이다. maxLines 값과 구별한다.
    std::function<void(std::uint32_t,std::uint32_t)> setSlider,setScrollable; //4ba1b0/4ba230의 실제 현재 재부착/스크롤 비트다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> attach; // 같은 현재 대상의 가상+20이다.
    std::function<void(std::uint32_t)> push,destructBase; // 같은 현재 가상+18/공통464420 수명이다.
    std::function<void(std::string_view,std::uint32_t,std::uint32_t)> registerCommand; //43f660의 이름/callback/flag1 등록이다.
    std::function<void(std::uint32_t,std::uint32_t)> bufferAppendCallback; // 실제 공유 StyleTextBuffer+34에42f590 callback을 설치한다.
    std::function<std::int32_t(std::uint32_t)> frameCount; // 현재 TYPE+114의 물리 프레임 수다.
    std::function<std::int16_t(std::uint32_t,std::uint32_t)> frameHeight; //419850의 실제 프레임 헤더+1a다.
    std::function<void(std::uint32_t,std::int32_t)> requestScroll,relativeScroll; //4ba360/4ba380의 실제 요청이다.
    std::function<void(std::uint32_t)> requestBottom; //4ba3b0의 실제 맨 아래 요청이다.
    std::function<std::uint32_t(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> baseHandle; //464990의 같은 Help 기본 입력이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> bubble; //464e90의 현재 부모 사건이다.
    std::function<bool(std::string_view)> recipient; //42dc70의 실제 다중 수신 대상 유효성 검사다.
    std::function<void(std::uint32_t,std::string_view)> sendChat; //42da00의 현재 Chatmanager와 실제 입력 바이트다.
    std::function<bool(o::PlayerIdentityRecord&)> resolve; // 기존 RawPlayerIdentity::Resolve에 같은 현재 기록을 제공한다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t)> log; //4c2630의 실제 이름 조회 실패 진단이다.
    std::function<std::int32_t(std::string_view)> config; //441090의 현재 정수 설정이다.
    std::function<void()> menuBegin,menuFinalize; //77bb0(0)/475c40의 현재 controlled 메뉴 시작/정리다.
    std::function<void(std::string_view)> menuTitle; //77ee0의 실제 제목이다.
    std::function<void(std::string_view,std::uint32_t,std::optional<std::string_view>)> menuItem; //77cb0(label,event,payload,1,1,0)이며 null과 문자열을 구별한다.
    std::function<void(std::uint32_t,std::string_view,std::uint32_t,std::string_view)> menuChecked; //77dd0(check,label,event,payload,1,1)이다.
    std::function<std::uint32_t()> menuTake; //475c00(1,0)의 현재 메뉴 pop이다.
    std::function<void(std::string_view,std::uint32_t)> menuSubmenu; //78160(label,child,1,0)이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t)> menuOpen; //475ca0의 현재 x/y/참조 객체다.
    std::function<void(std::uint32_t)> assertion; // 원본 진단 줄 번호이며 돌아오면 현재 값으로 계속한다.
};
// 실제 네 자식을 조립하며 helper·입력 처리와 동결된 Attach를 같은 저장소에서 실행한다.
class ChatWindow {
public:
    ChatWindow(ChatWindowState& state,ChatWindowHooks hooks); // 현재 root/기본 조회만 미리 검증한다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t width,std::int32_t height) const; //431810의 실제 생성이다.
    std::uint32_t Create() const; //431ee0의 명령 등록·200×100 root·공유 버퍼 callback이다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; // 동결된42e6e0 본체다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second,std::optional<std::string_view> text={}) const; //42fab0의 동일 사건이며 문자열 pointer는 별도 현재 뷰로 전달한다.
    void NormalizePrefix(std::uint32_t token) const; //42ebc0의 두 번 입력 조회/접두 재작성이다.
    void ChangeRecipient(std::uint32_t token,std::optional<std::string_view> name,std::uint32_t operation) const; //42ea60의0추가/1제거다.
    void Destruct(std::uint32_t token) const; //42e8d0의 현재 root0/공통 자식 우선 수명이다.
private:
    HudChatNode& Node(std::uint32_t token) const; // 실제 root와 기본 주소가 같아야 한다.
    GumpCreateNode& Base(std::uint32_t token) const; // 현재 도달한 자식만 조회한다.
    ConsoleGumpNode& Console(std::uint32_t token) const; // 현재 root+a0의 같은 Console이다.
    ScrollingTextGumpNode& Text(std::uint32_t token) const; // 현재 root+a4의 같은 ScrollingText다.
    TextEntryBuffer& Buffer(std::uint32_t token) const; // 현재 Console+108의 같은 편집 버퍼다.
    const TextEntryGump& Entry() const; // 같은 실제 편집 모듈을 요구한다.
    std::uint32_t Global(std::uint32_t address) const; // 현재 원문 전역을 매번 조회한다.
    std::string CopyInput(std::uint32_t token) const; // 실제 CopyText(128) 결과를 NUL까지 읽는다.
    void Prefix(std::uint32_t token,std::string_view text) const; // 현재128BYTE 원문 접두에만 쓴다.
    void Completions() const; // 두256색 목록과 실제8..16높이 아이콘 목록이다.
    void HelperMenu(std::uint32_t token) const; // 원문 helper의 전체 controlled 메뉴/그룹 조립이다.
    ChatWindowState& state_;ChatWindowHooks hooks_; // Client 수명의 실제 현재 전역과 자식/장치 경계다.
};
}
