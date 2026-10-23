// 원본 Editgump의 실제 문자 편집 노드와 Console 파생을 공통 Gump/문자 버퍼에 연결한다.
#pragma once
#include "client/GumpBackgroundCreate.h"
#include "client/ScrollGump.h"
#include "client/TextEntryBuffer.h"
#include <optional>
#include <string>

namespace netstorm::client {
// VT00508e10의 +b8..108 필드다. 실제 배경/기본 Gump 주소를 그대로 상속한다.
struct TextEntryGumpNode : GumpBackgroundNode {
    std::uint32_t verticalSlider{},horizontalSlider{},ownsBuffer{},reservedC4{}; // +b8/+bc/+c0/+c4이며 c4는 생성자가 쓰지 않는다.
    double blinkAt{}; // +c8의 다음 caret 점멸 시각이다.
    std::uint32_t blinkPhase{},readOnly{}; // +d0/+d4의 caret 위상과 입력/점멸 금지 관문이다.
    std::int32_t shiftAnchor{},shiftEnd{},scrollLine{},scrollColumn{}; // +d8/+dc/+e0/+e4의 선택 기준과 현재 표시 원점이다.
    std::uint32_t showVertical{},showHorizontal{},forwardInput{},multiline{},changeNotice{},recipientHighlight{}; // +e8..fc의 실제 원시DWORD다.
    std::int32_t maxLength{}; // +100: 양수이면 현재length+1과 엄격 비교하는 문자 입력 제한이다.
    std::uint32_t excludedChars{},buffer{}; // +104의 별도 소유 NUL문자열 토큰과 +108의 실제0x28 버퍼 토큰이다.
};
// VT00506a6c Console은 같은 TextEntry 위에 autocomplete/제출 callback을 더한다.
struct ConsoleGumpNode : TextEntryGumpNode {
    std::uint32_t reserved10C{},autocomplete{},submit{},resetWord{},wordStart{},wordLength{},reserved124{}; // +10c/+110..124, 앞뒤 예약DWORD는 생성자가 쓰지 않는다.
};
// 런타임 종류BYTE·팔레트/사건 전역은 실제 Client가 갱신한다.
struct TextEntryGumpState {
    std::int8_t kind{15},consoleKind{10}; //0054042a/425의 실제 signed 종류BYTE다.
    std::uint32_t brown{},black{},yellow{}; //005acd24/005a4058/0059afd8의 현재 파일 팔레트 색이다.
    std::uint32_t enterEvent{0x02000038},changeEvent{0x020000bf},sliderChange{0x02000036},sliderStep{0x0200002e}; //005427d8/5429f0/5427d0/5427b0의 실제 사건이다.
};
// 실제 노드/문자 버퍼/Slider와 장치 경계만 제공한다. 빈 성공 노드를 만들지 않는다.
struct TextEntryGumpHooks {
    std::function<TextEntryGumpNode*(std::uint32_t)> node; // 공통 기본 저장소와 같은 파생 주소다.
    std::function<ConsoleGumpNode*(std::uint32_t)> console; // Console 분기에 도달할 때만 현재 파생 주소를 조회한다.
    std::function<GumpCreateNode*(std::uint32_t)> base; // 현재 모든 노드의 같은 기본 주소다.
    std::function<TextEntryBuffer*(std::uint32_t)> buffer; // +108의 실제 같은 상태/저장소 처리기다.
    std::function<ScrollGumpNode*(std::uint32_t)> slider; // 같은 실제 두 Slider의 현재 position을 재조회한다.
    std::function<std::uint32_t(std::size_t)> allocate; //0x28 버퍼 객체/0xd8 Slider 객체를 같은 소유 저장소에 확보한다.
    std::function<void(std::uint32_t)> releaseBuffer; // 버퍼 Destroy 뒤 객체 자체를 반납한다.
    std::function<void(std::uint32_t,std::uint32_t)> constructBackground; //004655d0의 this=token 및 여섯 인자(0,0,flags,1,0,0)의 실제 생성이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t,std::uint32_t,std::uint32_t)> constructSlider; //004a64b0과 같은 실제 세 자식 생성이다.
    std::function<void(std::uint32_t,float,float,std::uint32_t,std::uint32_t)> baseAttach,attach; // 공통00466750과 현재 가상+20이다.
    std::function<void(std::uint32_t)> push; // 현재 가상+18이다.
    std::function<void(std::uint32_t,bool)> idle,redraw; // 실제00465330 등록과 가상+8이다.
    std::function<void(std::uint32_t,std::uint32_t,std::uint32_t,std::uint32_t)> bubble; //00464e90의 현재 부모 사건 전달이다.
    std::function<void(std::uint32_t,std::int32_t,std::int32_t)> sliderRange; // 기존ScrollGump::SetRange다.
    std::function<void(std::uint32_t,std::int32_t)> sliderPage,sliderPosition; // 기존ScrollGump의 실제 page/position 처리다.
    std::function<double()> now; //00460d70의 현재 벽 시계 초다.
    std::function<std::array<std::int32_t,2>(std::uint32_t,std::span<const char>,std::int32_t)> measure; //00465020의 실제 현재 기본 문맥 측정이다.
    std::function<ScreenPoint(std::uint32_t)> position; //00464740 공통 위치 조회다.
    std::function<void(std::uint32_t)> drawBackground; //00465c60의 실제 같은 배경 픽셀 출력이다.
    std::function<void(std::uint32_t,std::span<const char>,ScreenPoint,const std::array<std::uint32_t,8>*)> drawText; //00465050의 현재 기본/명시 문맥 상대 출력이다.
    std::function<void(ScreenRect,std::uint32_t)> fill; //004a2060의 같은 현재 클립/픽셀 채우기다.
    std::function<bool(std::string_view)> recipient; //0042dc70의 실제 수신 대상 검사다.
    std::function<std::uint32_t(std::size_t)> allocateBytes; //00451b80의 별도 NUL 문자열 할당이다.
    std::function<void(std::uint32_t)> releaseBytes; // 제외 문자열 교체 시의 실제 free다.
    std::function<std::span<std::uint8_t>(std::uint32_t)> bytes; // 같은 실제 문자 주소의 순수 범위 조회다.
    std::function<std::optional<std::string>(std::uint32_t,std::optional<std::string_view>,std::int32_t,bool)> complete; // Console callback의 새 단어/null 순환 요청과 null/문자열 반환을 구별한다.
    std::function<void(std::uint32_t,std::string_view)> submit; // Console +114의 현재 입력 제출 callback이다.
    std::function<void(std::uint32_t)> assertion; // 원본 줄 번호 진단이며 돌아오면 정상 원문 흐름을 계속한다.
    std::function<void(std::uint32_t)> destructBase; //00464420의 실제 텍스트/idle/기본 수명 반납이다.
};
// 같은 문자 버퍼와 Gump 저장소에 원본 필수 생성/입력/기하/Draw를 연결한다.
class TextEntryGump {
public:
    TextEntryGump(TextEntryGumpState& state,TextEntryGumpHooks hooks); // 조회만 생성 시 검증하며 장치는 실제 도달할 때 요구한다.
    std::uint32_t Construct(std::uint32_t token,std::int32_t width,std::int32_t height,std::optional<std::string_view> initial,
        std::uint32_t font,std::uint32_t background,std::uint32_t color) const; //00452430의 실제 버퍼와 두 Slider를 생성한다.
    std::uint32_t ConstructConsole(std::uint32_t token,std::int32_t width,std::int32_t height,std::optional<std::string_view> initial,
        std::uint32_t font,std::uint32_t background,std::uint32_t color) const; //0043f820의 같은 TextEntry 생성 뒤 파생 초기화다.
    void Attach(std::uint32_t token,float x,float y,std::uint32_t parent,std::uint32_t mode) const; //004527b0이다.
    void SetScroll(std::uint32_t token,std::int32_t line,std::int32_t column) const; //00451c90의 현재 줄 조회/측정/두 Slider 순서다.
    void RelativeScroll(std::uint32_t token,std::int32_t line,std::int32_t column) const; //004528b0의 DWORD delta다.
    std::uint32_t Handle(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; //004528e0이다.
    std::uint32_t HandleConsole(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; //0043f8a0의 Tab/제출 뒤 기본 위임이다.
    void Draw(std::uint32_t token) const; //00451f00의 배경·각 줄·선택·caret 출력이다.
    void Idle(std::uint32_t token) const; //00451e90의 실제 시각 두 번 조회/점멸이다.
    void SetText(std::uint32_t token,std::span<const char> text={}) const; //00451e00의 Reset 뒤 Redraw다.
    void AppendText(std::uint32_t token,std::span<const char> text,bool restoreSelection=false) const; //00451e20의 끝 삽입/선택 복구다.
    std::uint32_t CopyText(std::uint32_t token,std::span<char> output) const; //00451e80의 실제 CopyAll 위임이다.
    void SetExcluded(std::uint32_t token,std::optional<std::string_view> text) const; //00451b80의 현재 제외문자 소유 교체다.
    void SetHorizontal(std::uint32_t token,std::uint32_t value) const; //00451c60의 필드 쓰기 뒤 현재 위치 재부착다.
    void Destruct(std::uint32_t token,bool console=false) const; //00452730/43f890의 소유 버퍼만 반납한다.
private:
    TextEntryGumpNode& Node(std::uint32_t token) const; // 같은 파생/기본 주소를 검증한다.
    ConsoleGumpNode& Console(std::uint32_t token) const; // 같은 Console/기본 주소를 검증한다.
    GumpCreateNode& Base(std::uint32_t token) const; // 실제 도달한 자식만 조회한다.
    TextEntryBuffer& Buffer(std::uint32_t token) const; // 노드의 현재 +108을 매번 재조회한다.
    ScrollGumpNode& Slider(std::uint32_t token) const; // 현재 가상 호출 뒤 같은 Slider position을 읽는다.
    void Redraw(std::uint32_t token) const; // 가상+8(false)를 현재 노드에 요청한다.
    void Bubble(std::uint32_t token,std::uint32_t event,std::uint32_t first,std::uint32_t second) const; // 현재 부모 체인에 전달한다.
    std::array<std::int32_t,2> Measure(std::uint32_t token,std::span<const char> text,std::int32_t length=-1) const; // 현재 기본 문맥을 측정한다.
    std::span<const char> Excluded(std::uint32_t token) const; // 현재 별도 NUL 문자열을 빌린다.
    TextEntryGumpState& state_; // 현재 팔레트/사건/종류 전역이다.
    TextEntryGumpHooks hooks_; // 공통 트리/문자 버퍼와 외부 장치만 연결한다.
};
}
