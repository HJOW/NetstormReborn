// 전체 다리 품질 명령은 새 객체/해시 재등록/저장을 만들지 않고 원문의 직접 쓰기만 수행한다.
#include "o/RawBridgeEditor.h"
#include "o/Bridge.h"
#include <bit>
#include <stdexcept>
#include <utility>

namespace netstorm::o { //원문 PATCH의 풀 순회와 하위 프레임 변환을 제공한다.
namespace { //현재 선택된 외부 효과의 계약만 공통 검사한다.
// 도달한 효과가 미연결이면 앞선 실제 쓰기를 유지하고 그 경계에서 중단한다.
template<class Function> const Function& Need(const Function& value) { if (!value) throw std::logic_error("편집 다리의 실제 효과 미연결");return value; }
}
// CD의 다른 슬롯/함수 경로를 PATCH의 복원으로 합산하지 않는다.
RawBridgeEditor::RawBridgeEditor(SidPool& pool,const std::uint32_t& bridgeType,const std::int32_t& crackLife,GameRandom& random,BridgeEditorHooks hooks)
    :pool_(pool),bridgeType_(bridgeType),crackLife_(crackLife),random_(random),hooks_(std::move(hooks)) {
    if (pool.Edition()!=OriginalEdition::Patch1078) throw std::invalid_argument("편집 다리 명령은 10.78 원문 경로만 지원");
}
// 초기 iterator(1)의 선행 증가부터 같은 실제 free 비트와 현재 TYPE을 읽는다.
void RawBridgeEditor::SetAll(std::uint32_t quality) const {
    const auto end=pool_.Capacity(); //원문 iterator+4는 시작 시 개수를 보관하며 현재 풀의 물리 범위도 매 방문 확인한다.
    std::uint32_t index=2; //초기 iterator1을 원문0040eb40에서 한 번 증가시킨 값이다.
    while (index<end && index<pool_.Capacity() && (pool_.Slot(Sid{static_cast<std::uint16_t>(index)})[11]&1U)) ++index; //초기 증가도 free만 건너뛴다.
    while (index<pool_.Capacity()) { //effect 중 실제 풀이 커지면 원문처럼 처음 end 뒤에도 현재 물리 끝까지 방문한다.
        const Sid sid{static_cast<std::uint16_t>(index)};const auto raw=pool_.Slot(sid);
        if (raw[10]==bridgeType_) Change(sid,quality);
        ++index;
        while (index<end && index<pool_.Capacity() && (pool_.Slot(Sid{static_cast<std::uint16_t>(index)})[11]&1U)) ++index; //보관한 end 뒤에는 free 건너뛰기를 실행하지 않는 원문 iterator의 의미를 유지한다.
    }
}
// 원문421f20은 Editor가 아니면 SID를 읽지 않는다. free/dead·소유자 관문은 추가하지 않는다.
void RawBridgeEditor::SetOne(std::uint32_t editor,std::uint32_t sid,std::uint32_t quality) const {
    if (!editor) return;
    if (sid>=pool_.Capacity()) throw std::out_of_range("개별 편집 다리의 원문 물리 SID 범위");
    const Sid object{static_cast<std::uint16_t>(sid)}; // 범위를 확인한 현재 실제 슬롯만 빌린다.
    if (Bytes(object)[10]==bridgeType_) Change(object,quality);
}
// 원문의 주소 산술처럼 예약 SID도 물리 범위 안이면 직접 접근하며 벡터 재확보 뒤 매번 다시 빌린다.
std::span<std::uint8_t> RawBridgeEditor::Bytes(Sid sid) const {
    const auto offset=pool_.Offset(sid);return std::span<std::uint8_t>(pool_.bytes_).subspan(offset,pool_.Layout().stride);
}
// 00421db0/00421e60/00421eb0의 수명 쓰기와 +88 순서를 같은 물리에 적용한다.
void RawBridgeEditor::Change(Sid sid,std::uint32_t quality) const {
    if (quality>2) return;
    auto raw=Bytes(sid); //일반 할당의 예약/상태 관문을 추가하지 않고 현재 원문 물리에 쓴다.
    const auto life=quality==0 ? ((static_cast<std::uint32_t>(crackLife_)*8U-8U)&0x78U) : 0U;
    raw[12]=static_cast<std::uint8_t>((raw[12]&0x87U)|life);Need(hooks_.notifySurface)(sid);
    const auto current=pool_.Slot(sid);const auto& frames=Need(hooks_.frames)(current[10]);
    const auto frame=std::bit_cast<std::int32_t>(std::uint32_t(current[36])|(std::uint32_t(current[37])<<8)|(std::uint32_t(current[38])<<16)|(std::uint32_t(current[39])<<24));
    std::int32_t selected=-1; //검색 실패여도 앞선 수명 쓰기는 보존하고 표시 갱신은 호출하지 않는다.
    if (quality==0) selected=Bridge::CrackedFrame(frames,frame,true);
    else if (quality==1) selected=Bridge::NormalFrame(frames,frame);
    else {
        const auto codes=frames.Codes();if (frame<0 || static_cast<std::size_t>(frame)>=codes.size()) throw std::out_of_range("편집 다리의 현재 프레임 범위");
        std::vector<std::int32_t> candidates; //0049aa90의 최대256개 후보만 저장하며 다수여도 같은 순서로 첫256개를 사용한다.
        for (std::size_t index=0;index<codes.size();++index) { //Hard는 현재 방향/변형P/마스크40의 실제 표 순서로 후보를 모은다.
            if (codes[index].side==codes[static_cast<std::size_t>(frame)].side && codes[index].variant=='P' && (codes[index].flags&0x40U) && candidates.size()<256)
                candidates.push_back(static_cast<std::int32_t>(index));
        }
        if (!candidates.empty()) selected=candidates[random_.Next(static_cast<std::uint32_t>(candidates.size()))];
    }
    if (selected==-1) return;
    raw=Bytes(sid);
    for (std::size_t byte=0;byte<4;++byte) raw[36+byte]=static_cast<std::uint8_t>(static_cast<std::uint32_t>(selected)>>(byte*8)); //원문의 직접 DWORD 쓰기이므로 SquidFrame.Set/Unpop/Pop을 부르지 않는다.
    Need(hooks_.refresh)(sid);
}
}
