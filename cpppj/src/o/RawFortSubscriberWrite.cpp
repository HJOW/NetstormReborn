// helper가 global ID/이름포인터/manager를 바꾸어도 원문이 캡처·재조회하는 경계만 보존한다.
#include "o/RawFortSubscriberWrite.h"
#include "o/RawFortMissionSave.h"
#include <array>
#include <stdexcept>

namespace netstorm::o { //게임 상태나 임시 Fort 이미지를 대신 만들지 않는 원문 저장 부모다.
namespace { //선택한 실제 하위만 필요한 순간에 요구한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("Fort Subscriber 저장의 실제 하위 미연결");return function; } //앞선 진단/일반 쓰기 효과를 rollback하지 않는다.
}
void SaveFortSubscriber(std::uint32_t slot,std::uint32_t capturedId,std::optional<std::span<const std::uint8_t>> capturedHandle,const FortSubscriberWriteHooks& hooks) {
    if (!Need(hooks.manager)(slot)) Need(hooks.assertion)(0x1cfU);
    Need(hooks.registerSubscriber)();const auto manager=Need(hooks.manager)(slot),stream=Need(hooks.create)(manager,"Subscriber"); //등록 helper 뒤의 현재 manager만 Create에서 선택한다.
    std::array<std::uint8_t,4> id{};for (std::size_t byte=0;byte<id.size();++byte) id[byte]=static_cast<std::uint8_t>(capturedId>>(8*byte)); //global의 현재 재등록값과 별개인 부모 인자4BYTE다.
    Need(hooks.write)(stream,id);
    FortMissionSaveHooks string;string.write=hooks.write;string.assertion=[&](auto) { Need(hooks.assertion)(0x9bU); }; //기존004bcbf0의 null 진단과 길이WORD/본문 일반쓰기를 그대로 공유한다.
    SaveFortMissionName(stream,capturedHandle,string);
}
}
