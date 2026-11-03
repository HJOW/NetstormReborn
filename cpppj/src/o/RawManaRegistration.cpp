// 실제 목록/개수 폭과 원문 진단 앞뒤의 읽기 시점을 보존한다.
#include "o/RawManaRegistration.h"
#include "o/SquidPostPop.h"
#include <bit>
#include <limits>
#include <stdexcept>
namespace netstorm::o { // 원문 포인터 범위 밖 접근은 호스트 장치 오류로 구별한다.
void RegisterManaProvider(OriginalEdition edition,SquidPostPopList& list,std::uint32_t& productionDirty,
    std::uint32_t value,const ManaRegistrationDiagnostics& diagnostics) { // 저장된 중복 판단을 진단 뒤 다시 검색하지 않는다.
    const bool patch=edition==OriginalEdition::Patch1078;const auto count=std::bit_cast<std::int32_t>(list.Count());bool already{}; // 패치는 ECX에 최초 개수/EDX에 포인터를 저장한다.
    if (count>0) {
        const auto storage=list.Storage();
        std::int32_t index{}; // 첫 양수 count로 진입한 원문은 현재 개수 재조회 전에 첫 원소를 검사한다.
        do { // CD는 실패한 첫 비교/증가 뒤 반복 조건에서만 현재 count를 다시 읽는다.
            if (static_cast<std::size_t>(index)>=storage.size()) throw std::out_of_range("마나 공급원 검색의 실제 물리 범위 오류");
            if (storage[static_cast<std::size_t>(index)]==value) { already=true;break; }++index;
        } while (index<(patch ? count : std::bit_cast<std::int32_t>(list.Count())));
    }
    if (patch && diagnostics.checking && already) {
        if (!diagnostics.assertion) throw std::logic_error("마나 공급원 중복 원문 진단 장치 미연결");
        diagnostics.assertion("!alreadyThere","\\Ns\\O\\Mana.cpp",0x44);
    }
    if (!already) {
        const auto used=patch ? count : std::bit_cast<std::int32_t>(list.Count());const auto capacity=list.Capacity(); // 진단이 선택된 중복 경로는 이 분기에 다시 진입하지 않는다.
        if (capacity>std::numeric_limits<std::uint32_t>::max()) throw std::out_of_range("마나 공급원 용량 DWORD 범위 오류");
        if (used<std::bit_cast<std::int32_t>(static_cast<std::uint32_t>(capacity))) {
            if (used<0) throw std::out_of_range("마나 공급원 음수 저장 인덱스 오류");auto storage=list.Storage();const auto index=static_cast<std::uint32_t>(used); // 원문 범위 밖 쓰기를 더 큰 배열로 숨기지 않는다.
            if (index>=storage.size()) throw std::out_of_range("마나 공급원 저장의 실제 물리 범위 오류");storage[index]=value;list.SetCount(list.Count()+1U);
        }
    }
    ++productionDirty; // 중복/가득 참/비어 있는 음수 용량에도 현재 DWORD를 한 번 증가한다.
}
}
