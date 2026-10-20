// 전용 Help의 주제 선택·동적 TYPE 정보·본문 스트림을 기존 실제 HTML 파서에 결합한다.
#pragma once
#include "client/HtmlTextParser.h"
#include "client/HtmlLinkText.h"
#include "client/GumpCreate.h"
#include "client/RawPriestMenu.h"
namespace netstorm::client {
// 실제472e60이 만드는 ASCII 마나 그림 프레임 표다. 전용 도움말 외 같은 표식에도 사용할 수 있다.
struct ManaPictureState {
    std::uint32_t type{103}; // 실제005411f4의 마나 그림 TYPE 번호다.
    std::array<std::int32_t,128> frames{}; // signed ASCII 문자→프레임이며 원본 기본0을 보존한다.
    ManaPictureState(); // 원본 wrtsWRTS.?> 순서로 프레임0..10을 설치한다.
};
// 실제4735c0의 점 건너뛰기/앞쪽 강조+11/250바이트 한계다. 비ASCII 원본 전역 접근은 거부한다.
std::string ManaPictureText(std::string_view mana,const ManaPictureState& state,std::int32_t highlighted=-1);
// 원본5240의 파생 닫힘 슬롯이다. 현재 cursor 바로 앞 LF/CR만 textAllowed를0으로 만든다.
void QuickHelpBeforeClose(HtmlTextParserState& state,std::string_view source);
// 실제 TYPE에서 동적 정보가 읽는 현재 필드다. 게임 표의 복사/수명은 실제 연결 공급자가 결정한다.
struct QuickHelpTypeInfo {
    std::int32_t theme{}; // +98 원소 번호이며5이상은 None 문구다.
    std::uint32_t flags1{},flags2{}; // +e8/+ec의 HP/특수 피해 비트다.
    float range{},damage{},cost{}; // +44/+48/+c4이며 damage는 hpPerSec 원본 값이다.
    std::string classification,mana; // +aa 클래스와 후처리한+a0 요구 에너지다.
};
class GameAssets; // 월드 설치 전에도 전역 TYPE/AST 자산은 Client 수명으로 존재한다.
QuickHelpTypeInfo ReadAssetQuickHelpType(const GameAssets& assets,std::uint32_t type); // 메인 메뉴의 Help도 실제 TYPE/AST/요구 에너지 후처리로 정보를 읽는다.
// TYPE/번역/Config와 같은 실제 정보 pane·파일 스트림을 연결한다. 도달한 효과만 필수다.
struct QuickHelpHtmlHooks {
    std::function<const QuickHelpTypeInfo&()> type; // 매 선택 지점에서 같은 현재 TYPE 필드를 다시 읽는다.
    std::function<std::string()> description; // 실제49af80의 현재 타입 설명이다.
    std::function<std::int32_t()> hitPoints; // 실제49fce0이며 현재 사제1/4 모드를 포함한다.
    std::function<std::string(std::string_view)> translate,substitute,config; // 실제 번역·1024바이트 치환·influence 용어 조회다.
    std::function<std::string(std::string_view,std::span<const PriestMenuArgument>)> print; // 번역을 추가하지 않는 실제 고정%s/%d 서식이다.
    std::function<GumpCreateNode*()> pane; // 같은 정보 pane의 현재 flags를 조회한다.
    std::function<void(std::string_view)> paneText,stream; // 실제 정보 pane의 styled 본문과 파일 기록이다.
    std::function<void(std::int32_t)> paneScroll; // 실제4ba360의 절대 위치 요청이다.
    std::function<void()> panePush,paneAttach; // 같은 pane의 가상 Push와419df0 재부착이다.
};
// 004c4c00/4cc0/5240/4c60을 기존 기본 파서/동일 링크 저장소에 조립한다.
class QuickHelpHtml {
public:
    QuickHelpHtml(HtmlTextParserState& state,HtmlTextParserGlobals& globals,std::uint32_t& infoMode,
        std::uint32_t type,std::string_view topic,bool stream,ManaPictureState mana,QuickHelpHtmlHooks hooks,HtmlTextParserHooks base={},HtmlLinkTextState* links=nullptr); // 생성 시 shown0/주제 길이와 링크 소유자를 설치한다.
    std::string Parse(std::string_view source,std::int32_t capacity=8192) const; // 실제 기본 순회에 전용 세 가상 슬롯을 연결한다.
private:
    void Inside(HtmlTextParserState& state,std::string_view source,std::int32_t capacity) const; // 원본 주제 prefix/name과 info 분기다.
    void After(HtmlTextParserState& state,std::string_view source) const; // 현재 > 다음 평문/다음 태그를 한 바이트씩 파일에 기록한다.
    void Info() const; // 동적 정보 HTML→같은 기본 파서→현재 pane/스크롤/부착이다.
    std::string Print(std::string_view pattern,std::initializer_list<PriestMenuArgument> arguments,std::size_t capacity=256) const; // 고정 지역 서식 버퍼 한계를 유지한다.
    HtmlTextParserState& state_; // 현재 shown/cursor/output의 실제 파생 기본 필드다.
    HtmlTextParserGlobals& globals_; // 기본 HTML의 같은 단락 억제 전역이다.
    std::uint32_t& infoMode_; // 실제00565ec4의 현재 다중 줄 해석 모드다.
    std::uint32_t type_; // 생성 시 조회한 TYPE가 있는지 판단하는 원래 번호다.
    std::string topic_; // 원래 주제의 첫NUL까지이며 name 비교 길이도 같은 값이다.
    bool stream_; // 실제+40 파일 스트림의 유무다.
    ManaPictureState mana_; // 같은 원본 마나 타입/프레임 표다.
    QuickHelpHtmlHooks hooks_; // 현재 TYPE/정보 pane/Config/파일 효과다.
    HtmlTextParserHooks base_; // 같은 기본 명령/조건/그림/링크/배경 효과다.
    HtmlLinkTextState* links_; // 중첩 정보 파서가 빌리는 같은 링크 바이트이며 부모 cursor는 별도로 보존한다.
};
}
