// 강화 프레임이 없을 때도 원문은 플레어를 만들며 다른 프레임으로 대체하지 않는다.
#include "o/RawBombHardener.h"
#include "o/RawPathAnimation.h"
#include <bit>
#include <stdexcept>
#include <utility>
#include <vector>

namespace netstorm::o { // 현재 저장소를 빌리며 시전자/대상의 소유자·상태에 새 관문을 넣지 않는다.
RawBombHardener::RawBombHardener(SidPool& pool,std::span<const RiftTypeRecord> types,std::span<const RiftTypeFrames> frames,GameRandom& random,const SquidFrame& setter,std::function<void(Sid,std::int32_t,std::uint32_t)> flare)
    :pool_(pool),types_(types),frames_(frames),random_(random),setter_(setter),flare_(std::move(flare)) { // 별도 World/TYPE/난수 상태를 생성하지 않는다.
    if (pool.Edition()!=OriginalEdition::Patch1078 || types.size()!=188 || frames.size()!=188 || &setter.Pool()!=&pool) throw std::invalid_argument("하드너 실제 PATCH 풀/현재 TYPE/프레임 계약 오류");
}
std::uint32_t RawBombHardener::Read(Sid sid,std::size_t offset,std::size_t width) const { // 물리 슬롯의 지정 폭만 little endian으로 조합한다.
    const auto raw=pool_.Slot(sid);std::uint32_t result{};
    for (std::size_t byte=0;byte<width;++byte) result|=std::uint32_t(raw[offset+byte])<<(8*byte); // 이웃 필드와 free/dead 상태를 추가로 읽지 않는다.
    return result;
}
std::size_t RawBombHardener::Type(Sid sid) const { // 효과 뒤 현재 TYPE를 다시 읽을 수 있도록 번호를 고정하지 않는다.
    const auto type=Read(sid,10,1);if (type>=types_.size()) throw std::out_of_range("하드너 현재 TYPE 범위 오류");return type;
}
std::uint32_t RawBombHardener::MayTarget(Sid target) const { return types_[Type(target)].flags2&4U; } // 현재 HP/동맹/이미 강화됨을 자격에 추가하지 않는다.
std::int32_t RawBombHardener::ChooseFrame(Sid target) const { //실제 검색 함수의 반환 시 후보 메모리도 같은 수명으로 반납한다.
    const auto side=RawPathAnimation::Side(frames_[Type(target)],std::bit_cast<std::int32_t>(Read(target,36))); //004ad680의 현재 방향은 signed BYTE다.
    const auto codes=frames_[Type(target)].Codes();std::vector<std::int32_t> candidates;candidates.reserve(256); //0049aa90의 실제1024BYTE 후보 공간을 확보한다.
    for (std::size_t index=0;index<codes.size();++index) { // 첫256개만 보관하고 원문 표의 전체 순회/순서를 유지한다.
        if (codes[index].side==static_cast<std::uint8_t>(side) && codes[index].variant=='P' && (codes[index].flags&0x40U) && candidates.size()<256)
            candidates.push_back(static_cast<std::int32_t>(index));
    }
    return candidates.empty() ? -1:candidates[random_.Next(static_cast<std::uint32_t>(candidates.size()))]; //실패 시 난수를 소비하거나 wildcard 폴백을 하지 않는다.
}
void RawBombHardener::Apply(Sid parent,Sid target) const { // 선택된 실제 가상c0는 자격 검사를 반복하지 않는다.
    if (target.value==Read(parent,12,2)) return;
    const auto selected=ChooseFrame(target); //검색 배열을 해제한 뒤에만 실제 프레임/표시 갱신 효과를 실행한다.
    if (selected!=-1) setter_.Set(target,selected,0);
    if (!flare_) throw std::logic_error("하드너 실제 플레어 미연결");const auto call=flare_;call(target,1,2); //프레임 지정 뒤 같은 실제 부모/기본 방향 효과를 호출한다.
}
}
