// 원본 수평/수직 선분의 보정 지점과 현재 프레임 방향을 유지한다.
#include "o/RawFenceLine.h"
#include <bit>
#include <cmath>
#include <stdexcept>
#include <utility>

namespace netstorm::o {
namespace {
// 현재 네 바이트를 little endian DWORD로 읽는다.
std::uint32_t FenceLineRead(std::span<const std::uint8_t> raw,std::size_t offset) {
    std::uint32_t value{}; // 원본 비정렬 필드의 결합 결과다.
    for (std::size_t index=0;index<4;++index) value|=std::uint32_t(raw[offset+index])<<(index*8); // 이웃 바이트는 읽지 않는다.
    return value;
}
// 유효 CRT 입력에서 signed64 절삭의 낮은DWORD를 반환한다.
std::int32_t FenceLineInt(float value) {
    const auto wide=static_cast<double>(value);if (!std::isfinite(wide) || wide<-9223372036854775808.0 || wide>=9223372036854775808.0) throw std::out_of_range("펜스 연결선 CRT 범위 오류");
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(wide)));
}
}
// CD의 다른 연결선과 표 크기를 PATCH로 취급하지 않는다.
RawFenceLine::RawFenceLine(SidPool& pool,const SquidHash& hash,std::span<const RiftTypeRecord> types,
    std::span<const RiftTypeFrames> frames,const std::uint32_t& markType,std::function<void(Sid,std::uint32_t)> destroy)
    :pool_(pool),hash_(hash),types_(types),frames_(frames),markType_(markType),destroy_(std::move(destroy)) {
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188) throw std::invalid_argument("펜스 연결선 PATCH 표 크기 오류");
}
// mode0도 동일한 좌표 보정과100개 검색 상한을 사용한다. 끝점은 한 번 포함한다.
std::vector<std::array<std::int32_t,2>> RawFenceLine::SearchPoints(std::array<float,4> endpoints) {
    auto x=endpoints[0]-1.0f,y=endpoints[1]-1.0f,endX=endpoints[2]-1.0f,endY=endpoints[3]-1.0f;float dx{},dy{}; // 원본 스택에 저장되는 현재float 지점이다.
    if (x==endX) {
        if (endY<=y) { dy=-1;y-=3;if (y<endY) endY=y; }
        else { dy=1;endY-=3;if (endY<=y) y=endY; }
        if (x<=endX) x=endX;
        if (dy>0) { if (endY<=y) y=endY; }
        else if (y<=endY) y=endY;
    } else {
        if (endX<=x) { x+=1;dx=-1;endX-=1; }
        else { x-=1;dx=1;endX+=1; }
        y-=1;endY-=1;x=dx+dx+x;endX-=dx+dx;
        if (dx<=0) { if (x<=endX) x=endX; }
        else if (endX<=x) x=endX;
        if (y<=endY) y=endY;
    }
    std::vector<std::array<std::int32_t,2>> result; // 실제 mode0이 원본 finder에 넘긴 정수 좌표 순서다.
    for (std::uint32_t remaining=100;remaining;--remaining) { // 원본100회 상한을 유지하고 끝점 처리 뒤 종료한다.
        result.push_back({FenceLineInt(x),FenceLineInt(y)});if (x==endX && y==endY) break;x+=dx;y+=dy;
    }
    return result;
}
// 삭제 효과 뒤에도 동일 finder의 미리 보관한 next와 다음 현재 버킷을 사용한다.
void RawFenceLine::Clear(std::array<float,4> endpoints) const {
    const auto direction=endpoints[0]==endpoints[2] ? 0x4a : 0x4b; // 실제 첫 코드J는 수직, K는 수평이다.
    for (const auto& point:SearchPoints(endpoints)) { // 원본 보정 지점을 순서대로 검색한다.
        RawSquidFinder finder(pool_,hash_,types_);
        for (auto sid=finder.Begin({point[0],point[1],point[0],point[1]},0);sid.value;sid=finder.Next()) { // 표면도 포함하는 실제 일반 탐색이다.
            const auto raw=pool_.Slot(sid);if (raw[10]!=markType_) continue;const auto type=raw[10];
            const auto frame=std::bit_cast<std::int32_t>(FenceLineRead(raw,36));const auto codes=frames_[type].Codes(); // 현재 실제 프레임을 가상 getter 위치에서 읽는다.
            if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("펜스 연결선 현재 프레임 범위 오류");
            if (static_cast<std::int8_t>(codes[static_cast<std::size_t>(frame)].side)!=direction) continue;
            if (!destroy_) throw std::logic_error("펜스 연결선 실제 가상 삭제 미연결");destroy_(sid,0x40);
        }
    }
}
}
