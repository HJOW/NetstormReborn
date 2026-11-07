// 원문4955b0/495910의 현재 전역 publish·파일 반납·ProcessForm 부착을 같은 Client 장치에 연결한다.
#include "client/ClientBuildReplay.h"
#include "client/RawBuildReplay.h"
#include "client/ClientDiagnostics.h"
#include "client/UberGump.h"
#include "client/GameWorld.h"
#include "client/ClientMain.h"
#include "client/ClientGameplayGlobals.h"
#include "client/ClientPlayerRecords.h"
#include "client/RawSurfaceWorld.h"
#include "client/RawProcessHostStorage.h"
#include "client/RawCampaignSession.h"
#include "client/GameMenuCommands.h"
#include "o/RawGameAuxState.h"
#include "o/SquidProcess.h"
#include <bit>
#include <cstring>
#include <stdexcept>
#include <string>

namespace netstorm::client { // Client보다 긴 외부 Kernel도 해제된 장치 포인터를 역참조하지 않는다.
struct ClientBuildReplayContext {
    BuildReplayState state; //4955b0의200행과 현재 위치/시각이다. C++ 최초 tail0은 원문 미정 heap 기대값이 아니다.
    BuildReplayHooks hooks; // 현재 Client 조회/TYPE/파일/건설·선택 장치다. Process this를 붙잡지 않는다.
};
namespace { // 실제 재생 객체와 선택된 파일/장치의 호스트 수명을 소유한다.
// ClientTrace가 반납된 뒤에는 외부 Kernel이 이전 Client 주소를 사용하지 못하게 한다.
std::function<Client&()> LiveClient(Client& client) {
    return [lifetime=std::weak_ptr<std::uint8_t>(client.DiagnosticGlobals().buildReplayLifetime),pointer=&client]() -> Client& { if (lifetime.expired()) throw std::logic_error("건설 재생의 실제 Client 수명 종료");return *pointer; };
}
// 주어진 현재 레코드의 비정렬 DWORD를 안전하게 읽는다.
std::uint32_t Read(std::span<const std::uint8_t> bytes,std::size_t at) { if (at+4>bytes.size()) throw std::out_of_range("건설 재생 현재 Player 물리 범위");return std::uint32_t(bytes[at])|(std::uint32_t(bytes[at+1])<<8)|(std::uint32_t(bytes[at+2])<<16)|(std::uint32_t(bytes[at+3])<<24); }
// 사용 시점의 현재 Client만 조회하여 세션/해상도 교체 전 포인터를 보관하지 않는다.
BuildReplayHooks MakeHooks(Client& client) {
    const auto live=LiveClient(client);BuildReplayHooks hooks;
    hooks.now=[live] { return live().Time().game; };
    hooks.validPoint=[live](float x,float y) { const auto edge=static_cast<float>(live().GameplayGlobals()->WorldCells());return x>0.0F && x<edge && y>0.0F && y<edge; }; //40e800의 ordered strict 양축 범위다.
    hooks.findType=[live](std::string_view name) { const std::string terminated(name);const auto& types=live().SurfaceStorage()->types;
        for (std::size_t index=0;index<types.size();++index) if (_stricmp(types[index].name.c_str(),terminated.c_str())==0) return static_cast<std::int32_t>(index); //49a860의 현재 표 첫 일치를 같은 번호 순서로 찾는다.
        return std::int32_t{-1}; };
    hooks.constructPacket=[live](std::span<std::uint8_t> packet) { if (packet.size()<25) throw std::out_of_range("재생40a7c0의25BYTE 부족");auto& aux=*live().GameplayGlobals()->GameAux();packet[1]=0;packet[0]=0;packet[2]=packet[3]=0;
        for (std::size_t index=17;index<25;++index) packet[index]=0; // 실제 생성자가 쓰는 double시각0만 초기화하며 품질16은 caller 물리를 유지한다.
        packet[1]=aux.ConstructionRequestHeader()[0];const auto length=aux.ConstructionRequestLength();packet[2]=static_cast<std::uint8_t>(length);packet[3]=static_cast<std::uint8_t>(length>>8);packet[0]=aux.ConstructionRequestHeader()[1]&0x7f; };
    hooks.send=[live](auto target,auto packet) { live().GameplayGlobals()->RequireSession().SendLocalPacket(std::bit_cast<std::int32_t>(target),packet); }; // 같은 실제9 송신이며 현재 Session을 지연 조회한다.
    hooks.assertion=[live](auto expression,auto file,auto line) { live().SurfaceStorage()->AssertDisplay(expression,file,line); };
    hooks.label=[live](std::string_view,std::uint32_t sid) { live().GameplayGlobals()->RequireSession().ApplyBuildReplaySelection(sid); }; //4958d0은 라벨 문구를 사용하지 않고 실제7c/88/선택만 수행한다.
    return hooks;
}
// 동일 Host가 ProcessForm/PID/부모 수명을 관리하며 Run은 복원한 한 이벤트 몸체만 호출한다.
class ClientBuildReplayProcess final : public o::SquidProcess {
public:
    ClientBuildReplayProcess(Client& client,std::shared_ptr<ClientBuildReplayGlobals> globals,FILE* file):globals_(std::move(globals)),context_(std::make_shared<ClientBuildReplayContext>()) {
        if (!globals_) throw std::invalid_argument("건설 재생 현재 전역 미설치");context_->hooks=MakeHooks(client);globals_->current=context_;globals_->playing=1;ReadBuildReplay(file,context_->state,context_->hooks); // 실제 publish/활성1은 scanf보다 앞이다.
    }
    void RunFrame() override { const auto context=context_;auto hooks=context->hooks;hooks.kill=[this] { if (!Host()) throw std::logic_error("재생 Run의 실제 Host 미설치");Host()->Kill(*this,0); };RunBuildReplayFrame(context->state,globals_->playing,packet_,hooks); } // Kill이 this를 반납해도 현재 호출의 이벤트/장치는 stack pin이 유지한다.
private:
    std::shared_ptr<ClientBuildReplayGlobals> globals_; // Client의 현재 playing DWORD를 같은 주소에서 유지한다.
    std::shared_ptr<ClientBuildReplayContext> context_; // 실제 프로세스가 소유하며 전역 current는 약한 참조만 가진다.
    std::array<std::uint8_t,25> packet_{}; // C++ 호스트의 명시 초기 물리0이다. 미정 원문 stack의 품질BYTE와 같다고 주장하지 않는다.
};
// stale 원문 포인터를 조용한 성공/빈 이벤트로 바꾸지 않는다.
std::shared_ptr<ClientBuildReplayContext> Current(const std::shared_ptr<ClientBuildReplayGlobals>& globals) { if (!globals) throw std::logic_error("현재 건설 재생 전역 미연결");const auto context=globals->current.lock();if (!context) throw std::logic_error("현재 건설 재생 객체의 호스트 수명 종료");return context; }
}
//4958d0의 마지막 직접 선택은 현재 World의 원문 처리기를 사용한다.
void UberGump::SelectBuildReplayObject(std::uint32_t sid) { if (!world_) throw std::logic_error("재생 선택의 실제 현재 World 없음");world_->Select(sid); }
void StartLatestBuildReplay(Client& client) {
    const auto root=client.UserData() ? *client.UserData() : platform::UserDataRoot::Resolve();const auto directory=root.Captures()/L"BuildRecords"/L"d";std::uint32_t number{};std::filesystem::path path;
    for (;;) { path=directory/(std::to_string(std::bit_cast<std::int32_t>(number))+".rec");if (!std::filesystem::exists(path)) break;++number; } //495910은0부터 연속된 파일의 첫 빈 번호에서 멈춘다.
    if (!number) return;path=directory/(std::to_string(std::bit_cast<std::int32_t>(number-1U))+".rec");FILE* file{};if (_wfopen_s(&file,path.c_str(),L"r") || !file) throw std::runtime_error("현재 건설 재생 파일 열기 실패");const std::shared_ptr<FILE> stream(file,[](FILE* current) { std::fclose(current); }); // 실제 부착까지 파일을 유지하고 예외 때도 반납한다.
    const auto globals=client.DiagnosticGlobals().buildReplay;auto process=std::make_unique<ClientBuildReplayProcess>(client,globals,file);const auto game=client.GameplayGlobals();const auto records=client.PlayerRecords()->Records();const auto source=Read(records,std::size_t(game->player.battleOwner)*180+0x9c);if (source>65535U) throw std::out_of_range("기록 재생 현재 source SID의 WORD 범위");
    const auto type=game->GameAux()->BuildReplayProcessType();client.SurfaceStorage()->Processes().Host().Attach(std::move(process),type,o::Sid{static_cast<std::uint16_t>(source)},0x10); // 판독/시각 기록 뒤 현재 Player source→현재TYPE→실제1bd20이며 파일 닫기는 그 뒤다.
}
bool DispatchBuildReplayKey(Client& client,InputEvent event) {
    if ((event.code&0x03ffffffU)!=0x760000U || (event.code&InputCode::kRelease) || client.Paused() || !client.GameplayGlobals()->specialAuxiliary || !(event.code&InputCode::kControl) || client.SurfaceStorage()->ownerMode.challenge) return false;
    if (event.code&InputCode::kShift) { const auto command=GameMenuCommandValue(0x00542a40U);if (!command) throw std::logic_error("개발자 F7의 현재 NextStage 명령 미설치");client.Input().PushCommand(*command,0,0); }else StartLatestBuildReplay(client);return true; // NextStage는 재생 인덱스를 직접 바꾸지 않는다.
}
bool CurrentBuildReplayIsLabel(const std::shared_ptr<ClientBuildReplayGlobals>& globals) { const auto context=Current(globals);return IsBuildReplayLabel(context->state,context->hooks); }
void NotifyCurrentBuildReplay(const std::shared_ptr<ClientBuildReplayGlobals>& globals,std::uint32_t sid) { const auto context=Current(globals);NotifyBuildReplayLabel(context->state,sid,context->hooks); }
}
