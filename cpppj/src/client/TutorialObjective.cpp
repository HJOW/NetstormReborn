// 원본 진행 판정은 호출하지 않고 현재 단계에서 필요한 행동 안내만 공통 UI에 제출한다.
#include "client/TutorialObjective.h"
#include "client/RawMenuControls.h"
#include "o/OriginalText.h"
#include <algorithm>
#include <stdexcept>

namespace netstorm::client { // 초기 미션 추가 안내의 목록·배치·표시 수명을 구현한다.
namespace { // 안내 전용 불변 자료를 다른 UI 전역과 분리한다.
constexpr TutorialObjectiveEntry kObjectives[]{ // 영어 문구는 기존 번역표 키이고 단계 조건은 기존 판정기가 계속 소유한다.
#include "client/TutorialObjectiveCatalog.inc"
};
constexpr std::string_view kWaiting="Wait for the next tutorial message."; // 단계 예약과 실제 지연 타이머 동안 표시하는 공통 번역 키다.
constexpr int kMargin=8,kMaximumWidth=440,kLineSpacing=2; // 지도 오른쪽·아래 여백과 최대 글자 줄 폭·줄 사이 여백이다.
}
// 호출자는 불변 목록을 수정하거나 별도 완료 조건으로 실행하지 않는다.
std::span<const TutorialObjectiveEntry> TutorialObjectives() { return kObjectives; }
// 유형만 Tutorial인 일반 캠페인과 같은 번호를 쓰는 다른 맵을 공식 초기 미션으로 오인하지 않는다.
std::string_view TutorialObjectiveKey(std::string_view mission,const o::MissionFrameGateState& gate,std::uint32_t editing) {
    if (editing || gate.demo || !gate.started || gate.tutorialNumber<1 || gate.tutorialNumber>6) return {};
    if (o::AsciiLower(mission)!="tutorial"+std::to_string(gate.tutorialNumber)) return {};
    // 완료 섹션은 목록에 없으며 대화상자만 기존 순서로 표시된다.
    for (const auto& entry:kObjectives) if (entry.tutorial==gate.tutorialNumber && entry.stage==gate.stage) {
        if (gate.lockFrames>0 || (gate.timerSeconds!=0.0 && ((entry.tutorial==2 && (entry.stage=='F' || entry.stage=='H')) || (entry.tutorial==5 && entry.stage=='C')))) return kWaiting;
        return entry.english;
    }
    return {};
}
// 한글/일본어의4바이트 표시 시퀀스를 나누지 않고 영문은 가능하면 단어 사이에서 나눈다.
std::vector<std::string> WrapTutorialObjective(std::string_view bytes,int width,const std::function<int(std::string_view)>& measure) {
    if (width<=0 || !measure) throw std::invalid_argument("튜토리얼 안내의 실제 글꼴 폭/측정 연결 필요");
    std::vector<std::string> lines;std::size_t begin=0; // 아직 제출하지 않은 다음 줄의 첫 바이트다.
    const std::span<const std::uint8_t> raw(reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()); // 표시 부호화만 읽는 비소유 뷰다.
    // 매 반복에서 적어도 한 완전한 글자를 소비하며 매우 긴 영문 단어도 화면 폭 안에서 나눈다.
    while (begin<bytes.size()) {
        while (begin<bytes.size() && bytes[begin]==' ') ++begin; // 줄 시작의 단어 구분 공백은 그리지 않는다.
        if (begin==bytes.size()) break;
        std::size_t end=begin,lastSpace=std::string_view::npos; // 마지막으로 폭에 들어온 문자 끝과 단어 경계다.
        while (end<bytes.size() && bytes[end]!='\n') { // 폭 측정은 확장 시퀀스 전체를 포함한 줄 후보마다 실행한다.
            const auto next=end+(o::DecodeExtendedSequence(raw,end) ? o::kExtendedTextLength:1U);
            if (measure(bytes.substr(begin,next-begin))>width && end>begin) break;
            if (bytes[end]==' ') lastSpace=end;
            end=next;
        }
        const bool overflow=end<bytes.size() && bytes[end]!='\n'; // 개행과 폭 초과의 줄 끝을 구별한다.
        const auto cut=overflow && bytes[end]!=' ' && lastSpace!=std::string_view::npos && lastSpace>begin ? lastSpace:end;
        auto line=std::string(bytes.substr(begin,cut-begin));while (!line.empty() && line.back()==' ') line.pop_back(); // 줄 끝의 공백은 오른쪽 정렬 폭에 넣지 않는다.
        if (!line.empty()) lines.push_back(std::move(line));
        begin=cut;if (begin<bytes.size() && (bytes[begin]==' ' || bytes[begin]=='\n')) ++begin;
    }
    return lines;
}
// 공통 노드가 먼저 삭제됐거나 같은 토큰이 재사용돼도 자기 생성 serial만 선택한다.
void TutorialObjectiveGump::Clear(RawMenuControls& menus) {
    for (const auto& record:nodes_) { // 매 줄의 현재 등록 상태를 조회한 뒤 자기 생존 노드만 삭제 요청한다.
        menus.SetDisplayOnly(record[0],record[1],false);
        const auto* node=menus.Node(record[0]);
        if (node && node->serial==record[1] && !(node->flags&0x10000U)) menus.KillRegisteredGump(node->id);
    }
    nodes_.clear();text_.clear();root_=0;font_=nullptr;
}
// 기존 TextGump의 투명 글리프 출력·부분 갱신·등록/삭제를 그대로 사용해 카메라 이동 뒤 잔상을 남기지 않는다.
void TutorialObjectiveGump::Sync(RawMenuControls& menus,std::uint32_t root,const BitmapFont& font,std::string bytes,ScreenRect viewport,std::uint8_t color,std::uint8_t shadow) {
    const bool alive=std::all_of(nodes_.begin(),nodes_.end(),[&](const auto& record) { const auto* node=menus.Node(record[0]);return node && node->serial==record[1] && !(node->flags&0x10000U); }); // 부모가 먼저 정리한 노드도 다시 생성할 수 있다.
    if (bytes==text_ && root==root_ && font_==&font && color==color_ && shadow==shadow_ && viewport.left==viewport_.left && viewport.top==viewport_.top && viewport.right==viewport_.right && viewport.bottom==viewport_.bottom && alive) return;
    Clear(menus);
    if (bytes.empty() || !root || viewport.right-viewport.left<=2*kMargin || viewport.bottom-viewport.top<=2*kMargin) return;
    const auto width=std::min(kMaximumWidth,viewport.right-viewport.left-2*kMargin); // 생산 패널 밖의 실제 지도 너비만 사용한다.
    const auto lines=WrapTutorialObjective(bytes,width,[&font](auto line) { return font.Measure(line); });
    const auto lineHeight=std::max(font.Height(),font.ExtendedAdvance())+kLineSpacing; // 외부 비트맵 한글/일본어와 원본 영문 모두 수용한다.
    if (static_cast<int>(lines.size())*lineHeight>viewport.bottom-viewport.top-2*kMargin) throw std::length_error("튜토리얼 안내가 지도 표시 높이를 초과했습니다");
    text_=std::move(bytes);viewport_=viewport;root_=root;font_=&font;color_=color;shadow_=shadow;
    const std::array<std::uint32_t,8> context{2,0,color,0,0,shadow,1,1}; // 배경 없이 한 픽셀 그림자와 보통체 글리프만 출력한다.
    int y=viewport.bottom-kMargin-static_cast<int>(lines.size())*lineHeight; // 마지막 줄 아래에 지도 여백을 남긴다.
    for (const auto& line:lines) { // 실제 공통 노드마다 한 줄을 넣어 오른쪽 정렬하고 입력 후보에서는 제외한다.
        const auto token=menus.CreateMetaText(0,0,0,color);auto* node=menus.Node(token);
        nodes_.push_back({token,node->serial});menus.SetDisplayOnly(token,node->serial,true);
        menus.TextBackend().AppendText(token,{line.data(),line.size()},context,static_cast<std::int32_t>(line.size()));
        menus.TextBackend().Fit(token,2,2);
        menus.Attach(token,static_cast<float>(viewport.right-kMargin-font.Measure(line)-2),static_cast<float>(y),root,1);
        y+=lineHeight;
    }
}
// 보고서의 표시 여부도 마지막 실제 제출 상태를 사용한다.
std::string_view TutorialObjectiveGump::Text() const { return text_; }
}
