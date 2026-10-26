// Client와 실제 로컬 양끝이 같은 숫자 토큰/고정표/수명 장치를 공유한다.
#pragma once
#include "o/RawConnectionRegistration.h"
#include <map>
#include <memory>

namespace netstorm::o { //숫자 토큰은 호스트 포인터를32비트로 절삭하지 않는다.
// 실제 Node 주소는 owner가 유지하며 표와 장치는 Session보다 오래 보관할 수 있다.
class RawConnectionRegistry {
public:
    RawConnectionRegistry()=default; //독립 전송의 첫 BSS0과 고정 주소만 구성한다.
    RawConnectionRegistry(const RawConnectionRegistry&)=delete; //동일 연결의 표/반납 권한을 복제하지 않는다.
    RawConnectionRegistry& operator=(const RawConnectionRegistry&)=delete; //생존 숫자 토큰을 다른 표로 덮지 않는다.
    RawConnectionRegistry(RawConnectionRegistry&&)=delete; //빌린 고정표 주소를 이동하지 않는다.
    RawConnectionRegistry& operator=(RawConnectionRegistry&&)=delete; //현재 연결/장치의 주소 수명을 유지한다.
    void BindSynchronization(std::uint32_t& value,std::shared_ptr<void> lifetime={}); //실제005c8bf4와 그 owner를 빌리며 현재 값은 쓰지 않는다.
    ConnectionCleanupGlobals Cleanup(); //같은 현재 동기화 토큰을 포함한 실제 표의 뷰다.
    ConnectionCleanupGlobals Cleanup(std::uint32_t& synchronization); //기존 직접 owner 검사는 동일 표에 명시 동기화를 빌릴 수 있다.
    std::uint32_t Track(PacketConnection& connection,std::function<void()> destroy,std::function<void()> release); //현재 실제 노드와 두 실제 수명 장치를 단일 opaque 토큰에 연결한다.
    bool Owns(std::uint32_t token) const; //해제 후 노드를 다시 읽지 않는 순수 존재 조회다.
    PacketConnection& Node(std::uint32_t token) const; //같은 현재 생존 노드만 읽는다.
    void Register(std::int32_t id,std::uint32_t token); //004e2870의 현재 슬롯/sender를 쓴다.
    std::int32_t Accept(std::uint32_t token); //현재 첫 빈 pending1..31에 같은 노드를 등록하고 개수를 증가한다.
    bool CanAccept() const; //004e02d0의 현재 개수<31 관문이며 슬롯을 새로 지우지 않는다.
    void Assign(std::int32_t pending,std::int32_t target,std::uint32_t server,std::function<void(std::uint32_t)> buffered); //004e27b0 전체 현재 재배치를 실행한다.
    void Destroy(std::uint32_t token); //현재 노드의 실제 소멸 장치만 호출한다.
    void Release(std::uint32_t token); //실제 free 성공 뒤 주소 장치를 반납하고 일반 분배의 현재 별칭도 해제한다.
    void Disconnect(std::int32_t id); //457의 원문 개별 종료를 같은 실제 표/장치에 실행한다.
    void CloseAll(); //457의 양수22칸 정리이며 버퍼/pending 전체 정리를 추가하지 않는다.
    void EndPlayers(std::uint32_t server); //원문 server 관문과 owner1..8 종료다.
    void ClosePending(); //원문32pending 정리이며 개수를 임의 재계산하지 않는다.
    void Forget(std::uint32_t token); //호스트 owner 최종 소멸에서 같은 남은 별칭/장치만 반납한다. Native 전체 정리의 대체가 아니다.
private:
    struct Binding {
        PacketConnection* node{}; //실제 전송이 소유하는 동일 고정 주소다.
        std::function<void()> destroy,release; //현재 노드의 실제 close/free이며 callback 뒤 표를 다시 읽는다.
    };
    void RemoveAliases(std::uint32_t token); //같은 token의 현재 별칭만 지우고 새 세대의 칸은 보존한다.
    ConnectionRegistrationHooks RegistrationHooks(std::function<void(std::uint32_t)> buffered={}); //같은 노드/실제 진단만 조립한다.
    ConnectionCleanupHooks CleanupHooks(); //기존457 부모가 같은 실제 수명 장치를 호출하도록 한다.
    void RunCleanup(const std::function<void()>& operation); //Native 부모가 자신의 칸/count store를 마칠 때까지 일반 별칭 해제를 유예한다.
    std::array<std::uint32_t,kTransceiverSlots> transceivers_{},buffers_{}; //005e4a18/4a70의 서로 다른 두22칸이다.
    std::array<std::uint32_t,kPendingConnectionSlots> pending_{}; //005e46f0의 같은32칸이며0칸도 보존한다.
    std::uint32_t pendingCount_{},localSynchronization_{},nextToken_{0x6d000000U}; //실제 개수·독립 동기화와 재사용하지 않는 호스트 식별자 발급기다.
    std::uint32_t* synchronization_{&localSynchronization_}; //실제 Client 바인딩 시 기존005c8bf4를 직접 가리킨다.
    std::shared_ptr<void> synchronizationOwner_; //외부 동기화 주소가 대여자보다 먼저 사라지지 않게 한다.
    std::map<std::uint32_t,Binding> bindings_; //현재 실제 연결만 보관하며 해제된 노드의 주소는 남기지 않는다.
    std::uint32_t cleanupDepth_{}; //원문 부모의 직접 슬롯/count 쓰기와 일반 free의 별칭 해제를 구별하는 호스트 경계다.
};
}
