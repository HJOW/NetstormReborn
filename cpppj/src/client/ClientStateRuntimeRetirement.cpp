// Native 부모의 각 위치에서 실제 작은 자식을 호출하고 C++ 소멸자의 중복 정리를 막는다.
#include "client/ClientStateRuntimeRetirement.h"
#include "client/ClientMain.h"
#include "client/ClientCoreDataGlobals.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawCampaignSession.h"
#include "client/RawSurfaceWorld.h"
#include "o/RawPlayerLinksCleanup.h"
#include <stdexcept>
#include <utility>

namespace netstorm::client { // 첫 메뉴의 자연 반환과 실제 Runtime의 정상 자식 반환을 혼동하지 않는다.
namespace { // 초기 메뉴에서도 같은 실제 Player 기록의 현재 DWORD를 조회한다.
std::uint32_t Brain(const ClientPlayerRecords& players,std::uint32_t owner) { //00413ae0의 최초 Player+0xb0 조회이며 실제 비영이면 없는 AI owner로 정리하지 않는다.
    const auto records=players.Records();const auto offset=std::size_t(owner)*180U+0xb0U;
    if (owner<1 || owner>8 || offset+4>records.size()) throw std::out_of_range("State AI 정리의 현재 Player1..8 물리 범위");
    std::uint32_t value{};for (std::size_t byte=0;byte<4;++byte) value|=std::uint32_t(records[offset+byte])<<(byte*8);return value; //원문 한 DWORD만 읽고 나머지 필드는 보존한다.
}
}
ClientStateRuntimeRetirement::ClientStateRuntimeRetirement(Client& client,std::shared_ptr<RawSurfaceStorage> storage):client_(client),storage_(std::move(storage)) {
    const auto& actual=client_.SurfaceStorageIfInitialized();
    if (!storage_ || storage_->pool.Edition()!=o::OriginalEdition::Patch1078 || storage_->Authority()!=client_.GameplayGlobals()->Authority() || storage_->GameAux()!=client_.GameplayGlobals()->GameAux() || (actual && actual!=storage_) || !storage_->Processes().UsesKernel(&client_.GetKernel()))
        throw std::invalid_argument("State Runtime 반납의 같은 Client/표면/권한/Kernel 필요");
    if (client_.GameplayGlobals()->Borrowed()) session_=&client_.GameplayGlobals()->RequireSession();
    generation_=storage_->Generation(); //새 숫자를 발급하거나 실제 게임 State를 쓰지 않고 현재 대여 세대만 고정한다.
    static_cast<void>(Current()); //검증 자체로 Session·Player·Kernel을 새로 만들거나 정리하지 않는다.
}
RawCampaignSession* ClientStateRuntimeRetirement::Current() const {
    auto& game=*client_.GameplayGlobals();
    if (storage_->Generation()!=generation_) throw std::logic_error("State 정리 도중 표면 대여 세대 변경");
    if (game.Borrowed()) {
        auto& current=game.RequireSession();
        if (&current!=session_ || current.World().Storage()!=storage_ || !storage_->Borrowed() || current.PlayerRecords().data()!=client_.PlayerRecords()->Records().data())
            throw std::logic_error("State 정리 도중 현재 Session/표면/Player 정체성 변경");
        return &current;
    }
    if (session_ || storage_->Borrowed()) throw std::logic_error("State 자식 정상 반납 전에 현재 Runtime 대여가 사라짐");
    return nullptr;
}
void ClientStateRuntimeRetirement::ResetAi(std::uint32_t owner) {
    if (owner<1 || owner>8) throw std::out_of_range("State AI 정리의 owner1..8 범위");
    if (auto* current=Current()) { auto pass=current->ResetAiForState(owner);Current()->ReturnAiForState(std::move(pass)); }
    else if (Brain(*client_.PlayerRecords(),owner)) throw std::logic_error("현재 비영 brain의 실제 Runtime 소멸 장치 누락"); //원문 brain0의 자연 반환만 허용하며 가짜 파괴/0쓰기를 하지 않는다.
}
void ClientStateRuntimeRetirement::ClearPlayerLinks() {
    if (auto* current=Current()) { auto pass=current->ClearPlayerLinksForState();Current()->ReturnPlayerLinksForState(std::move(pass)); }
    else o::CleanupPlayerLinks({[this] { return client_.PlayerRecords()->Records(); },[this](auto token) { client_.PlayerRecords()->ProductionDeckMemory()->Release(token); }}); //World 없는 HUD가 만든 같은 실제 Deck도 원문 free 위치에서 반납한다.
}
void ClientStateRuntimeRetirement::ClearKernel() {
    if (processes_) throw std::logic_error("같은 State Runtime의 단일 Kernel Clear 증빙 중복 생성");
    if (auto* current=Current()) processes_.emplace(current->ClearProcessesForState());
    else client_.GetKernel().Clear(); //실제 첫 메뉴의 현재 슬롯도 단일 순회로 정리하며 World 부재로 생략하지 않는다.
}
void ClientStateRuntimeRetirement::Retire(const std::function<void()>& releaseOwner) {
    auto* current=Current();auto& game=*client_.GameplayGlobals();const auto core=client_.CoreDataGlobals();
    if (game.player.playerKnown || core->Active() || !core->Current().empty()) throw std::logic_error("State Runtime 반납 전 실제 known0/Core0 선택 필요");
    if (!current) return; //현재 Runtime 대여가 없다는 실제 관문일 뿐 Player/Pool/전역 정리 완료를 뜻하지 않는다.
    if (!current->Production().Deck().PlayerLinksReturnedForState()) throw std::logic_error("State Runtime 반납 전 실제 Player 링크 증빙 필요");
    for (std::uint32_t owner=1;owner<9;++owner) if (!current->Participation().BrainReturnedForState(owner)) throw std::logic_error("State Runtime 반납 전 실제 여덟 AI 증빙 필요"); //각 원문 owner의 실제 반환을 개별 조회한다.
    if (!releaseOwner) throw std::logic_error("State Runtime의 실제 소유자 반납 장치 누락");
    if (!current->World().ProcessBindingsReturned()) {
        if (!processes_) throw std::logic_error("State Runtime 반납 전 실제 단일 Kernel Clear 증빙 필요");
        current->ReturnProcessBindingsForState(std::move(*processes_));processes_.reset(); //실제 Host의 관문이 실패하면 Session/Pool 수명은 그대로 남는다.
    }
    releaseOwner(); //호출 뒤 이전 Session 주소를 읽지 않으며 실제 외부 소유자가 모든 고정뷰 소비자를 소멸시켜야 한다.
    if (game.Borrowed() || storage_->Borrowed()) throw std::logic_error("State 소유자 장치가 실제 Session/표면 대여를 반납하지 않음");
    session_=nullptr; //실제 반납을 확인한 뒤에만 오래된 비소유 포인터를 버린다.
}
}
