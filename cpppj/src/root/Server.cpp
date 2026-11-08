// 같은 현재 R 전역과 실제 파일/진단/전송·CCh/메인 부모를 생성 순서에 맞춰 접합한다.
#include "root/Server.h"
#include "root/AddressCodec.h"
#include "root/CommandPages.h"
#include "root/Lifecycle.h"
#include "root/MainLoop.h"
#include "root/PacketRecord.h"
#include "root/ProcessTrace.h"
#include "root/Random.h"
#include "root/RemoteCommand.h"
#include "root/RootControl.h"
#include "root/ServerList.h"
#include "client/ConfigFileRead.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include "o/PacketRegistrationTable.h"
#include "platform/Utf8Path.h"
#include "platform/LegacyRegistryValue.h" // R Config의 {&경로}도 실제 원문 legacy 기본값 조회를 사용한다.
#include <cstdlib>
#include <process.h>
#include <stdexcept>
#include <utility>

namespace netstorm::root { // 원문 R의 프로세스 전역은 각 reset/cycle마다 새 owner로 복제하지 않는다.
namespace { // 실제 생성 순서의 forward callback과 같은 CRT 환경 경계다.
template<class Module> Module& Require(const std::unique_ptr<Module>& module) { if (!module) throw std::logic_error("R 실제 장치 구성 완료 전 callback 실행");return *module; } // 아직 구성되지 않은 소비자를 빈 성공으로 대신하지 않는다.
std::optional<std::string> Environment(std::string_view name) { const std::string key(name.substr(0,name.find('\0')));const auto* value=std::getenv(key.c_str());return value ? std::optional<std::string>{value}:std::nullopt; } // 원문의 getenv를 UTF-8 프로세스 manifest 아래에서 같은 IDENTITY/USER·Config 환경 조회에 연결한다.
}
struct Server::State { // 마지막 실행 소비자부터 먼저 반납하며 참조한 상태/파일·옵션은 뒤에 남긴다.
    std::filesystem::path root;platform::UserDataRoot user; // 불변 가상CWD와 같은 사용자 홈 출력 정책이다.
    std::shared_ptr<o::RawFileName> names; o::BaseFileSystem files;o::ConfigInterface config; // 같은10칸 파일명 고리/VFS/실제 Config 등록부다. 별도Client를 만들지 않는다.
    ConnectionState connections;NetworkState network;LifecycleState lifecycleState;ClockState clockState;Random random; // 같은 현재 번호/타임아웃·프로세스/시계·초기1 난수 상태다.
    ChallengeMessages messages;SharedFileTables fileTables;MirrorPlayerDefaults mirror;MainLoopState mainState; // 같은 뉴스/관리 표·별도BSS0 기본 참가자/예약·요청/감시 상태다.
    SharedFileStorage storage;PacketRecord record; // 실제 사용자 홈 우선 읽기/자동 저장과 같은 현재 시계의 기록 파일이다.
    std::unique_ptr<SharedFiles> sharedFiles;std::unique_ptr<Configuration> configuration;std::unique_ptr<ProcessTrace> trace; // 같은 파일 owner/현재 Options와 복귀형 진단이다. Configuration 생성 중에는 forward callback을 실행하지 않는다.
    std::unique_ptr<Clock> clock;std::unique_ptr<TransceiverTransport> transport;std::unique_ptr<ChallengeStore> store; // 같은 시계·실제 노드와 현재 CCh 저장소다.
    std::unique_ptr<AddressCodec> addresses;std::unique_ptr<Routing> routing;std::unique_ptr<CommandPages> pages;std::unique_ptr<ServerList> servers; // 현재 주소/라우팅·템플릿/서버목록과 checksum 참조다.
    std::unique_ptr<LoginRequest> login;std::unique_ptr<ChallengeRuntime> challenges;std::unique_ptr<RootControl> control;std::unique_ptr<RootDirectory> directory; // 서로 forward 호출하는 실제 수신 부모들이며 실행 전 전부 구성한다.
    std::unique_ptr<RemoteCommandServer> remote;std::unique_ptr<Lifecycle> lifecycle;std::unique_ptr<MainLoop> main; // 원문 기본 비활성 보조 객체와 실제 전체 시작/정리·메인 루프다.
    State(std::filesystem::path working,platform::UserDataRoot destination); // 모든 실제 hook을 먼저 결합하고 설정읽기/소켓·시각/창 실행은 MainLoop.Run의 원문 위치로 미룬다.
    ~State(); // 실행 등록을 반납한 뒤 객체를 역순 정리한다. 정상 원문 Shutdown을 예외 경로에 합성하지 않는다.
};
Server::State::State(std::filesystem::path working,platform::UserDataRoot destination)
    :root(std::filesystem::absolute(std::move(working)).lexically_normal()),user(std::move(destination)),names(std::make_shared<o::RawFileName>()),files(root,{},names),
     config([this](auto name) { return files.TryRead(name); },names,o::FileSpecDirectoryHooks(root),client::MakeConfigFileReadHooks(files)),storage(root,root/L"d",user),record(clockState,user) {
    config.Registry().environment=Environment;
    config.Registry().registryValue=[this](std::string_view path) { return platform::ReadLegacyRegistryValue(path,[this] { Require(trace).Assertion("keyValue","c:\\ns\\o\\winutil.cpp",0x2db); }); }; // 실제 Load는 trace 구성 뒤 실행되며 R00481600의704+27 진단도 같은 현재 장치에서 복귀한다.
    config.SetUserFileOverride([this](std::string_view name)->std::optional<std::vector<std::uint8_t>> { // 5차 사용자 Options 우선 읽기만 적용하며 원문 나머지 Config파일 순서는 바꾸지 않는다.
        const auto slash=name.find_last_of("\\/:");if (o::AsciiLower(slash==name.npos ? name:name.substr(slash+1))!="options.cfg") return std::nullopt;const auto path=user.OptionsFile();std::error_code error;if (!std::filesystem::is_regular_file(path,error)) return std::nullopt;return o::ReadFileBytes(path);
    });
    const ProcessLogHook forwardLog=[this](auto level,auto text) { Require(trace).Log(level,text); };const ProcessAssertionHook forwardAssertion=[this](auto expression,auto file,auto line) { Require(trace).Assertion(expression,file,line); }; // 실제 같은 Trace 생성 전에는 실행되지 않으며 구성 오류도 명시 거부한다.
    ConfigurationHooks configHooks;configHooks.environment=Environment;configHooks.probe=[this](auto name) { return files.Probe(name); };configHooks.log=forwardLog;configHooks.assertion=forwardAssertion;
    TransceiverHooks transceiverHooks;ChallengeRuntimeHooks challengeHooks;sharedFiles=std::make_unique<SharedFiles>(fileTables,messages,storage,forwardLog);sharedFiles->Bind(configHooks,challengeHooks);record.Bind(configHooks,transceiverHooks);
    configuration=std::make_unique<Configuration>(config,*names,connections,network,std::move(configHooks),o::FileSpecDirectoryHooks(root));trace=std::make_unique<ProcessTrace>(connections,configuration->Values(),user);const auto log=trace->LogDevice();const auto assertion=trace->AssertionDevice();
    clock=std::make_unique<Clock>(clockState,assertion);clock->Bind(transceiverHooks);transceiverHooks.log=log;transceiverHooks.assertion=assertion;transceiverHooks.exit=[](auto code) { ::_exit(code); }; // 원문의 종료0/1은 실제 CRT 비복귀이며 예외·성공값으로 바꾸지 않는다.
    transceiverHooks.endParticipant=[this](auto token,auto participant) { Require(challenges).EndParticipant(token,participant); };transceiverHooks.formatAddress=[this](auto address) { return std::string(Require(addresses).Format(address,true)); }; // 현재 노드 주소1/0/0과 실제 CCh 종료를 같은 owner에 연결한다.
    transport=std::make_unique<TransceiverTransport>(connections,network,std::move(transceiverHooks));store=std::make_unique<ChallengeStore>(connections,*transport);addresses=std::make_unique<AddressCodec>();routing=std::make_unique<Routing>(connections,*transport,*store,mirror,log);
    pages=std::make_unique<CommandPages>(connections,storage);pages->Bind(challengeHooks);servers=std::make_unique<ServerList>(configuration->Values().rootServersChecksum,*names,storage);routing->Bind(challengeHooks);BindSchedule(challengeHooks,clockState,mainState,*routing,log);
    LoginRequestHooks loginHooks;loginHooks.log=log;loginHooks.login=[this](auto token,auto source,auto packet) { Require(challenges).Login(token,source,packet); };addresses->Bind(loginHooks,challengeHooks);login=std::make_unique<LoginRequest>(connections,*transport,*store,std::move(loginHooks));
    challengeHooks.claimSocket=[this](auto token) { return Require(login).ClaimSocket(token); };challengeHooks.log=log;challenges=std::make_unique<ChallengeRuntime>(connections,*transport,*store,configuration->Values(),messages,clockState,random,std::move(challengeHooks));
    control=std::make_unique<RootControl>(connections,*transport,configuration->Values(),*addresses,*clock,log);directory=std::make_unique<RootDirectory>(connections,*transport,configuration->Directory(),[this](auto token)->std::span<const std::uint8_t> { return Require(store).Bytes(token); });remote=std::make_unique<RemoteCommandServer>(RemoteCommandHooks{log,assertion});
    const LifecycleModules modules{connections,network,lifecycleState,*configuration,*clock,*store,*servers,*sharedFiles,storage,*control,*directory,*login,*routing,*challenges,*remote,*addresses,*transport};lifecycle=std::make_unique<Lifecycle>(modules,platform::PathToUtf8(root),log);MainLoopHooks mainHooks;mainHooks.log=log;lifecycle->Bind(mainHooks);
    main=std::make_unique<MainLoop>(connections,*clock,*transport,*store,*challenges,*routing,*sharedFiles,mainState,std::move(mainHooks)); // 이 시점부터 모든 실제 모듈/forward 수신·표와 메인 장치가 같은 장기 owner에 연결돼 있다.
}
Server::State::~State() { // 서버 stack owner는 Run이 복귀한 뒤 반납한다. 부분 구성/실행 예외에도 가짜 종료 통지를 만들지 않는다.
    main.reset();lifecycle.reset();if (transport && !transport->CallbacksActive()) transport->PacketHandlers().Registrations()->ResetCallbacks(); // 등록 함수가 참조하는 수신 모듈보다 먼저 현재 표만 반납한다.
}
Server::Server(std::filesystem::path workingDirectory,platform::UserDataRoot user):state_(std::make_unique<State>(std::move(workingDirectory),std::move(user))) {}
Server::~Server()=default;
std::int32_t Server::Run(std::span<const std::string> arguments) { return Require(state_->main).Run(arguments.size()==2 && arguments[1]=="lan"); } // 같은 메인이 banner/LAN사전통지→Lifecycle.Start/수신·프레임→정리/재시작을 수행한다.
}
