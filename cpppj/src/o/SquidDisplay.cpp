// 패치 004ad500/004990d0/00498ff0 ↔ CD 004ae380/00456a00/00456930.
#include "o/SquidDisplay.h"
#include "o/RawRiftShapeAccess.h"
#include "o/RawSquidDisplayParent.h"
#include "o/SquidUnpop.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <limits>
#include <optional>
#include <stdexcept>
#include <utility> //현재view장치의함수소유권만이동한다.

namespace netstorm::o {
namespace {
// 표시 레이어와 extra의 선택/표식 비트다.
constexpr std::uint32_t kDoubleFrames=0x440000,kShadow=0x40000,kMarker=0x10;
constexpr std::uint32_t kPatchManaVtable=0x50c3b0; // 실제 마나 update88=473770이며8c는 같은88로 가상 꼬리 호출한다.
constexpr std::uint32_t kPatchGeyserVtable=0x50da88; // 부분8c=004854b0이며 일반88과 다른 고정 외형 프레임을 사용한다.
constexpr int kGeyserVerticalInset=50; // 원문 lazy 사각형의 위/아래 픽셀 여백이다. 확대율을 곱하지 않는다.
// WORD0인 신전 가상8c는 실제 rune 사각형의 네 inset으로 본그림만 갱신한다. 썬 신전은 기본88의 thunk다.
std::optional<std::array<int,4>> VortexInsets(OriginalEdition edition,std::uint32_t table) {
    const bool patch=edition==OriginalEdition::Patch1078;
    if (table==(patch ? 0x5196f0U : 0x501d28U)) return std::array<int,4>{12,29,12,23};
    if (table==(patch ? 0x519860U : 0x501de0U)) return std::array<int,4>{39,30,33,34};
    if (table==(patch ? 0x519918U : 0x501e98U)) return std::array<int,4>{27,0,28,30};
    return std::nullopt;
}
// little endian raw 값을 읽으며 부족한 슬롯을 먼저 거부한다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset,std::size_t width=4) {
    if (offset>bytes.size() || width>bytes.size()-offset) throw std::out_of_range("표시 raw 슬롯 범위 오류");
    std::uint32_t value=0;
    // CD의 1바이트 frame 옆 extra를 읽지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(8*i);
    return value;
}
// 새 구현은 큰/손상된 입력의 정수 좌표 넘침을 거부한다.
int Coordinate(std::int64_t value) {
    if (value<std::numeric_limits<int>::min() || value>std::numeric_limits<int>::max())
        throw std::overflow_error("표시 좌표 넘침");
    return static_cast<int>(value);
}
// x87의 x*16/y*11+0.5 뒤 절삭이다. float 곱셈으로 먼저 반올림하지 않는다.
int Project(float value,double scale,int camera) {
    const auto pixel=static_cast<double>(value)*scale+0.5;
    if (!std::isfinite(pixel) || pixel<std::numeric_limits<int>::min() || pixel>std::numeric_limits<int>::max())
        throw std::out_of_range("표시 좌표 범위 오류");
    return Coordinate(static_cast<std::int64_t>(pixel)-camera);
}
// 원본 imul low DWORD와 산술 우측 이동을 정의된 비트 연산으로 보존한다.
int Scale(std::int16_t value,int zoom) {
    const auto product=static_cast<std::uint32_t>(static_cast<std::int64_t>(value)*zoom);
    return std::bit_cast<std::int32_t>(product)>>16;
}
// 원본은 교집합 계산 대신 각 모서리를 독립 clamp한다.
SquidDisplayRect Clip(SquidDisplayRect r,SquidDisplayRect v) {
    return {std::clamp(r.left,v.left,v.right),std::clamp(r.top,v.top,v.bottom),
        std::clamp(r.right,v.left,v.right),std::clamp(r.bottom,v.top,v.bottom)};
}
// 양의 면적만 변경 표에 보낸다.
bool Nonempty(SquidDisplayRect r) { return r.left<r.right && r.top<r.bottom; }
}
//43d40의 원문 서식/이름/현재 프레임 개수를 호스트 소유 문자열에 그대로 보관한다.
std::string BuildSquidFrameAssertionDetail(std::span<const RiftTypeRecord> types,std::span<const SquidDisplayShape> shapes,std::uint32_t type,std::int32_t frame,std::uint32_t abstract) {
    if (types.size()!=shapes.size()) throw std::invalid_argument("표시 오류 상세 TYPE/shape 개수 불일치");
    const auto signedType=std::bit_cast<std::int32_t>(type);const bool valid=signedType>=10 && type<types.size(); // 원문은 signed TYPE 범위를 먼저 판정한다.
    const auto name=valid ? types[type].name : std::string("invalidType");const auto frames=valid ? shapes[type].frameCount : 0; // 실제 getType 두 조회의 읽기 전용 현재 필드다.
    return "Frame assertion failed. Asking for type="+std::to_string(signedType)+" ("+name+") frame="+std::to_string(frame)+" abs="+std::to_string(std::bit_cast<std::int32_t>(abstract))+"\n\nType has "+std::to_string(frames)+" frames\n";
}
// 타입 번호 체계가 다른 목록의 혼용을 막는다.
SquidDisplay::SquidDisplay(OriginalEdition edition,std::span<const RiftTypeRecord> types,
    std::span<const SquidDisplayShape> shapes,SquidDisplaySink& sink,SquidDisplayView view)
    :edition_(edition),types_(types),shapes_(shapes),sink_(sink) {
    if (types.size()!=shapes.size()) throw std::invalid_argument("표시 타입/SHP 목록 크기 오류");
    SetView(view);
}
// 역전 viewport와 양수 아닌 확대율을 장치 호출 전에 거부한다.
void SquidDisplay::BindView(std::function<SquidDisplayView()> view) { currentView_=std::move(view); } //생성시값형view는독립호환이며실제부모는같은장기getter를쓴다.
SquidDisplayView SquidDisplay::CurrentView() const { return currentView_ ? currentView_():view_; } //진단/갱신효과뒤의다음좌표읽기도현재Client에서구한다.
void SquidDisplay::SetView(SquidDisplayView view) {
    if (view.zoom<=0 || view.viewport.left>view.viewport.right || view.viewport.top>view.viewport.bottom)
        throw std::invalid_argument("표시 카메라/viewport 오류");
    view_=view;
}
// 타입 번호가 같아도 다른 표의 플래그/발자국으로 공간을 쓰는 연결을 구성 전에 거부한다.
void SquidDisplay::ValidateFrameBinding(OriginalEdition edition,std::span<const RiftTypeRecord> types) const {
    if (edition!=edition_ || types.size()!=types_.size()) throw std::invalid_argument("프레임 표시 판본/타입 목록 불일치");
    // 표시 레이어/유효성 및 공간 재등록이 사용하는 타입 필드를 비교한다.
    for (std::size_t i=0;i<types.size();++i) {
        const auto& a=types[i];const auto& b=types_[i];
        if (a.flags1!=b.flags1 || a.flags2!=b.flags2 || a.maxHitPoints!=b.maxHitPoints || a.footX!=b.footX || a.footY!=b.footY)
            throw std::invalid_argument("프레임 표시 타입 내용 불일치");
    }
}
// 해시 단계용 헤더는 표시 억제와 무관하게 읽는다. shadow/abstract 물리 프레임 순서도 그대로 유지한다.
std::array<float,2> SquidDisplay::FrameSize(OriginalEdition edition,std::span<const std::uint8_t> bytes,int frame) const {
    if (edition!=edition_) throw std::invalid_argument("프레임 SHP 판본 불일치");
    const auto type=Read(bytes,10,1);
    if (type>=shapes_.size()) throw std::out_of_range("프레임 SHP 타입 번호 범위 오류");
    if (edition_==OriginalEdition::Patch1078) {
        RiftShapeAccessHooks hooks;hooks.typeNumber=[](RawSquidDisplayTypeView selected) { return selected.token; };
        hooks.frameError=[this](auto number,auto savedFrame,auto abstract) { sink_.FrameError(number,savedFrame,abstract); };
        hooks.assertion=[this](auto expression,auto file,auto line) { sink_.Assertion(expression,file,line); };
        hooks.shape=[this](std::uint32_t selected,std::int32_t savedFrame) {
            const auto& current=shapes_[selected];if (!current.loaded || savedFrame<0 || static_cast<std::size_t>(savedFrame)>=current.frames.size()) throw std::out_of_range("진단복귀뒤SHP물리범위오류");
            const auto& header=current.frames[static_cast<std::size_t>(savedFrame)];return std::array<float,2>{header.cellWidth,header.cellHeight};
        };
        return ReadRiftShapeFrame({type,types_[type].flags1,shapes_[type].frameCount,shapes_[type].loaded,types_[type].maxHitPoints},frame,0,hooks); //004ace40은rawabstract대신고정인자0을전달한다.
    }
    const auto& shape=shapes_[type];
    if (!shape.loaded)
        throw std::out_of_range("프레임 SHP 조회 실패(00419850 frameCheck)");
    if (frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("프레임 SHP 물리 범위 오류");
    const auto& header=shape.frames[static_cast<std::size_t>(frame)];
    return {header.cellWidth,header.cellHeight};
}
// 패치의 일반 경로에도 남아 있는 frameCheck 조건이다. debug/assert UI는 복원 범위 밖이다.
bool SquidDisplay::PatchFrameValid(std::size_t type,int frame) const {
    const auto& record=types_[type]; const auto& shape=shapes_[type];
    const auto count=static_cast<std::int64_t>(shape.frameCount)*((record.flags1&kDoubleFrames)!=0 ? 2 : 1);
    return frame>=0 && frame<count && shape.loaded && record.maxHitPoints!=-0x22222223 && record.maxHitPoints!=-0x32323233;
}
// 미복원 파생 update88/update8c는 표시가 꺼져도 지원한다고 가장하지 않는다.
void SquidDisplay::Validate(OriginalEdition edition,std::span<const std::uint8_t> bytes,std::uint32_t flags) const {
    if (edition!=edition_) throw std::invalid_argument("표시 판본 불일치");
    if (edition_==OriginalEdition::Patch1078 && (Read(bytes,0)==0x50c9f0U || Read(bytes,0)==0x50cc08U)) return; //실제 메타/일반 Sprite88/8c는 공통 frameCheck 부모가 아니며 사전 검증에서 표시 효과를 앞당기지 않는다.
    if (!SquidUnpop::SupportsDisplay(edition_,Read(bytes,0),flags)) throw std::logic_error("파생 표시 갱신 미복원");
    if (edition_==OriginalEdition::Patch1078 && Read(bytes,0)==kPatchGeyserVtable && (flags&0x2000U)) {
        const auto type=Read(bytes,10,1);if (type>=types_.size()) throw std::out_of_range("간헐천 고정 외형 TYPE 범위 오류");
        const auto frame=types_[type].defaultFrame; // 실제 가상78은 표시 억제보다 먼저 현재 TYPE+128을 읽는다.
        if (!sink_.Suppressed()) static_cast<void>(Bounds(type,frame,std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18))));
        return; // 현재 애니메이션 프레임/WORD/extra/그림자는 이 경로의 입력이 아니다.
    }
    // 마나의 WORD0 경로는 TYPE/SHP/프레임을 읽지 않는다. WORD 비영점도 억제 중이면 bounds에 진입하지 않는다.
    if (edition_==OriginalEdition::Patch1078 && (Read(bytes,0)==kPatchManaVtable || Read(bytes,0)==0x509810)) {
        if ((Read(bytes,0)==0x509810 || Read(bytes,12,2)!=0) && !sink_.Suppressed()) static_cast<void>(Bounds(Read(bytes,10,1),std::bit_cast<std::int32_t>(Read(bytes,36)),std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18))));return;
    }
    const auto type=Read(bytes,10,1);
    if (type>=types_.size()) throw std::out_of_range("표시 타입 번호 범위 오류");
    const bool patch=edition_==OriginalEdition::Patch1078;
    const auto frame=patch ? std::bit_cast<std::int32_t>(Read(bytes,36)) : static_cast<int>(Read(bytes,34,1));
    // 부분 갱신의 물리 계약만 검증한다. 실제 변경 영역은 Update에서 한 번만 등록한다.
    if ((flags&0x2000) && Read(bytes,12,2)==0 && VortexInsets(edition_,Read(bytes,0))) {
        if (!sink_.Suppressed()) static_cast<void>(Bounds(type,frame,std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18))));
        return;
    }
    static_cast<void>(Read(bytes,patch ? 40 : 35,1));
    if ((patch && !PatchFrameValid(type,frame)) || sink_.Suppressed()) return;
    const auto& shape=shapes_[type];
    if (!shape.loaded) return;
    const auto last=static_cast<std::int64_t>(frame)+((types_[type].flags1&kShadow)!=0 ? shape.frameCount : 0);
    if (frame<0 || shape.frameCount<0 || last<0 || last>=static_cast<std::int64_t>(shape.frames.size()))
        throw std::out_of_range("표시 물리 프레임 범위 오류");
}
// VFX 압축 사각형 대신 별도 Squid 헤더의 pixel 크기와 hotspot을 사용한다.
SquidDisplayRect SquidDisplay::Bounds(std::size_t type,int frame,float x,float y) const {
    if (type>=types_.size()) throw std::out_of_range("표시 타입 번호 범위 오류");
    const auto& shape=shapes_[type];
    if (!shape.loaded) return {};
    if (frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("표시 프레임 범위 오류");
    const auto& header=shape.frames[static_cast<std::size_t>(frame)];
    const auto left=Coordinate(static_cast<std::int64_t>(Project(x,16,CurrentView().cameraX))-Scale(header.hotspotX,CurrentView().zoom));
    const auto top=Coordinate(static_cast<std::int64_t>(Project(y,11,CurrentView().cameraY))-Scale(header.hotspotY,CurrentView().zoom));
    return {left,top,Coordinate(static_cast<std::int64_t>(left)+Scale(header.width,CurrentView().zoom)+1),
        Coordinate(static_cast<std::int64_t>(top)+Scale(header.height,CurrentView().zoom)+1)};
}
// 현재 VT의 특수8c를 먼저 분배하고 나머지 확인된 가상 경로만 공통 표시 부모에 전달한다.
void SquidDisplay::Update(std::span<const std::uint8_t> bytes,std::uint32_t flags) {
    if (edition_==OriginalEdition::Patch1078 && (Read(bytes,0)==0x50c9f0U || Read(bytes,0)==0x50cc08U)) { sink_.MetaRefresh(bytes);return; } //8c의004ad670도 현재88로 꼬리 호출하므로 flags와 무관하게 같은 실제 파생을 선택한다.
    if (edition_==OriginalEdition::Patch1078 && Read(bytes,0)==kPatchGeyserVtable && (flags&0x2000U)) {
        const auto type=Read(bytes,10,1);if (type>=types_.size()) throw std::out_of_range("간헐천 고정 외형 TYPE 범위 오류");
        const auto frame=types_[type].defaultFrame; // 004acfe0은 현재 raw TYPE의 고정 외형을 반환하며 frameCheck 부모를 호출하지 않는다.
        if (sink_.Suppressed()) return;
        auto rect=Bounds(type,frame,std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18)));
        rect.top=Coordinate(static_cast<std::int64_t>(rect.top)+kGeyserVerticalInset);
        rect.bottom=Coordinate(static_cast<std::int64_t>(rect.bottom)-kGeyserVerticalInset);
        rect=Clip(rect,CurrentView().viewport);if (Nonempty(rect)) sink_.Invalidate(rect,0);
        return; // 원문의 유일한 cache 소비자/작성자가 고정한 (0,50,0,50)을 같은 픽셀 순서로 적용한다.
    }
    const auto table=Read(bytes,0);const bool patchParent=edition_==OriginalEdition::Patch1078 && table!=kPatchManaVtable && table!=0x509810; // 파생 마나/볼트 부분 표시는 기존 몸체다.
    const bool vortexPart=patchParent && (flags&0x2000) && Read(bytes,12,2)==0 && VortexInsets(edition_,table).has_value(); // 신전 부분 갱신도 기본88 부모를 호출하지 않는다.
    if (patchParent && !vortexPart) {
        if (!SquidUnpop::SupportsDisplay(edition_,table,flags)) throw std::logic_error("파생 표시 갱신 미복원");
        RawSquidDisplayParentHooks hooks; // 타입/shape는 현재 같은 배열의 필드 주소를 직접 빌린다.
        hooks.type=[this](std::uint32_t type) { if (type>=types_.size()) throw std::out_of_range("표시 타입 번호 범위 오류");return RawSquidDisplayTypeView{type,types_[type].flags1,shapes_[type].frameCount,shapes_[type].loaded,types_[type].maxHitPoints}; };
        hooks.typeNumber=[](RawSquidDisplayTypeView type) { return type.token; }; // 실제49adc0의 표 인덱스 반환이다.
        hooks.frameError=[this](auto type,auto frame,auto abstract) { sink_.FrameError(type,frame,abstract); };
        hooks.assertion=[this](auto expression,auto file,auto line) { sink_.Assertion(expression,file,line); };
        hooks.mark=[this](auto type,auto frame,auto x,auto y,auto marker) { Mark(type,frame,std::bit_cast<float>(x),std::bit_cast<float>(y),marker); };
        RunRawSquidDisplayParent(bytes,sink_.Checking(),hooks);return;
    }
    Validate(edition_,bytes,flags);
    // 473770→499240의 실제 부분 갱신이다. 추가10픽셀은 Q16 확대율을 곱하지 않으며 선택/그림자를 더하지 않는다.
    if (edition_==OriginalEdition::Patch1078 && (Read(bytes,0)==kPatchManaVtable || Read(bytes,0)==0x509810)) {
        const bool bolt=Read(bytes,0)==0x509810; // 45ac90은 WORD와 무관하다. 499240이 음수 여백을 빼므로 오른쪽/아래가10픽셀 늘어난다.
        if ((!bolt && !Read(bytes,12,2)) || sink_.Suppressed()) return;
        auto rect=Bounds(Read(bytes,10,1),std::bit_cast<std::int32_t>(Read(bytes,36)),std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18)));
        const auto extra=10;rect.right=Coordinate(static_cast<std::int64_t>(rect.right)+extra);rect.bottom=Coordinate(static_cast<std::int64_t>(rect.bottom)+extra);rect=Clip(rect,CurrentView().viewport);if (Nonempty(rect)) sink_.Invalidate(rect,0);return;
    }
    const bool patch=edition_==OriginalEdition::Patch1078; const auto type=Read(bytes,10,1);
    const auto frame=patch ? std::bit_cast<std::int32_t>(Read(bytes,36)) : static_cast<int>(Read(bytes,34,1));
    // 실제 부분 갱신 helper는 표시 억제부터 검사하고 선택 표식/그림자 갱신을 추가하지 않는다.
    if ((flags&0x2000) && Read(bytes,12,2)==0) {
        if (const auto insets=VortexInsets(edition_,Read(bytes,0))) {
            if (sink_.Suppressed()) return;
            const auto x=std::bit_cast<float>(Read(bytes,14)),y=std::bit_cast<float>(Read(bytes,18));
            auto rect=Bounds(type,frame,x,y);rect.left=Coordinate(static_cast<std::int64_t>(rect.left)+(*insets)[0]);
            rect.top=Coordinate(static_cast<std::int64_t>(rect.top)+(*insets)[1]);
            rect.right=Coordinate(static_cast<std::int64_t>(rect.right)-(*insets)[2]);
            rect.bottom=Coordinate(static_cast<std::int64_t>(rect.bottom)-(*insets)[3]);
            rect=Clip(rect,CurrentView().viewport);if (Nonempty(rect)) sink_.Invalidate(rect,0);return;
        }
    }
    if (patch && !PatchFrameValid(type,frame)) return;
    Mark(type,frame,std::bit_cast<float>(Read(bytes,14)),std::bit_cast<float>(Read(bytes,18)),(Read(bytes,patch ? 40 : 35,1)&4)!=0 ? kMarker : 0);
}
// 기존4990d0 본문을 현재 인자로 실행한다. 진단 앞의 공간 preflight와 실제 표시 효과를 구별한다.
void SquidDisplay::Mark(std::size_t type,int frame,float x,float y,std::uint32_t marker) {
    if (sink_.Suppressed()) return;const bool patch=edition_==OriginalEdition::Patch1078; // 실제 표시 억제는 부모 진단 이후 자식 진입에서 평가한다.
    auto main=Clip(Bounds(type,frame,x,y),CurrentView().viewport);
    if (marker) {
        if (static_cast<std::int64_t>(main.right)-main.left<19) { main.left=Coordinate(static_cast<std::int64_t>(main.left)-9); main.right=Coordinate(static_cast<std::int64_t>(main.right)+9); }
        main.top=Coordinate(static_cast<std::int64_t>(main.top)-(patch ? 15 : 9));
        main.left=Coordinate(static_cast<std::int64_t>(main.left)-3); main.right=Coordinate(static_cast<std::int64_t>(main.right)+3);
    }
    // clamp 뒤 확장한 범위는 다시 viewport로 자르지 않는다. Dirty/Draw 단계가 화면을 처리한다.
    if (Nonempty(main)) sink_.Invalidate(main,marker);
    if ((types_[type].flags1&kShadow)!=0) {
        const auto shadow=Clip(Bounds(type,frame+shapes_[type].frameCount,x,y),CurrentView().viewport);
        if (Nonempty(shadow)) sink_.Invalidate(shadow,marker);
    }
}
}
