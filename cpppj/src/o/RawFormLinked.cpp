//타입/상태 필터나 첫 자기 발견의 조기 성공을 추가하지 않는다.
#include "o/RawFormLinked.h"
#include <stdexcept>
#include <unordered_set>

namespace netstorm::o { //같은 현재 부모 head/종속 next WORD를 직접 읽는다.
namespace { //원문의 비정렬 WORD를 호스트 포인터와 구별한다.
std::uint16_t Word(std::span<const std::uint8_t> raw,std::size_t offset) { return static_cast<std::uint16_t>(raw[offset]|(std::uint16_t(raw[offset+1])<<8)); } //지정한 두 바이트만 읽는다.
}
Sid ReadRawProcessFormParent(const SidPool& pool,Sid form) { //Patch4ad010/CD4b04a0의 실제5/7BYTE getter를 같은 현재 슬롯에 적용한다.
    const auto raw=pool.Slot(form);const auto vtable=std::uint32_t(Word(raw,0))|(std::uint32_t(Word(raw,2))<<16);
    const auto expected=pool.Edition()==OriginalEdition::Patch1078 ? 0x512098U : 0x505990U; //양판본 실제 ProcessForm 생성자 가상 표 토큰이다.
    if (vtable!=expected) throw std::logic_error("Form 진단의 미복원 부모 가상 getter");return Sid{Word(raw,16)};
}
bool CheckRawFormChain(std::span<const std::uint8_t> bytes,std::uint32_t stride,std::uint32_t capacity,Sid form,Sid parent) { //부모 조회가 끝난 시점의 현재 물리를 사용하며 raw 타입/상태를 추가로 검사하지 않는다.
    if (stride<8 || capacity>65535 || capacity>bytes.size()/stride) throw std::out_of_range("Form 연결 검사의 실제 풀 물리 범위");
    const auto slot=[&](Sid sid) { if (sid.value>=bytes.size()/stride) throw std::out_of_range("Form 연결 검사의 부모 물리 범위");return bytes.subspan(static_cast<std::size_t>(sid.value)*stride,stride); }; //논리 범위 검사는 종속 방문 전에만 수행하고 부모의 선택된 물리는 별도로 확인한다.
    auto current=Sid{Word(slot(parent),6)};bool found=false;std::unordered_set<std::uint16_t> visited;
    while (current.value) { //자기를 발견해도 나머지 체인의 범위/끝을 검사한다.
        if (current.value>=capacity) return false;
        if (!visited.insert(current.value).second) throw std::logic_error("Form 연결 검사의 원문 순환 호스트 중단");
        if (current==form) found=true;current=Sid{Word(slot(current),4)};
    }
    return found;
}
bool CheckRawFormLinked(const SidPool& pool,Sid form,const std::function<Sid(Sid)>& parent) { //가상 부모 조회 뒤 현재 풀의 span을 새로 빌려 재확보 전 사본을 보관하지 않는다.
    if (!parent) throw std::logic_error("Form 연결 검사의 현재 부모 가상 장치 미연결");const auto selected=parent(form);
    return CheckRawFormChain(pool.Bytes(),pool.Layout().stride,pool.Capacity(),form,selected);
}
}
