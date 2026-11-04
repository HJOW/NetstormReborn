// noIsland 최초 등록과 받침 소유자 전파. 독립 제한 x86 기대값 islandpostpop-x86.tsv로 검사한다.
#include "o/RawIslandPostPop.h"
#include <bit>
#include <stdexcept>
#include <string>
#include <utility>

namespace netstorm::o {
namespace {
// raw 타입·좌표 필드와 표면 지도 한 변/전체 칸 수다.
constexpr std::size_t kType=10,kX=14,kY=18,kSide=256,kCells=kSide*kSide;
// 첫 프레임 검색에 사용하는 변형/번호와 받침 묶음의 한 변이다.
constexpr std::uint8_t kVariant='P',kFrameNumber=1;
constexpr int kSupportSize=3;
// 표면 조회가 더하는 실제 float 0.9999와 CD H 조회가 빼는 1.0001이다(00500edc / CD 00502b70·0050692c).
constexpr float kSurfaceBias=std::bit_cast<float>(0x3f7ff972U),kCdHOffset=std::bit_cast<float>(0x3f800347U);
// 호스트 정렬에 의존하지 않고 DWORD를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value=0;
    // 낮은 바이트부터 조합한다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
// 프레임 필드의 폭만큼 쓰며 주변 바이트는 보존한다.
void WriteFrame(SidPool& pool,Sid sid,std::int32_t frame) {
    const auto slot=pool.Slot(sid);
    auto raw=pool.Edition()==OriginalEdition::Patch1078 ? std::span<std::uint8_t>(const_cast<std::uint8_t*>(slot.data()),slot.size()) : pool.AllocatedBytes(sid); // PATCH 원문의 직접 DWORD 저장은 물리 슬롯에 수행한다.
    if (pool.Edition()!=OriginalEdition::Patch1078) { raw[34]=static_cast<std::uint8_t>(frame);return; }
    const auto bits=std::bit_cast<std::uint32_t>(frame);
    // 패치의 DWORD 프레임 필드 네 바이트를 쓴다.
    for (std::size_t i=0;i<4;++i) raw[36+i]=static_cast<std::uint8_t>(bits>>(8*i));
}
// 원본 _ftol의 하위 DWORD. 비유한/64비트 밖 입력은 정수 불확정값의 하위 0이다.
std::int32_t Cell(double value) {
    if (!(value>-9223372036854775808.0 && value<9223372036854775808.0)) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::uint64_t>(static_cast<std::int64_t>(value))));
}
// 현재 raw 좌표를 float로 읽는다.
float Coordinate(const SidPool& pool,Sid sid,std::size_t offset) { return std::bit_cast<float>(Read(pool.Slot(sid),offset)); }
// 북쪽 칸에 따른 다음 행의 첫 글자. 서쪽의 알 수 없는 글자도 이 규칙을 따른다.
std::uint8_t FromNorth(std::uint8_t north) { return north=='F' ? 'B' : north=='B' ? 'I' : 'F'; }
// 원본 분기 순서대로 서쪽/북쪽 칸의 글자로 자신의 글자를 정한다.
std::uint8_t NextLetter(std::uint8_t west,std::uint8_t north) {
    switch (west) {
    case 'C':return 'G'; case 'F':return 'C'; case 'A':return 'D';
    case 'B':return 'A'; case 'E':return 'H'; case 'I':return 'E';
    default:return FromNorth(north);
    }
}
// 도달한 장치만 복사해 요구하며 콜백이 자기 연결을 반납해도 현재 호출의 수명을 유지한다.
template<class F> F SurfaceNeed(const F& callback) {
    auto selected=callback;if (!selected) throw std::logic_error("표면 후처리의 선택 장치 미연결");return selected;
}
// Native DEC의 DWORD 순환 뒤 signed 값을 좌표 덧셈에 올린다. x87 FILD는 중간 float 저장 없이 현재 좌표를 더한다.
double FootDelta(std::int32_t footprint) { return static_cast<double>(std::bit_cast<std::int32_t>(std::bit_cast<std::uint32_t>(footprint)-1U)); }
}
// 지도/타입 표의 물리 크기는 구성 때 확인한다. PATCH의 현재 TYPE/장치는 실제로 도달한 호출 위치에서만 요구한다.
RawIslandPostPop::RawIslandPostPop(SidPool& pool,std::span<const std::uint16_t> surfaceMap,
    std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,const BridgeConnectState& typeState,
    const IslandPostPopState& state,IslandPostPopHooks hooks,std::span<const std::uint16_t> ownerMap)
    :pool_(pool),surfaceMap_(surfaceMap),ownerMap_(ownerMap.empty() ? surfaceMap : ownerMap),types_(types),frames_(frames),
     typeState_(typeState),state_(state),hooks_(std::move(hooks)) {
    if (surfaceMap_.size()!=kCells || ownerMap_.size()!=kCells || types_.size()!=frames_.size())
        throw std::invalid_argument("noIsland postPop 지도/타입 표 범위 오류");
    if (pool_.Edition()!=OriginalEdition::Patch1078) {
        if (typeState_.islandType>=types_.size() || typeState_.noIslandType>=types_.size() || typeState_.stalagType>=types_.size())
            throw std::invalid_argument("CD noIsland postPop 타입 표 범위 오류");
        if (!hooks_.objects.create || !hooks_.objects.setOwner || !hooks_.objects.pop || !hooks_.connect || !hooks_.findTypeAt || !hooks_.updateDisplay || !hooks_.incompleteSupport)
            throw std::invalid_argument("CD noIsland postPop 훅이 모두 연결되어야 한다");
    }
}
Sid RawIslandPostPop::SurfaceAt(int x,int y,std::span<const std::uint16_t> map) const {
    if (x<0 || y<0 || x>=static_cast<int>(kSide) || y>=static_cast<int>(kSide)) return {};
    return Sid{map[static_cast<std::size_t>(y)*kSide+static_cast<std::size_t>(x)]};
}
int RawIslandPostPop::Letter(Sid sid) const {
    if (pool_.Edition()==OriginalEdition::Patch1078 && pool_.Slot(sid)[kType]<70)
        SurfaceNeed(hooks_.assertion)("!isForm()",R"(\Ns\O\Squid.cpp)",0x854); // 진단이 TYPE/프레임을 바꿀 수 있으므로 아래에서 모두 다시 읽는다.
    const auto raw=pool_.Slot(sid);
    if (raw[kType]>=frames_.size()) throw std::out_of_range("noIsland 프레임 타입 범위 오류");
    const auto codes=frames_[raw[kType]].Codes();
    const auto frame=pool_.Edition()==OriginalEdition::Patch1078 ? Read(raw,36) : raw[34];
    if (frame>=codes.size()) throw std::out_of_range("noIsland 프레임 번호 범위 오류");
    return pool_.Edition()==OriginalEdition::Patch1078 ? static_cast<int>(std::bit_cast<std::int8_t>(codes[frame].side)) : codes[frame].side;
}
std::uint8_t RawIslandPostPop::SurfaceLetter(int x,int y) const {
    // 서/북 조회는 고정 표면 지도(005c84bc / CD 0052d590)를 읽는다.
    const Sid sid=SurfaceAt(x,y,ownerMap_);
    return sid.value && pool_.Slot(sid)[kType]==typeState_.noIslandType ? static_cast<std::uint8_t>(Letter(sid)) : 0;
}
void RawIslandPostPop::Prefix(Sid surface,std::uint32_t flags) const {
    if ((flags&1U)==0) return;
    const bool patch=pool_.Edition()==OriginalEdition::Patch1078;
    if (!patch && pool_.Slot(surface)[kType]>=frames_.size()) throw std::out_of_range("noIsland postPop 타입 범위 오류");
    if (state_.loadingDepth) {
        // 1을 뺀 뒤 절삭한다. 소수 좌표의 칸을 먼저 절삭한 뒤 빼지 않는다.
        const double x=Coordinate(pool_,surface,kX),y=Coordinate(pool_,surface,kY);
        const auto west=SurfaceLetter(Cell(x-1.0),Cell(y));
        const auto north=SurfaceLetter(Cell(patch ? Coordinate(pool_,surface,kX) : x),Cell((patch ? Coordinate(pool_,surface,kY) : y)-1.0)); // PATCH는 서쪽 조회의 진단 복귀 뒤 현재 좌표를 다시 읽는다.
        const auto letter=NextLetter(west,north);
        const auto raw=pool_.Slot(surface);
        if (raw[kType]>=frames_.size()) throw std::out_of_range("noIsland 선택 프레임 타입 범위 오류");
        const int frame=frames_[raw[kType]].FindNumber(letter,kVariant,kFrameNumber);
        if (frame<0) {
            if (!patch) throw std::logic_error("noIsland 방향에 해당하는 첫 프레임이 없다");
            SurfaceNeed(hooks_.assertion)("0",R"(\Ns\O\Rifttype.cpp)",299); // 필수 검색은 진단 뒤에도 -1을 반환하며 다시 검색하지 않는다.
        }
        // 원본은 SetFrame을 호출하지 않고 프레임 필드를 직접 쓴다.
        WriteFrame(pool_,surface,frame);
        if (patch && Letter(surface)!=letter) SurfaceNeed(hooks_.assertion)("getOrientation() == orientation",R"(\Ns\O\Construction.cpp)",0xfa);
    }
    const std::size_t ownerOffset=pool_.Edition()==OriginalEdition::Patch1078 ? 34U : 32U;
    if (Letter(surface)=='F') {
        const Sid island=SurfaceNeed(hooks_.objects.create)(typeState_.islandType,kConnectCreateFlags);
        if (patch) {
            if (typeState_.islandType>=types_.size()) throw std::out_of_range("noIsland 생성 뒤 받침 타입 범위 오류");
            const auto& type=types_[typeState_.islandType]; // 생성 뒤 선택한 항목의 주소를 소유자 호출 동안 유지한다. 필드 값은 그 뒤에 읽는다.
            SurfaceNeed(hooks_.objects.setOwner)(island,pool_.Slot(surface)[ownerOffset]);
            SurfaceNeed(hooks_.objects.pop)(island,static_cast<float>(FootDelta(type.footX)+Coordinate(pool_,surface,kX)),
                static_cast<float>(FootDelta(type.footY)+Coordinate(pool_,surface,kY)),0);
        } else {
            hooks_.objects.setOwner(island,pool_.Slot(surface)[ownerOffset]);const auto& type=types_[typeState_.islandType];
            hooks_.objects.pop(island,static_cast<float>(static_cast<double>(Coordinate(pool_,surface,kX))+type.footX-1),
                static_cast<float>(static_cast<double>(Coordinate(pool_,surface,kY))+type.footY-1),0);
        }
    }
    if (!state_.SavedGame() && Letter(surface)=='H') {
        const float x=Coordinate(pool_,surface,kX),y=Coordinate(pool_,surface,kY);
        // 패치는 2를 뺀 float에 조회 함수가 0.9999를 더한다. CD는 x87에서 1.0001을 한 번 빼고 절삭한다.
        const auto hCell=[patch](float value) {
            return patch ? Cell(static_cast<double>(static_cast<float>(static_cast<double>(value)-2))+kSurfaceBias)
                : Cell(static_cast<double>(value)-kCdHOffset);
        };
        const Sid first=SurfaceAt(hCell(x),hCell(y),surfaceMap_);
        if (!first.value) {
            if (patch) SurfaceNeed(hooks_.assertion)("gSid",R"(\Ns\O\Construction.cpp)",0x10a);
        } else SetSupportOwner(x,y,pool_.Slot(first)[ownerOffset]);
    }
    SurfaceNeed(hooks_.connect)(surface);
}
void RawIslandPostPop::SetSupportOwner(float x,float y,std::uint32_t owner) const {
    const int right=Cell(x),bottom=Cell(y);
    // 진입 조건의 표면 조회는 0.9999를 더해 절삭하고 실제 3×3 반복 기준점은 보정 없는 절삭값이다.
    const Sid target=SurfaceAt(Cell(static_cast<double>(x)+kSurfaceBias),Cell(static_cast<double>(y)+kSurfaceBias),surfaceMap_);
    if (!target.value || pool_.Slot(target)[kType]!=typeState_.noIslandType) return;
    bool missingCells=false;std::string detail; // 고정 지도 한 칸의 소유자/표시 호출이 끝난 현재 TYPE로 진단 문자열을 누적한다.
    // 원본은 x를 바깥 반복문, y를 안쪽 반복문으로 순회한다.
    for (int cx=right-kSupportSize+1;cx<=right;++cx) {
        // 타입을 제한하지 않고 각 칸의 현재 머리 객체에 소유자와 표시 갱신을 준다.
        for (int cy=bottom-kSupportSize+1;cy<=bottom;++cy) {
            const Sid sid=SurfaceAt(cx,cy,ownerMap_);
            if (!sid.value) { missingCells=true;detail+='-';continue; }
            SurfaceNeed(hooks_.objects.setOwner)(sid,owner);SurfaceNeed(hooks_.updateDisplay)(sid);
            const auto type=pool_.Slot(sid)[kType];detail+=type==typeState_.noIslandType ? "X" : "["+std::to_string(type)+"]";
        }
    }
    const Sid island=SurfaceNeed(hooks_.findTypeAt)(x,y,typeState_.islandType);
    if (island.value) { SurfaceNeed(hooks_.objects.setOwner)(island,owner);SurfaceNeed(hooks_.updateDisplay)(island); }
    else detail+=" (No islandSid) ";
    const Sid stalag=SurfaceNeed(hooks_.findTypeAt)(x,y,typeState_.stalagType);
    if (stalag.value) { SurfaceNeed(hooks_.objects.setOwner)(stalag,owner);SurfaceNeed(hooks_.updateDisplay)(stalag); }
    else detail+=" (No islandStalagSid) ";
    if (!(missingCells || !island.value || !stalag.value)) return;
    if (pool_.Edition()!=OriginalEdition::Patch1078 || (!hooks_.detail && !hooks_.assertion)) {
        SurfaceNeed(hooks_.incompleteSupport)(missingCells,!island.value,!stalag.value);return; // 기존 독립 fixture의 하나인 D 경계를 보존한다. 실제 World는 아래 두 장치를 연결한다.
    }
    const auto typeWas=hooks_.typeWas ? SurfaceNeed(hooks_.typeWas)() : 0U;
    if (typeWas) detail+=" TypeWas "+std::to_string(std::bit_cast<std::int32_t>(typeWas));
    SurfaceNeed(hooks_.detail)(detail);
    SurfaceNeed(hooks_.assertion)("0",R"(\Ns\O\Damageable.cpp)",0x18b);
}
}
