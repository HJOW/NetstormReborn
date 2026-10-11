// 00433660 ↔ CD 004887a0: 일반 해시 체인과 본체/그림자 프레임의 판본별 월드 교차다.
#include "client/ObjectCollection.h"
#include <algorithm>
#include <bit>
#include <cmath>
#include <stdexcept>
#include <unordered_set>

namespace netstorm::client {
namespace {
// 타입의 abstract/그림자와 walker 분류 비트, 월드 경계 여유 및 그림자 표시 y 보정이다.
constexpr std::uint32_t kAbstract=0x400000,kShadow=0x40000,kWalker=0x10000;
constexpr double kPadding=1.0,kWalkerBias=0.5;
constexpr float kShadowBias=0.1f;
// raw 슬롯의 unaligned little endian 필드를 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> raw,std::size_t offset,std::size_t width=4) {
    std::uint32_t value=0;
    // 요청한 바이트 폭만 읽어 CD 프레임 옆 extra를 섞지 않는다.
    for (std::size_t i=0;i<width;++i) value|=static_cast<std::uint32_t>(raw[offset+i])<<(8*i);
    return value;
}
}
// 표 길이/토큰 범위를 검증하고 기존 raw 풀과 타입/프레임 저장소를 참조한다.
ObjectCollection::ObjectCollection(o::SquidHash& hash,const o::SidPool& pool,std::span<const o::RiftTypeRecord> types,
    std::span<const o::SquidDisplayShape> shapes,std::uint32_t tokenBase)
    :hash_(hash),pool_(pool),types_(types),shapes_(shapes),tokenBase_(tokenBase) {
    if (types.empty() || types.size()!=shapes.size()) throw std::invalid_argument("Object collection type/shape tables");
    if (static_cast<std::uint64_t>(tokenBase)+static_cast<std::uint64_t>(pool.Capacity())*pool.Layout().stride>0x100000000ULL)
        throw std::invalid_argument("Object collection token range");
}
// 패치판은 논리 count/해제 흔적도 검사한다. CD판은 실제 물리 프레임 경계만 안전하게 검증한다.
const o::SquidDisplayFrame& ObjectCollection::Frame(std::uint32_t type,std::int32_t frame) const {
    const auto& record=types_[type];const auto& shape=shapes_[type];
    if (!shape.loaded || frame<0 || static_cast<std::size_t>(frame)>=shape.frames.size()) throw std::out_of_range("Object collection physical frame");
    if (pool_.Edition()==o::OriginalEdition::Patch1078) {
        const auto count=static_cast<std::int64_t>(shape.frameCount)*((record.flags1&(kAbstract|kShadow)) ? 2 : 1);
        if (frame>=count || record.maxHitPoints==-0x22222223 || record.maxHitPoints==-0x32323233)
            throw std::out_of_range("Object collection frameCheck");
    }
    return shape.frames[static_cast<std::size_t>(frame)];
}
// 원본 정수 절삭과 signed idiv를 사용한 뒤 현재 단계의 상단 칸으로 제한한다.
int ObjectCollection::Cell(float coordinate,int level) {
    const double value=static_cast<double>(coordinate)+0.5;std::int64_t integer=0;
    if (std::isfinite(value) && value>=-0x1p63 && value<0x1p63) integer=static_cast<std::int64_t>(value);
    const auto cell=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(integer))/o::SquidHash::kScales[static_cast<std::size_t>(level)];
    return std::min(cell,o::SquidHash::Side(level)-1);
}
// 패치의 x 차이는 x87에 남고 y 차이는 float로 저장된다. CD는 차이 둘과 top 경계를 float로 저장한다.
bool ObjectCollection::Visible(o::RenderWorldRect view,float x,float y,const o::SquidDisplayFrame& frame) const {
    const double dx=static_cast<double>(x)-frame.cellHotspotX;
    const double dy=static_cast<float>(static_cast<double>(y)-frame.cellHotspotY);
    const double left=static_cast<double>(view.left)-frame.cellWidth-kPadding;
    const double top=static_cast<double>(view.top)-frame.cellHeight-kPadding;
    if (pool_.Edition()==o::OriginalEdition::Patch1078)
        return left<=dx && dx<=view.right && top<=dy && dy<=view.bottom;
    const double cdX=static_cast<float>(dx),cdTop=static_cast<float>(top);
    return !(left>cdX) && !(cdX>view.right) && dy>=cdTop && !(dy>view.bottom);
}
// 현재 SID를 본체/그림자 순서로 제출하며 next는 판정이 끝난 후 현재 raw 슬롯에서 다시 읽는다.
std::int32_t ObjectCollection::Collect(o::RenderWorldRect view,std::span<o::RenderCollectionEntry> output,std::int32_t limit) const {
    const bool patch=pool_.Edition()==o::OriginalEdition::Patch1078;std::int32_t count=0;
    // 화면/해시 단계 범위만 방문한다. 다른 칸의 반복 SID를 전역 중복 제거하지 않는다.
    for (int level=1;level<4;++level) {
        hash_.Select(level);const int side=o::SquidHash::Side(level);
        // 원본 DWORD 한 칸 확장과 각 끝의 독립 클램프를 signed overflow 없이 재현한다.
        const auto expand=[side](int cell,std::uint32_t step) { return std::clamp(std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(cell)+step),0,side-1); };
        const int left=expand(Cell(view.left,level),0xffffffffU),right=expand(Cell(view.right,level),1);
        const int top=expand(Cell(view.top,level),0xffffffffU),bottom=expand(Cell(view.bottom,level),1);
        // y 바깥, x 안쪽 순서이며 빈 칸은 체인 조회를 생략한다.
        for (int y=top;y<=bottom;++y) {
            // 한 칸의 next 체인을 끝까지 방문한 뒤 다음 x 칸으로 이동한다.
            for (int x=left;x<=right;++x) {
                o::Sid sid{hash_.At(x,y)};std::unordered_set<std::uint16_t> chain;
                // 같은 체인 안의 순환만 거부한다. 풀 범위/type/frame은 기록 전에 검사한다.
                while (sid.value) {
                    if (!chain.insert(sid.value).second) throw std::logic_error("Object collection SID cycle");
                    const auto raw=pool_.Slot(sid);const auto type=raw[10];
                    if (!type || type>=types_.size()) throw std::out_of_range("Object collection nonzero type");
                    const auto& record=types_[type];const auto frame=patch ? std::bit_cast<std::int32_t>(Read(raw,36)) : raw[34];
                    const float positionX=std::bit_cast<float>(Read(raw,14)),positionY=std::bit_cast<float>(Read(raw,18));
                    // 수집되는 레이어만 기록하고 원본 strict limit은 기록 후 검사한다. 실제 span 넘침은 쓰기 전에 거부한다.
                    const auto append=[&](bool shadow) {
                        if (static_cast<std::size_t>(count)>=output.size()) throw std::length_error("Object collection output capacity");
                        const auto current=pool_.Slot(sid);auto& entry=output[static_cast<std::size_t>(count)];
                        entry.x=std::bit_cast<float>(Read(current,14));entry.y=std::bit_cast<float>(Read(current,18));
                        if (shadow) entry.y=static_cast<float>(static_cast<double>(entry.y)-kShadowBias);
                        else if (record.flags2&kWalker) entry.y=static_cast<float>(static_cast<double>(entry.y)+kWalkerBias);
                        entry.token=tokenBase_+sid.value*pool_.Layout().stride;
                        entry.depth=std::bit_cast<std::int8_t>(current[patch ? 35 : 33]);entry.kind=0;
                        entry.tail=shadow || ((record.flags1&kAbstract) && frame>=shapes_[type].frameCount) ? 4 : 0;
                        ++count;if (count>=limit) throw std::length_error("Object collection count < limit");
                    };
                    if (Visible(view,positionX,positionY,Frame(type,frame))) append(false);
                    if (record.flags1&kShadow) {
                        const auto shadowFrame=std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(frame)+static_cast<std::uint32_t>(shapes_[type].frameCount));
                        if (Visible(view,positionX,positionY,Frame(type,shadowFrame))) append(true);
                    }
                    sid={static_cast<std::uint16_t>(Read(pool_.Slot(sid),4,2))};
                }
            }
        }
    }
    hash_.Select(2);return count;
}
// 원본 tokenBase/stride와 슬롯 경계를 검사해 같은 풀의 번호만 반환한다.
o::Sid ObjectCollection::Object(std::uint32_t token) const {
    if (token<tokenBase_) throw std::out_of_range("Object collection token");
    const auto offset=token-tokenBase_,stride=pool_.Layout().stride;
    if (offset%stride || !offset || offset/stride>=pool_.Capacity()) throw std::out_of_range("Object collection token SID");
    return {static_cast<std::uint16_t>(offset/stride)};
}
// 가시 월드 범위와 공통 작업 배열의 잔여 공간을 실제 자산 수집기에 넘긴다.
void BindObjectCollection(DrawCollectionHooks& hooks,const ObjectCollection& collector) {
    hooks.objects=[&collector](auto view,auto output,auto limit) { return collector.Collect(view,output,limit); };
}
}
