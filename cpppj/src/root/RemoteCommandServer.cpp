// 실제 원격 명령 listener/4세션을 현재 Root 메인 select/Poll에 연결한다.
#include "root/RemoteCommand.h"
#include <stdexcept>

namespace netstorm::root { //479bb0은 실제 flags40 조회이며 생성과 분리한다.
namespace { // 선택된 console/진단과 원문 중복보존 집합 추가만 공유한다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R 원격 명령의 선택된 실제 장치 미연결");return function; } // 비활성 상태에서 불필요한 IO/로그 장치를 요구하지 않는다.
void Append(std::uintptr_t socket,NetworkSocketSet* read,NetworkSocketSet* write,NetworkSocketSet* error,std::uintptr_t& maximum) {
    for (auto* set:std::array<NetworkSocketSet*,3>{read,write,error}) if (set) { set->Add(socket);if (static_cast<std::intptr_t>(socket)>=static_cast<std::intptr_t>(maximum)) maximum=socket; } // 각 nullable집합의64상한/중복/현재순서를 유지하며 최고핸들은 현대 포인터 폭으로 보존한다.
}
}
RemoteCommandServer::RemoteCommandServer(RemoteCommandHooks hooks):hooks_(std::move(hooks)) {}
RemoteCommandServer::~RemoteCommandServer() noexcept(false) { Shutdown();handler_.reset(); } // 이후 std::array가 실제4세션을 역순 소멸한다.
SocketDevice& RemoteCommandServer::Listener() { if (!listener_) throw std::logic_error("R 원격 명령의 실제 NULL listener 접근");return *listener_; }
bool RemoteCommandServer::Active() const { return listener_ && (listener_->Flags()&0x40U); }
bool RemoteCommandServer::Open(std::int32_t port) {
    if (listener_) { Need(hooks_.assertion)("listening == NULL","c:\\ns\\zacket\\remotecommand.cpp",333);displacedListeners_.push_back(std::move(listener_)); } // 원문 진단이 복귀해 덮어쓰면 옛 OS 소켓은 즉시 닫지 않는다. 최종 호스트 owner만 반납한다.
    listener_=std::make_unique<SocketDevice>();static_cast<void>(listener_->Listen(static_cast<std::uint32_t>(port)&0xffffU));return (listener_->Flags()&0x40U)!=0; // 실패한 실제 listener 객체도 보관하며 포트는 원문 parser의 낮은WORD다.
}
void RemoteCommandServer::Handler(RemoteCommandHandler handler) {
    handler_=handler ? std::make_shared<RemoteCommandHandler>(std::move(handler)):nullptr;
    for (auto& session:sessions_) if (session.Active()) session.Handler(handler_); // 원문은 현재 활성 세션만 즉시 갱신한다. 재사용 시 Start가 새 handler를 설정한다.
}
bool RemoteCommandServer::Shutdown() {
    if (listener_) { if (listener_->Flags()&0x40U) for (auto& session:sessions_) if (session.Active()) session.End();listener_.reset(); }return true; // 현재listener가닫힌경우 세션을여기서종료하지않으며 원문세션소멸자가 나중에 처리한다.
}
bool RemoteCommandServer::Poll() {
    if (Listener().Flags()&0x40U) {
        std::unique_ptr<SocketDevice> accepted;if (Listener().Accept(accepted)>0) {
            Need(hooks_.log)(0,"Incoming remote command connection\n");
            for (auto& session:sessions_) if (!session.Active()) { Need(hooks_.log)(0,"Starting remote command session...\n");const auto started=session.Start(std::move(accepted),handler_);const auto current=handler_;if (started && current && !(*current)(RemoteCommandEvent::Started,session,{})) session.End();break; } // 첫빈4칸만 선택하며 모두바쁘면 실제 새accepted를 범위끝에서 닫는다. Started는 세션이 아닌 현재 서버handler를 읽는다.
        }
        for (auto& session:sessions_) if (session.Active()) session.Poll(); // Started callback 뒤 현재4칸을 다시 읽어 이미 종료된 세션을 건너뛴다.
    }return true;
}
std::uintptr_t RemoteCommandServer::Sets(NetworkSocketSet* read,NetworkSocketSet* write,NetworkSocketSet* error) {
    std::uintptr_t maximum{};if (Listener().Flags()&0x40U) { Append(Listener().Handle(),read,write,error,maximum);for (const auto& session:sessions_) if (session.Active()) Append(session.Handle(),read,write,error,maximum); }return maximum; // 원문0초select의 결과 집합으로 수신 여부를 새로 제한하지 않는다.
}
void RemoteCommandServer::Bind(NetworkHooks& hooks) { hooks.auxiliaryActive=[this] { return Active(); };hooks.auxiliarySets=[this](auto& sets) { return Sets(&sets.read,nullptr,&sets.error); };hooks.auxiliaryPoll=[this] { Poll(); }; } // 기존메인부모의 조건부중복Sets/순회뒤Poll을 같은 실제 owner에 연결한다.
}
