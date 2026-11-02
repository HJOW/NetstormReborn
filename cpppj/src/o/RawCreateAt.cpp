// 원문 signed 관문·owner 재조회·직접 HP/frame 쓰기와 float32 Pop 저장을 유지한다.
#include "o/RawCreateAt.h"
#include <bit>
#include <new>
#include <stdexcept>

namespace netstorm::o { //기존 실제 World/Factory/Owner/HP/Pop을 외부 장치로 빌리는 원문 부모다.
namespace { //도달한 실제 하위의 누락만 선택 위치에서 거부한다.
template<class T> const T& Need(const T& value) { if (!value) throw std::logic_error("CreateAt의 실제 현재 부모 미연결");return value; } //실패를 가짜 SID/owner/프레임으로 바꾸지 않는다.
}
//권한 진단/같은 parser/세 signed 양수 관문 뒤에만 실제 생성·현재 후처리를 수행한다.
void CreateObjectAt(std::string_view source,const CreateAtHooks& hooks) {
    if (!Need(hooks.boss)()) { Need(hooks.assertion)(803);if (!Need(hooks.boss)()) return; }
    const auto text=source.substr(0,source.find('\0'));if (text.size()>=256) throw std::length_error("CreateAt 위치 parser의 원문256BYTE 지역 버퍼 범위");
    const auto location=Need(hooks.parse)(text);if (location.x<=0 || location.y<=0 || std::bit_cast<std::int32_t>(location.type)<=0) return;
    const auto created=Need(hooks.create)(location.type,0);auto owner=location.owner;
    if (!owner) { const auto terrain=Need(hooks.surface)(location.x,location.y);if (terrain) { const auto slot=Need(hooks.slot)(terrain);if (slot.size()<=34) throw std::out_of_range("CreateAt의 실제 지면 owner BYTE 범위");owner=slot[34]; } } //지역/로컬 소유자를 새로 추론하지 않고 현재 지면 BYTE만 사용한다.
    if (!created) throw std::bad_alloc(); //원문의 NULL 생성 객체 가상74 역참조를 호스트에서 명시 거부하되 선행 지면 조회는 보존한다.
    Need(hooks.owner)(created,owner);
    if (Need(hooks.hasHitPoints)(created)) { const auto maximum=Need(hooks.maximumHitPoints)(created);Need(hooks.hitPoints)(created,maximum); }
    if (Need(hooks.directional)(location.type)) {
        const auto half=location.radius/2;const auto offset=0x15cU+static_cast<std::uint32_t>(half)*4U; //원문 signed0방향 나눗셈 뒤 LEA의DWORD 감김이다. owner 인자가 아니다.
        const auto value=Need(hooks.frameField)(created,offset);Need(hooks.frame)(created,value);
    }
    Need(hooks.pop)(created,static_cast<float>(location.x),static_cast<float>(location.y),0); //각 축은FILD signed32→FSTP32 한 번이며 x/y를 뒤집지 않는다.
}
//원문 TYPE+15c 기준 인덱스가 첫 프레임/개수/종류 필드로 넘어가는 정상 물리도 보존한다.
std::uint32_t CreateAtFrameField(const RiftTypeFrames& frames,std::uint32_t offset) {
    if (offset==0x12cU) return static_cast<std::uint32_t>(frames.LetterKinds());
    if (offset>=0x130U && offset<0x170U && !(offset&3U)) return std::bit_cast<std::uint32_t>(frames.Run(static_cast<std::uint8_t>('A'+(offset-0x130U)/4U)).first);
    if (offset>=0x170U && offset<0x1b0U && !(offset&3U)) return static_cast<std::uint32_t>(frames.Run(static_cast<std::uint8_t>('A'+(offset-0x170U)/4U)).count);
    throw std::out_of_range("CreateAt frame이 참조한 TYPE의 이웃 필드/포인터 물리 미복원"); //실제 raw TYPE500BYTE 전체와32BIT 포인터 ABI를 합성하지 않는다. 앞선 생성/Owner/HP는 남는다.
}
}
