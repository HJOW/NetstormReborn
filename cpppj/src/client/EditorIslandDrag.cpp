// 각 원문 하위 호출 뒤 변경될 수 있는 목록/drag 필드를 재조회한다.
#include "client/EditorIslandDrag.h"
#include <bit>
#include <cmath>
#include <limits>
#include <stdexcept>

namespace netstorm::client { //이 모듈은 실제 bitmap이나 전용 Gump의 성공 대체물을 만들지 않는다.
namespace { //원문 사각형/ftol의 작은 수치 몸체만 소유한다.
//004239c0은 right/bottom을 제외한다.
bool Inside(const std::array<std::int32_t,4>& b,std::int32_t x,std::int32_t y) { return b[0]<=x && x<b[2] && b[1]<=y && y<b[3]; }
//__ftol은 x87에서 event정수−float크기×0.5를 계산한 뒤 int64로 절삭한다.
std::int32_t Origin(std::int32_t point,float size) {
    const auto value=static_cast<double>(point)-static_cast<double>(size)*0.5;
    if (!std::isfinite(value) || value>=9223372036854775808.0 || value< -9223372036854775808.0) return 0;
    return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(static_cast<std::int64_t>(value)));
}
//원문 SUB는 DWORD 감김이며 signed /16은0방향 절삭이다.
std::int32_t Chunk(std::int32_t value,std::int32_t offset) { return std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(value)-static_cast<std::uint32_t>(offset))/16; }
}
bool HandleEditorIslandDrag(EditorIslandDragState& s,const InputEvent& e,const EditorIslandDragHooks& h) {
    auto& flags=s.sharedFlags ? *s.sharedFlags : s.flags; // 가상 Kill/Push가 바꾼 같은 원본+4를 덮어쓰지 않는다.
    const auto code=e.code&0x03ffffffU;
    if (code==0x0200002cU || code==0x0200002fU) { if (std::bit_cast<std::uint32_t>(e.x)==h.closeSerial()) h.killParent();return true; }
    if (code==0x02000004U) {
        if (e.y) {
            if (s.drag) h.assertion("dragIsland == NULL");
            const auto image=h.image(e.x);
            if (image) {
                s.drag=h.createDrag(image);if (!s.drag) h.assertion("dragIsland != NULL");
                s.sourceIndex=h.recordIndex(h.selection());
                if (s.sourceIndex<0 || s.sourceIndex>=21) throw std::out_of_range("Add Island 실제21행 레코드의 물리 범위");
                const auto offset=static_cast<std::size_t>(s.sourceIndex)*6U;
                for (std::size_t byte=0;byte<6;++byte) s.record[byte]=s.choices[offset+byte]; //원문DWORD+WORD 복사와 같은6BYTE다.
                h.adjustDrag(s.drag);static_cast<void>(h.parentBounds());
                const auto size=h.size(s.drag);h.attachDrag(s.drag,static_cast<float>(Origin(e.x,size[0])),static_cast<float>(Origin(e.y,size[1])));
                flags|=0x100U;h.pushParent();flags|=0x200U;
            }
        }
        return true;
    }
    if (code==0x02000003U) s.dirty=1;
    if (code!=0x10000U || (e.code&InputCode::kRelease) || !s.drag) return false;
    if (Inside(h.listBounds(),e.x,e.y)) {
        flags&=~0x300U;h.killDrag(s.drag);s.drag=0;h.clearSelectionAndUpdate();return true;
    }
    const auto bounds=h.mapBounds();if (!Inside(bounds,e.x,e.y)) return false;
    const auto size=h.size(s.drag);const auto x=Chunk(Origin(e.x,size[0]),bounds[0]),y=Chunk(Origin(e.y,size[1]),bounds[1]);
    if (x<0 || x>13 || y<0 || y>13) return true;
    s.record[2]=static_cast<std::uint8_t>((y<<4)|(x&15));const auto map=h.mapSize();
    if (!h.drop(s.record)) return false;
    h.camera(static_cast<float>(map[0]*16)*0.5F,static_cast<float>(map[1]*16)*0.5F);
    flags&=~0x300U;h.killDrag(s.drag);s.drag=0;
    const auto image=h.image(h.selection());if (image) h.releaseImage(image);
    h.remove(h.selection());h.clearSelectionAndUpdate();s.dirty=1;h.incrementRevision();return true;
}
}
