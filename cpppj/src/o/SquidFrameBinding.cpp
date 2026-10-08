// 제자리 경로는 old/new Update, 단계 이동 경로는 실제 Unpop/Pop과 그 안의 Update를 사용한다.
#include "o/SquidFrameBinding.h"
#include <bit>
#include <stdexcept>

namespace netstorm::o {
namespace {
// 패치 DWORD/CD BYTE의 현재 프레임을 읽는다. CD extra 바이트를 섞지 않는다.
std::int32_t CurrentFrame(const SidPool& pool,Sid sid) {
    const auto raw=pool.Slot(sid);
    if (pool.Edition()!=OriginalEdition::Patch1078) return raw[34];
    std::uint32_t bits=0;
    // 패치 프레임은 little endian 네 바이트다.
    for (std::size_t i=0;i<4;++i) bits|=static_cast<std::uint32_t>(raw[36+i])<<(8*i);
    return std::bit_cast<std::int32_t>(bits);
}
}
// callback 생성 전에 판본·타입·풀·표시 연결을 확인한다. 원본 주소를 호스트에서 호출하지 않는다.
SquidFrameHooks MakeSquidFrameHooks(SidPool& pool,std::span<const RiftTypeRecord> types,
    SquidDisplay& display,SquidUnpop& unpop,SquidPop& pop) {
    display.ValidateFrameBinding(pool.Edition(),types);pop.ValidateFrameBinding(pool,display,unpop);
    return {
        // 현재 프레임의 실제 SHP 칸 크기가 다음 해시 단계 판단에 쓰인다.
        [&pool,&display](Sid sid,std::int32_t frame) { return display.FrameSize(pool.Edition(),pool.Slot(sid),frame); },
        // 가상 +0x88/+0x8c 공통 경로를 실제 변경 영역 계산기로 이어 준다.
        [&pool,&display](Sid sid,std::uint32_t flags) { display.Update(pool.Slot(sid),flags); },
        // 옛 프레임으로 공간과 옛 표시 영역을 해제한다.
        [&pool,types,&unpop](Sid sid,std::uint32_t flags) {
            const auto number=pool.Slot(sid)[10];
            if (number>=types.size()) throw std::out_of_range("프레임 Unpop 타입 번호 범위 오류");
            unpop.Unpop(sid,types[number],flags);
        },
        // Unpop 뒤 현재 프레임의 SHP 크기로 공간과 새 표시 영역을 등록한다.
        [&pool,types,&display,&pop](Sid sid,float x,float y,std::uint32_t flags) {
            const auto number=pool.Slot(sid)[10];
            if (number>=types.size()) throw std::out_of_range("프레임 Pop 타입 번호 범위 오류");
            const auto size=display.FrameSize(pool.Edition(),pool.Slot(sid),CurrentFrame(pool,sid));
            static_cast<void>(pop.Pop(sid,types[number],size[0],size[1],x,y,flags));
        }};
}
}
