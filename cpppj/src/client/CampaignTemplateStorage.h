// Client와 Session이 같은 Template9/실제 manager 이미지·State cursor 수명을 공유한다.
#pragma once
#include "o/Template.h"
#include "o/RawFortWorldLoad.h"
#include "o/RawPlayerTemplateState.h"
#include "o/RawTemplateLifetime.h"
#include "o/RawDataManagerFileSave.h"
#include "o/RawFortImage.h"
#include "o/RawMemStream.h"
#include "o/FortSerializationGlobals.h"
#include "o/RawFortCurrentName.h"
#include "o/RawDataManagerInitialize.h"
#include "client/ClientTemplateSectionNames.h"
#include <map>
#include <memory>
namespace netstorm::client {
// 원본 저장 부모가 선택한 현재 파일 장치이며 이미지를 복사해 별도 규칙으로 바꾸지 않는다.
struct TemplateFileWriteHooks {
    std::function<void(std::string_view,std::uint32_t)> permissions; //현재 이름의 CRT 권한 요청을 실제 쓰기 경계에 인계한다.
    std::function<void(std::string_view,std::span<const std::uint8_t>)> write; //저장 부모가 결정한 현재 이름과 같은 이미지 원문을 쓴다.
};
// 원문 Load의 실제 파일/CRT/현재 슬롯/제목 경계다. 선택한 호출만 연결을 요구한다.
struct TemplateFileLoadHooks {
    std::function<std::vector<std::uint8_t>(std::string_view)> read; // 실제 읽기 시점의 파일 할당이다. 빈 벡터는 원문 open 실패이며 throw와 구별한다.
    std::function<bool(std::string_view,std::string_view)> sameName; // 기존 manager 이름과 요청 이름의 실제 CRT stricmp==0이다.
    std::function<void()> wrongName; // 기존 다른 이름의 isCalled 진단이며 반환하면 원문대로 기존 manager를 유지한다.
    std::function<bool(std::string_view)> readOnly; // 파일 출처/권한으로 판정하는0041af80 경계다. 기존 header+8 비영이면 호출하지 않는다.
    std::function<std::int32_t()> typeCount; // 실제 서명 검사가 도달한 순간의 현재 signed TYPE 개수다.
    std::function<void()> tooManyTypes; // numTypes<256 진단이다. 반환하면 현재 원문 서명 읽기를 계속한다.
    std::function<void(std::string_view)> rejectFile; // 잘못된 서명의 실제 파일 제거 요청이다. 보호 파일 삭제를 자동 허용하지 않는다.
    std::function<std::uint32_t()> currentSlot; // 성공 뒤 원문 현재 Template 인덱스를 다시 조회한다.
    std::function<void()> refreshTitle; // 현재 슬롯의 성공에서만00460730의 중간 제목 효과를 수행한다.
};
// 현재 섹션이 바뀌어도 같은 manager의 같은 생성 수명에 쓰기 위한 불투명 참조다.
struct TemplateWriteStream {
    std::uint32_t manager{},section{}; // 실제 호스트 manager와 같은 생성 수명의 동적 섹션 토큰이다. 포인터를 DWORD로 자르지 않는다.
    std::uint64_t generation{}; // 같은 이름의 섹션을 재생성한 뒤 이전 참조로 쓰는 것을 거부한다.
};
// 실제 파싱 자료를 단독 소유하고 빌린 헤더는 같은 manager/State cursor를 조회한다.
class CampaignTemplateStorage {
public:
    explicit CampaignTemplateStorage(std::shared_ptr<o::FortSerializationGlobals> serializationGlobals={},std::shared_ptr<ClientTemplateSectionNames> sectionNames={}); // 원본9개헤더를초기화하고같은Client전역을빌린다. 생략하면독립전역한개를소유한다.
    ~CampaignTemplateStorage(); // 호스트 종료 때 남은 실제 소유 이미지를 반납한다. 원본 State 전환을 대신하지 않는다.
    CampaignTemplateStorage(const CampaignTemplateStorage&)=delete; // 고정 헤더/이미지 참조는 복사하지 않는다.
    CampaignTemplateStorage& operator=(const CampaignTemplateStorage&)=delete; // 같은 수명의 표/실제 이미지를 별개 사본으로 만들지 않는다.
    std::span<std::uint8_t> Headers(); // 실제180BYTE를 같은 Player/로더에 공급한다.
    std::uint32_t AllocateEmptyManager(std::uint32_t bytes); // 원문new276의실제미생성Image를소유표에추가하고불투명token/0을돌려준다.header는쓰지않는다.
    std::uint32_t ConstructEmptyManager(std::uint32_t manager,std::string_view name); // 같은할당에기존원문ctor와공유version0을쓰며header공개는바깥부모가맡는다.
    void InitializeEmptyManager(std::uint32_t manager,const std::function<void(o::DataManagerInitializeAssertion)>& assertion={}); // 고정manager의실제35개Create를공유mutable표로순회한다.선택진단은생략하지않는다.
    std::span<std::uint8_t> ManagerBytes(std::uint32_t manager); // 이미선택한같은manager276BYTE다.현재header를다시읽지않는다.
    o::TemplateLifetimeHooks ManagerLifetime(); // 같은owner의실제Destroy/Release를분리하여바깥NewManager에빌린다.
    std::shared_ptr<ClientTemplateSectionNames> SectionNamesOwner() const; // reader/신규초기화가공유하는Clientowner를읽는다.
    std::uint32_t& SerializationVersion(); // 생성자/객체로더/저장부모가직접같이쓰는실제005c89c0 DWORD다. 복사값을반환하지않는다.
    void BindDiagnosticLog(std::function<void(std::int32_t,std::string_view)> log); // 실제 Client level29 로그 장치를 빌린다. 지역 송신 이미지의 덤프가 같은 공유 formatter/FILE/Chat을 사용한다.
    void Install(std::uint32_t slot,o::FortTemplate fort,std::string name); // 이전 한 헤더를 실제 반납한 뒤 새 소유 manager를 설치한다.
    void InstallFile(std::uint32_t slot,o::FortTemplate fort,std::span<const std::uint8_t> image,std::string name,std::int32_t typeCount=188); // 실제 파일/서명/섹션 읽기 부모를 실행한다. 기본188은10.78 독립 호출용이며 Session은 현재 타입 수를 전달한다.
    void LoadFile(std::uint32_t slot,o::FortTemplate fort,std::string_view name,const TemplateFileLoadHooks& file); //004bf550의기존파일유지/읽기전공개/정상false부분정리를실제owner에서실행한다. fort는이미해석된보조metadata이며실제section은reader의할당을쓴다.
    o::FortImageData ReadLoginImage(std::string_view name,const TemplateFileLoadHooks& file,std::uint32_t territory); //004b63d0의지역Template읽기와004bf3e0의5섹션복제/직렬화/반납을실행한다.현재Client의version/이름표만공유하고전역9헤더는보존한다.
    void Borrow(std::uint32_t slot,std::uint32_t source); // 같은 manager를 빌리며 State/지형 cursor도 같은 객체를 사용한다.
    void ClearAll(); // 원본 전역 정리의 실제 소멸/할당 반납을 수행한다.
    void Clear(std::uint32_t slot); // 원본 단일 반납이다. 빈 포인터의 부가 필드도 비운다.
    o::FortTemplate& Template(std::uint32_t slot); // 현재 헤더의 manager를 매번 읽는다.
    o::PlayerTemplateStateStream& State(std::uint32_t slot); // 같은 manager의 현재 State bytes/pos/end다.
    o::PlayerTemplateStateStream& SelectState(std::uint32_t slot); // 원본 검색/쓰기허용 선택으로 같은 manager.current/State.pos를 변경한다.
    std::string ReadSubscriberName(std::uint32_t slot); // 004bd4d0처럼 실제 Subscriber 선택/ID 건너뜀/signed 길이 읽기로 현재 이름을 반환한다.
    o::PlayerTemplateStateStream& InitializeState(std::uint32_t slot,std::span<const std::uint8_t,28> bytes); // 기존 State 반납 뒤 새 섹션을 목록 끝에 만들고 원본16384BYTE 버퍼에28BYTE를 쓴다.
    TemplateWriteStream CreateSection(std::uint32_t slot,std::string_view name); // 원본 생성 부모로 이름의 기존 노드를 교체하고 새 마지막 노드의 참조를 반환한다. Temp 같은 동적 이름도 허용한다.
    TemplateWriteStream CreateManagerSection(std::uint32_t manager,std::string_view name); //원문이 이미 선택한 현재 manager를 고정해 섹션을 만든다. 뒤 callback의 header 교체로 다른 manager를 재선택하지 않는다.
    TemplateWriteStream FindSection(std::uint32_t slot,std::string_view name); // 원본 검색으로 current를 갱신한다. 없으면 current0/빈 참조이며 pos는 보존한다.
    TemplateWriteStream SelectSection(std::uint32_t slot,std::string_view name,std::int32_t writable); // 실제 선택 부모의 검색/진단/pos0을 같은 노드에 수행한다.
    TemplateWriteStream RenameSection(std::uint32_t slot,std::string_view destination,std::string_view source); // 실제 Rename 부모가 대상을 반납한 뒤 원래 source 노드의 이름만 교체한다. handle/생성수명/버퍼할당을 보존한다.
    TemplateWriteStream CurrentWriteStream(std::uint32_t slot); // 현재 헤더의 manager/current를 다시 읽어 같은 노드의 생성 수명을 고정한다.
    void WriteSection(TemplateWriteStream stream,std::span<const std::uint8_t> bytes); // 현재 선택과 무관하게 보관한 섹션의 원본 MemStream에 쓴다.
    void AccessSection(TemplateWriteStream stream,const std::function<void(std::span<std::uint8_t>,const o::MemStreamHooks&)>& operation); // 인라인 쓰기에 같은16BYTE/버퍼를 동기적으로 빌린다. 할당/노드 교체와 같은 노드의 중첩 쓰기 빌림은 허용하지 않는다.
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
    o::FortCurrentNameHooks BindCurrentName(o::FortCurrentNameHooks hooks); // 원문 이름 부모에 같은 헤더/manager+c256BYTE만 연결한다. Notify/저장/삭제 등은 실제 호출자가 공급한다.
    std::array<std::uint32_t,2> FileHeader(std::uint32_t slot); // 같은 manager+10c/+110의 서명/FE 판정DWORD다. 읽기/저장 전에는0/ffffffff다.
    void SaveFile(std::uint32_t slot,const o::DataManagerFileSaveState& state,o::FortImageHooks image,const TemplateFileWriteHooks& file); //같은 manager의 실제 서명/검색/current와 저장 부모를 통해 현재 섹션을 파일에 쓴다.
    std::span<std::uint8_t> GrowState(std::uint32_t slot,std::uint32_t size); // 원문을 보존한 채 같은 Fort의 실제 State 섹션에 새 버퍼를 만든다.
    void CommitStateGrowth(std::uint32_t slot); // Get이 복사를 끝낸 뒤 이전 State 할당을 반납한다.
    std::size_t OwnedImages() const; // 현재 실제 manager 할당 수를 읽기만 한다.
private:
    struct Image; // 파싱한 요새·같은State/지형 cursor·임시 이전 State를 하나로 소유한다.
    Image& Current(std::uint32_t slot); // 현재 불투명 manager 핸들로 실제 이미지를 찾는다.
    Image& Writable(TemplateWriteStream stream); // manager 반납이나 같은 섹션 재생성 뒤 오래된 참조를 거부한다.
    std::span<std::uint8_t> Header(std::uint32_t slot); // 같은 표의 한20BYTE를 빌린다.
    o::TemplateLifetimeHooks Effects(); // 실제 manager 소멸과 객체 할당 반납을 분리한다.
    std::shared_ptr<o::FortSerializationGlobals> serializationGlobals_; // 모든Image/외부Session보다먼저확보하는실제공유DWORD소유권이다.
    std::shared_ptr<ClientTemplateSectionNames> sectionNames_; // 모든Image보다오래사는동일512BYTE lazy표owner다.독립Storage는새Client상태를한개소유한다.
    std::function<void(std::int32_t,std::string_view)> diagnosticLog_; // 실제 Client보다 짧은 Template 수명이 빌리는 현재 진단 출력이다. 파일 읽기/일반 생성에는 요구하지 않는다.
    std::array<std::uint8_t,180> headers_{}; // Client보다 짧은 Session이 사라져도 유지되는 실제9개 헤더다.
    std::map<std::uint32_t,std::unique_ptr<Image>> images_; // 호스트 포인터를DWORD로 자르지 않는 실제 소유 객체 표다.
    std::uint32_t nextHandle_{1}; // 서로 살아 있는 manager를 구별하는 비영 호스트 핸들이다.
};
}
