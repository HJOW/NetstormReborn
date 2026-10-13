// 원본 miniMap dirty 큐의 실질 두 좌표와 판본별 카메라 비교를 보존한다.
#include "o/RawMetadisplayDirty.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 원본 bitmap 행 폭과 전체 갱신 셀 행/열 수다. 월드 256칸을 4칸씩 묶는다.
constexpr std::int32_t kBitmapWidth=128,kCellCount=64;
// 정렬되지 않은 raw 슬롯의 little endian float 비트를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t offset) {
    std::uint32_t value=0;
    // +14/+18의 네 BYTE만 조합하여 다른 필드를 읽지 않는다.
    for (std::size_t i=0;i<4;++i) value|=static_cast<std::uint32_t>(bytes[offset+i])<<(i*8);
    return value;
}
// 패치는 float 동등 비교, CD의 x87 C3 단독 검사는 unordered NaN도 같은 좌표로 간주한다.
bool SameCamera(OriginalEdition edition,const MetadisplayDirtyView& current,const MetadisplayDirtyView& cached) {
    // 두 좌표 모두 동등해야 한다. +0/-0은 두 판본에서 같은 좌표다.
    for (std::size_t i=0;i<current.camera.size();++i) {
        const auto a=std::bit_cast<float>(current.camera[i]),b=std::bit_cast<float>(cached.camera[i]);
        if (a==b) continue;
        if (edition==OriginalEdition::Cd1072 && (std::isnan(a) || std::isnan(b))) continue;
        return false;
    }
    return true;
}
}
// 표시 비활성/아직 없는 자연 반환 분기에서는 훅을 요구하지 않는다.
RawMetadisplayDirty::RawMetadisplayDirty(const SidPool& pool,MetadisplayDirtyState& state,MetadisplayDirtyHooks hooks)
    :pool_(pool),state_(state),hooks_(std::move(hooks)) { ValidateState(); }
// 배열 밖 원본 메모리 손상 입력은 정상 게임 경로가 아니므로 사전에 거부한다.
void RawMetadisplayDirty::ValidateState() const {
    if (state_.count<0 || state_.count>static_cast<std::int32_t>(state_.cells.size()))
        throw std::out_of_range("미니맵 dirty count 범위 오류");
    if (!state_.bitmap.empty() && (state_.bitmapWidth<=0 ||
        static_cast<std::uint64_t>(state_.bitmapWidth)*state_.bitmapWidth!=state_.bitmap.size()))
        throw std::invalid_argument("미니맵 bitmap 폭/크기 불일치");
}
// __ftol 결과를 signed /4하므로 -3.9는 0이고 -4.0은 -1이다. floor는 사용하지 않는다.
std::int32_t RawMetadisplayDirty::Cell(float coordinate) {
    const auto value=std::trunc(static_cast<double>(coordinate));
    if (!std::isfinite(value) || value<std::numeric_limits<std::int32_t>::min() ||
        value>std::numeric_limits<std::int32_t>::max()) throw std::out_of_range("미니맵 dirty 유한 signed32 좌표가 필요합니다");
    return static_cast<std::int32_t>(value)/4;
}
// 카메라 표시가 꺼져 있으면 큐와 포화 플래그를 보존하며 좌표 변환도 하지 않는다.
void RawMetadisplayDirty::Enqueue(float x,float y) const {
    if (!state_.displayPresent || (state_.displayFlags&1)==0) return;
    ValidateState();const auto cx=Cell(x),cy=Cell(y);
    // 같은 셀의 재통지는 먼저 검사하므로 꽉 찬 큐에서도 전체 갱신을 새로 요구하지 않는다.
    for (std::int32_t i=0;i<state_.count;++i)
        if (state_.cells[static_cast<std::size_t>(i)].x==cx && state_.cells[static_cast<std::size_t>(i)].y==cy) return;
    if (state_.count>=static_cast<std::int32_t>(state_.cells.size())) { state_.fullRefresh=1;return; }
    state_.cells[static_cast<std::size_t>(state_.count)]={cx,cy,0};++state_.count;
}
// 원본 상태를 복사한 별도 좌표 대신 그 시점의 슬롯을 읽는다.
void RawMetadisplayDirty::Enqueue(Sid sid) const {
    const auto raw=pool_.Slot(sid);
    Enqueue(std::bit_cast<float>(Read(raw,14)),std::bit_cast<float>(Read(raw,18)));
}
// 실제 Refresh는 비활성 표시에서도 bitmap을 최초 확보하고 full을 1로 쓴다.
void RawMetadisplayDirty::Refresh(const MetadisplayDirtyView& view) const {
    ValidateState();
    if (state_.bitmap.empty()) { state_.bitmapWidth=kBitmapWidth;state_.bitmap.assign(kBitmapWidth*kBitmapWidth,0);state_.fullRefresh=1; }
    if (!state_.displayPresent || (state_.displayFlags&1)==0) return;
    state_.initialized|=1;
    if ((state_.initialized&2)==0) { state_.initialized|=2;state_.cached.rectangle={}; }
    if (state_.count!=0 || state_.fullRefresh!=0 || !SameCamera(pool_.Edition(),view,state_.cached) || view.rectangle!=state_.cached.rectangle) {
        if (!hooks_.rebuild) throw std::logic_error("미니맵 표시 재생성 몸체가 연결되지 않았습니다");
        state_.cached=view;hooks_.rebuild();
    }
    ++state_.frames;
    if (state_.fullRefresh!=0) {
        if (!hooks_.redrawCell) throw std::logic_error("미니맵 전체 셀 그림 몸체가 연결되지 않았습니다");
        // 전체 갱신은 y를 먼저 증가시키며 모든 64×64 셀을 처리한다.
        for (std::int32_t y=0;y<kCellCount;++y) {
            // 새 큐 등록은 이 전체 순서를 바꾸지 않으며 마지막에 count를 0으로 만든다.
            for (std::int32_t x=0;x<kCellCount;++x) hooks_.redrawCell(x,y);
        }
    } else {
        if (state_.count!=0 && !hooks_.redrawCell) throw std::logic_error("미니맵 부분 셀 그림 몸체가 연결되지 않았습니다");
        // 셀 그림 callback 뒤 현재 count를 다시 읽으므로 그 callback의 신규 통지도 같은 프레임에 처리한다.
        for (std::int32_t i=0;i<state_.count;++i) {
            ValidateState();const auto cell=state_.cells[static_cast<std::size_t>(i)];hooks_.redrawCell(cell.x,cell.y);
        }
    }
    state_.count=0;state_.fullRefresh=0;
}
// 풀 주소 자체로 연결의 일치 여부를 확인한다.
const SidPool& RawMetadisplayDirty::Pool() const { return pool_; }
// 반환 훅은 dirty보다 오래 살아서는 안 된다. 게임별 전투/로딩 판정을 덧붙이지 않는다.
std::function<void(float,float)> MakeMetadisplayDirtyHook(const SidPool& pool,RawMetadisplayDirty& dirty) {
    if (&pool!=&dirty.Pool()) throw std::invalid_argument("미니맵 dirty SID 풀이 다릅니다");
    return [&dirty](float x,float y) { dirty.Enqueue(x,y); };
}
}
