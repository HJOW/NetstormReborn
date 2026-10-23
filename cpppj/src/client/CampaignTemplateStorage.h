// Client와 Session이 같은 Template9/실제 manager 이미지·State cursor 수명을 공유한다.
#pragma once
#include "o/Template.h"
#include "o/RawFortWorldLoad.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawTemplateLifetime.h"
#include "o/RawDataManagerFileSave.h"
#include "o/RawFortImage.h"
#include <map>
#include <memory>
namespace netstorm::client {
// 원본 저장 부모가 선택한 현재 파일 장치이며 이미지를 복사해 별도 규칙으로 바꾸지 않는다.
struct TemplateFileWriteHooks {
    std::function<void(std::string_view,std::uint32_t)> permissions; //현재 이름의 CRT 권한 요청을 실제 쓰기 경계에 인계한다.
    std::function<void(std::string_view,std::span<const std::uint8_t>)> write; //저장 부모가 결정한 현재 이름과 같은 이미지 원문을 쓴다.
};
// 실제 파싱 자료를 단독 소유하고 빌린 헤더는 같은 manager/State cursor를 조회한다.
class CampaignTemplateStorage {
public:
    CampaignTemplateStorage(); // 원본처럼9개 초기 헤더를0으로 만든다. 이후 전역 정리는 별도다.
    ~CampaignTemplateStorage(); // 호스트 종료 때 남은 실제 소유 이미지를 반납한다. 원본 State 전환을 대신하지 않는다.
    CampaignTemplateStorage(const CampaignTemplateStorage&)=delete; // 고정 헤더/이미지 참조는 복사하지 않는다.
    CampaignTemplateStorage& operator=(const CampaignTemplateStorage&)=delete; // 같은 수명의 표/실제 이미지를 별개 사본으로 만들지 않는다.
    std::span<std::uint8_t> Headers(); // 실제180BYTE를 같은 Player/로더에 공급한다.
    void Install(std::uint32_t slot,o::FortTemplate fort,std::string name); // 이전 한 헤더를 실제 반납한 뒤 새 소유 manager를 설치한다.
    void InstallFile(std::uint32_t slot,o::FortTemplate fort,std::span<const std::uint8_t> image,std::string name,std::int32_t typeCount=188); // 실제 파일/서명/섹션 읽기 부모를 실행한다. 기본188은10.78 독립 호출용이며 Session은 현재 타입 수를 전달한다.
    void Borrow(std::uint32_t slot,std::uint32_t source); // 같은 manager를 빌리며 State/지형 cursor도 같은 객체를 사용한다.
    void ClearAll(); // 원본 전역 정리의 실제 소멸/할당 반납을 수행한다.
    void Clear(std::uint32_t slot); // 원본 단일 반납이다. 빈 포인터의 부가 필드도 비운다.
    o::FortTemplate& Template(std::uint32_t slot); // 현재 헤더의 manager를 매번 읽는다.
    o::PlayerTemplateStateStream& State(std::uint32_t slot); // 같은 manager의 현재 State bytes/pos/end다.
    o::PlayerTemplateStateStream& SelectState(std::uint32_t slot); // 원본 검색/쓰기허용 선택으로 같은 manager.current/State.pos를 변경한다.
    std::string ReadSubscriberName(std::uint32_t slot); // 004bd4d0처럼 실제 Subscriber 선택/ID 건너뜀/signed 길이 읽기로 현재 이름을 반환한다.
    o::PlayerTemplateStateStream& InitializeState(std::uint32_t slot,std::span<const std::uint8_t,28> bytes); // 기존 State 반납 뒤 새 섹션을 목록 끝에 만들고 원본16384BYTE 버퍼에28BYTE를 쓴다.
    o::PlayerTemplateStateStream& EnsureStateSize(std::uint32_t slot,std::int32_t minimum); // 원본 임시 생성/복사/추가/이름 교체 뒤 같은 새 현재 State를 돌려준다.
    std::vector<std::string> SectionOrder(std::uint32_t slot); // 실제 head/next를 따라 현재 섹션 순서를 조회한다.
    std::string_view CurrentSection(std::uint32_t slot); // 같은 manager가 마지막으로 선택한 실제 섹션 이름을 읽는다.
    o::FortTerritoryStream& Territory(std::uint32_t slot); // 같은 manager의 실제 Territory cursor다.
    o::FortTerritoryStream& SelectTerritory(std::uint32_t slot); // 원본 읽기 선택으로 같은 manager.current와 지형 커서를 초기화한다.
    o::FortMoneyCursor& Money(std::uint32_t slot); // 같은 manager의 Money pos/end를 빌리며 현재 섹션은 바꾸지 않는다.
    o::FortMoneyCursor& SelectMoney(std::uint32_t slot); // 004bf340의 읽기 선택(0)으로 Money를 선택하고 pos를0으로 되돌린다.
    o::FortMetaStream& Badges(std::uint32_t slot); // 같은 실제 Badges 버퍼/present/pos/end를 빌린다.
    o::FortMetaStream& SelectBadges(std::uint32_t slot); // 004be8e0의 쓰기 허용 선택(1)으로 빈 Badges도 그대로 전달한다.
    o::FortMetaStream& Technology(std::uint32_t slot); // 같은 실제 Technology 버퍼/present/pos/end를 빌린다.
    o::FortMetaStream& SelectTechnology(std::uint32_t slot); // 004beee0의 쓰기 허용 선택(1)으로 현재 Technology를 선택한다.
    const std::string& Name(std::uint32_t slot); // 같은 manager의 현재 파일 이름이다.
    std::array<std::uint32_t,2> FileHeader(std::uint32_t slot); // 같은 manager+10c/+110의 서명/FE 판정DWORD다. 읽기/저장 전에는0/ffffffff다.
    void SaveFile(std::uint32_t slot,const o::DataManagerFileSaveState& state,o::FortImageHooks image,const TemplateFileWriteHooks& file); //같은 manager의 실제 서명/검색/current와 저장 부모를 통해 현재 섹션을 파일에 쓴다.
    std::span<std::uint8_t> GrowState(std::uint32_t slot,std::uint32_t size); // 원문을 보존한 채 같은 Fort의 실제 State 섹션에 새 버퍼를 만든다.
    void CommitStateGrowth(std::uint32_t slot); // Get이 복사를 끝낸 뒤 이전 State 할당을 반납한다.
    std::size_t OwnedImages() const; // 현재 실제 manager 할당 수를 읽기만 한다.
private:
    struct Image; // 파싱한 요새·같은State/지형 cursor·임시 이전 State를 하나로 소유한다.
    Image& Current(std::uint32_t slot); // 현재 불투명 manager 핸들로 실제 이미지를 찾는다.
    std::span<std::uint8_t> Header(std::uint32_t slot); // 같은 표의 한20BYTE를 빌린다.
    o::TemplateLifetimeHooks Effects(); // 실제 manager 소멸과 객체 할당 반납을 분리한다.
    std::array<std::uint8_t,180> headers_{}; // Client보다 짧은 Session이 사라져도 유지되는 실제9개 헤더다.
    std::map<std::uint32_t,std::unique_ptr<Image>> images_; // 호스트 포인터를DWORD로 자르지 않는 실제 소유 객체 표다.
    std::uint32_t nextHandle_{1}; // 서로 살아 있는 manager를 구별하는 비영 호스트 핸들이다.
    std::array<std::uint8_t,4> managerGlobal_{}; // 원본 생성자가0으로 쓰는005c89c0의 같은 Client 수명 저장소다. 전체 State7의 다른 사용처는 후속이다.
};
}
