// 원본 StyleText의 개행 줄 저장소와 폭에 따라 다시 만든 표시 줄을 같은 조각 체인에 연결한다.
#pragma once
#include "client/StyleText.h"

namespace netstorm::client {
// 기존 그림 조각 처리기다. 버퍼는 이 처리기와 같은 공통 조각/파생 저장소를 사용한다.
class Markup;
// 원본 24바이트 표시 줄의 소유 head·높이·폭·세로 위치·양방향 링크다. 0 링크는 센티널을 뜻한다.
struct StyleTextBufferLine {
    std::uint32_t head{};
    std::int32_t height{},width{},y{};
    std::uint32_t next{},previous{};
};
// +0 max/+4 완료 줄 수/+8 전체 칸/+c 센티널/+24 표시 줄 수/+28 미사용/+2c 변경/+30 줄바꿈 금지다.
// hardLines[lineCount]는 아직 개행하지 않은 끝 줄이며 완료 줄 수에 포함되지 않는다.
struct StyleTextBufferState {
    std::int32_t maxLines{},lineCount{},softCount{};
    std::vector<std::uint32_t> hardLines;
    StyleTextBufferLine sentinel;
    std::map<std::uint32_t,StyleTextBufferLine> softLines;
    std::uint32_t reserved{},dirty{},noWrap{},nextSoftToken{1};
    bool initialized{};
    // 원본 +34 통지는 문자열 추가 직전에 입력을 받는다. 그림 추가에서는 호출하지 않는다.
    std::function<void(std::span<const char>)> onAppend;
};
// 문자열 전체/명시 길이는 실제 Screen 측정에, 비문자 조각 복제는 원본 가상 +10에 연결한다.
struct StyleTextBufferHooks {
    std::function<std::array<std::int32_t,2>(std::span<const char>,const std::array<std::uint32_t,8>&,std::int32_t)> measureText;
    std::function<std::uint32_t(std::uint32_t)> cloneExternal;
};
// TextGump 계열이 사용하는 원본 다중 줄 버퍼다. 입력 줄과 표시 복제 조각은 서로 다른 소유권을 가진다.
class StyleTextBuffer {
public:
    // 상태는 새 저장소여야 한다. style은 fragments와 같은 조각 표/파생 삭제 경계를 사용해야 한다.
    StyleTextBuffer(StyleTextBufferState& state,StyleTextState& fragments,const StyleText& style,
        std::int32_t maxLines,StyleTextBufferHooks hooks);
    // 004b9790/CD 004b54f0: 음수 길이는 NUL 탐색, 양수 길이는 지정 범위다. LF에서만 줄 수를 늘린다.
    // 끝 칸이 차면 완료된 첫 줄을 버리고 모든 칸을 이동한다. 마지막 미완료 구간도 같은 태그를 보관한다.
    void AppendText(std::span<const char> bytes,const std::array<std::uint32_t,8>& context,
        std::int32_t length=-1,std::uint32_t metadata=0) const;
    // 004b9690/CD 004b53d0: 현재 끝 줄에 그림을 더하고 두 맞춤 값 중 하나라도 비0이면 Fit을 호출한다.
    void AppendPicture(const Markup& pictures,std::uint32_t type,std::uint32_t frame,
        std::uint32_t metadata=0,std::int32_t fitWidth=0,std::int32_t fitHeight=0) const;
    // 004b95e0/CD 004b52e0: 완료 줄을 scalar 삭제한 뒤 미완료 끝 칸까지 포함해 전체 배열을 앞으로 이동한다.
    void RemoveLine(std::int32_t index) const;
    // 004b9a50/CD 004b5800: 양수 폭에서 기존 표시 조각을 반납하고 완료 줄만 원본 비교/공백 규칙으로 나눈다.
    // dirty는 원본처럼 호출자가 관리한다. 폭 0 이하이면 기존 표시 목록도 유지한다.
    void Layout(std::int32_t width) const;
    // 004b9ed0/CD 004b5be0: 현재 표시 줄을 지정 개수만 순회해 signed 최대 폭을 반환한다.
    std::int32_t MaximumWidth() const;
    // TextGump 스크롤 경계에서 쓰는 마지막 표시 줄 y+높이다. 표시 줄이 없으면 0이다.
    std::int32_t TotalHeight() const;
    // 004bbf20/CD 004b5190: 완료 입력 줄·표시 줄·센티널의 소유 조각을 반납한다.
    // 원본은 미완료 끝 칸을 삭제하지 않는다. 이 칸의 소유 조각은 호출자가 먼저 닫거나 직접 반납해야 한다.
    void Destroy() const;
private:
    // 현재 입력/표시 저장소가 살아 있는지 확인한다. 소멸 뒤의 재호출은 호스트 오류로 거부한다.
    void RequireStorage() const;
    // 현재 완료 줄 수에 대응하는 끝 칸을 읽는다. 배열 범위 밖 원본 쓰기는 거부한다.
    std::uint32_t& CurrentLine() const;
    // 공통 조각 표에서 현재 next/문자열/문맥/태그를 읽는다.
    StyleTextFragment& Fragment(std::uint32_t token) const;
    // 0은 센티널이며 비0은 안정된 표시 줄 저장소에서 조회한다.
    StyleTextBufferLine& Line(std::uint32_t token) const;
    // 다음 조각을 scalar 삭제 전에 저장한다. 호출자의 소유 head는 별도로 갱신한다.
    void DeleteChain(std::uint32_t head) const;
    // 기존 표시 목록만 반납하고 센티널 링크를 복구한다. resetCount는 레이아웃에서만 적용한다.
    void ClearSoft(bool resetCount) const;
    // 004b99e0/CD 004b5750: 지정 줄 뒤에 빈 줄을 삽입하고 전임 y+높이를 새 y로 저장한다.
    std::uint32_t NewSoftLine(std::uint32_t previous) const;
    // 저장된 NUL 문자열의 측정 범위를 구한다. 문맥/태그는 각 복사 직전에 다시 읽는다.
    std::span<const char> TextBytes(std::uint32_t token) const;
    // 원본 공백 단위 분할을 반복하며 첫 단어 강제 배치/빈 표시 줄/마지막 NUL 복사도 유지한다.
    void SplitText(std::uint32_t token,std::uint32_t& line,std::int32_t width) const;
    StyleTextBufferState& state_;
    StyleTextState& fragments_;
    const StyleText& style_;
    StyleTextBufferHooks hooks_;
};
// 전체/명시 길이의 원본 글자 폭·논리 슬롯 높이를 실제 FontStore/GumpText에서 공급한다.
void BindStyleTextBufferFonts(StyleTextBufferHooks& hooks,const GumpText& text);
// 같은 Markup의 가상 복제를 공급한다. 그림 복제의 배율 1/색 치환 0 규칙을 그대로 사용한다.
void BindStyleTextBufferMarkup(StyleTextBufferHooks& hooks,const Markup& pictures);
}
