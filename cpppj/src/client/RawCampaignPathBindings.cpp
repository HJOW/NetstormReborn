// 기존 원본 복원 몸체를 조립하며 별도 좌표 캐시·지면 사본·걷기 loop를 만들지 않는다.
#include "client/RawCampaignPathBindings.h"
#include "o/RawGameAuxState.h" // 같은 현재00540c68 로그 수준을 수신 trace에서 읽는다.
#include <bit>
#include <cstdio>
#include <stdexcept>
#include <utility>

namespace netstorm::client {
namespace {
constexpr std::uint32_t kActualPriestProcess=54; // 기존 RawPathStart가 복원한 사제 이동 ProcessForm 타입 번호다.
// 정렬되지 않은 raw vtable DWORD를 실제 판본 주소 기록값으로 읽는다.
std::uint32_t Vtable(std::span<const std::uint8_t> raw) {
    std::uint32_t result=0;
    // 정확4바이트만 낮은 바이트부터 조합한다.
    for (std::size_t index=0;index<4;++index) result|=static_cast<std::uint32_t>(raw[index])<<(8*index);
    return result;
}
}
// 같은 월드 자료/모듈을 확인한 뒤 실제 Start callback을 구성한다.
RawCampaignPathBindings::RawCampaignPathBindings(RawSurfaceWorld& world,RawWorldCore core,const RawPriestWorldBindings& priests,
    const o::RawPriestHalt& halt,const o::RawPriestArrival& arrival,const o::RawPathCleanup& cleanup,
    const o::RawContainedLookup& lookup,o::RawPathStartState& startState,o::RawPathProcessState& processState,
    o::RawPathSearchState& searchState,RawCampaignPathEffects effects)
    :world_(world),core_(core),priests_(priests),halt_(halt),arrival_(arrival),cleanup_(cleanup),lookup_(lookup),
     startState_(startState),processState_(processState),effects_(std::move(effects)),
     search_(core.pool,core.hash,core.spots,core.types,core.frames,searchState,&world.Storage()->pathSearchWork) {
    const auto current=world_.Core();const auto* pool=&core_.pool;
    if (&current.pool!=pool || &current.hash!=&core_.hash || current.spots.data()!=core_.spots.data() ||
        current.types.data()!=core_.types.data() || current.frames.data()!=core_.frames.data() || current.shapes.data()!=core_.shapes.data() ||
        &current.processes!=&core_.processes || &current.processState!=&core_.processState || &current.kernel!=&core_.kernel ||
        &priests_.PriestState().Pool()!=pool || &halt_.Pool()!=pool || &arrival_.Pool()!=pool || &cleanup_.Pool()!=pool || &lookup_.Pool()!=pool ||
        &startState_.clock!=&core_.processState || &processState_.clock!=&core_.processState ||
        startState_.priestType!=o::kPriestType || processState_.priestType!=o::kPriestType || processState_.priestProcess!=kActualPriestProcess ||
        !effects_.metadata || !effects_.soundId || !effects_.stepSound || !effects_.startBlocked || !effects_.arrivalBlocked || !effects_.retarget)
        throw std::invalid_argument("캠페인 사제 경로의 같은 월드/시계/실제54/필수 효과 연결 오류");
    o::RawPathStartHooks hooks;
    hooks.processType=[this](std::uint32_t type) { return priests_.MovementProcessType(type); };
    hooks.create=[this](bool priest) { return Create(priest); };hooks.metadata=effects_.metadata;hooks.clearBlocked=effects_.startBlocked;
    hooks.createType=[this](std::uint32_t type) { return CreateType(type); };
    start_=std::make_unique<o::RawPathStart>(core_.pool,core_.hash,core_.types,core_.processes,lookup_,core_.frame,startState_,std::move(hooks));
}
// 외부 runtime은 같은 Kernel을 먼저 비운다. 이 해제에서 임의 새 공간 삭제를 수행하지 않는다.
RawCampaignPathBindings::~RawCampaignPathBindings()=default;
// 실제 사제 생성자가 설치한 판본별 가상 표를 사용한다.
void RawCampaignPathBindings::ValidatePriest(o::Sid parent) const {
    if (parent.value<5 || parent.value>=core_.pool.Capacity()) throw std::out_of_range("캠페인 사제 명령 SID 범위");
    const auto raw=core_.pool.AllocatedBytes(parent);const auto expected=core_.pool.Edition()==o::OriginalEdition::Patch1078 ? o::kPatchPriestVtable : o::kCdPriestVtable;
    if (raw[10]!=startState_.priestType || Vtable(raw)!=expected || (raw[11]&9))
        throw std::invalid_argument("캠페인 사제 명령의 실제 타입/가상 표/살아 있는 자산 오류");
    if (priests_.MovementProcessType(raw[10])!=kActualPriestProcess || processState_.priestProcess!=kActualPriestProcess)
        throw std::logic_error("캠페인 사제의 다른 mutable 이동 타입 미복원 경계");
}
// 실제54만 생성하며 같은 공간과 부모 몸체의 효과 뒤 조회 순서는 기존 Process가 담당한다.
std::unique_ptr<o::RawPathProcess> RawCampaignPathBindings::Create(bool priest) {
    return CreateType(priest ? processState_.priestProcess : processState_.teleportProcess);
}
// 현재 선택된 실제 가상 표의 Init/frame/Attach를 조립한다.41은 독립 회전/보행을 사용한다.
std::unique_ptr<o::RawPathProcess> RawCampaignPathBindings::CreateType(std::uint32_t type) {
    const bool priest=type==processState_.priestProcess,flying=type==processState_.flyProcess,worm=type==processState_.wormProcess,balloon=type==processState_.balloonProcess,dude=type==processState_.dudeProcess,bulf=type==processState_.bulfProcess,walker=type==processState_.walkerProcess;
    if (!priest && !flying && !worm && !balloon && !dude && !bulf && !walker && type!=processState_.teleportProcess) throw std::logic_error("다른 캠페인 경로 생성자 미연결");
    return CreateVariant(priest,flying,worm,balloon,dude,bulf,walker);
}
// 함수 주소가 고정한 클래스를 현재 프로세스 번호 비교로 다시 바꾸지 않는다.
std::unique_ptr<o::RawPathProcess> RawCampaignPathBindings::CreateByConstructor(std::uint32_t address) {
    if (core_.pool.Edition()!=o::OriginalEdition::Patch1078) return nullptr;
    switch (address) {
    case 0x48be40: return CreateVariant(false,false,false,false,false,false,false);
    case 0x48be70: return CreateVariant(false,false,false,true,false,false,false);
    case 0x48bea0: return CreateVariant(false,true,false,false,false,false,false);
    case 0x48bed0: return CreateVariant(false,false,false,false,false,false,true);
    case 0x48bf00: return CreateVariant(false,false,false,false,false,true,false);
    case 0x48bf30: return CreateVariant(false,false,false,false,true,false,false);
    case 0x48bf60: return CreateVariant(true,false,false,false,false,false,false);
    case 0x48bf90: return CreateVariant(false,false,true,false,false,false,false);
    default: return nullptr;
    }
}
// 기존 명령과 직접 cdecl 생성자가 같은 월드/cleanup/halt/arrival 장치를 사용한다.
std::unique_ptr<o::RawPathProcess> RawCampaignPathBindings::CreateVariant(bool priest,bool flying,bool worm,bool balloon,bool dude,bool bulf,bool walker) {
    o::RawPathProcessHooks hooks;
    hooks.assertion=effects_.assertion; // 같은 현재 Client 장치를 경로의 독립 수명 복귀 부모로 복사한다.
    hooks.unpop=[this](o::Sid sid,std::uint32_t flags) { world_.Unpop(sid,flags); };
    hooks.repop=[this](o::Sid sid,std::uint32_t flags) { world_.Repop(sid,flags); };
    hooks.pop=[this](o::Sid sid,o::RawPathPoint point,std::uint32_t flags) { world_.Pop(sid,point.x,point.y,flags); };
    hooks.halt=[this,priest,worm,balloon,dude,bulf,walker](o::Sid sid) { if (dude) { if (!effects_.dudeHalt) throw std::logic_error("실제 Dude 부모 정지 미연결");return effects_.dudeHalt(sid); }if (priest) return halt_.Halt(sid);if (worm || balloon || bulf || walker) { ValidateCarrier(sid);if (walker && core_.pool.Slot(sid)[10]==processState_.rainWalkerType) { if (!effects_.crabHalt) throw std::logic_error("크리스탈 크랩42ab80 미연결");return effects_.crabHalt(sid); }return halt_.CommonHalt(sid); }ValidateConstruction(sid);return 10U; }; // Bulf의 부모 VT도 실제42a5f0이며 Dude+38과 구분한다.
    hooks.arrival=[this,priest,worm,balloon,dude,bulf,walker](o::Sid sid,std::uint32_t first,std::uint32_t second) { if (dude) { if (!effects_.dudeArrival) throw std::logic_error("실제 Dude 부모 도착 미연결");return effects_.dudeArrival(sid); }if (priest) return arrival_.Arrival(sid,first,second);if (worm || balloon || bulf || walker) { ValidateCarrier(sid);if (!effects_.carrierArrival) throw std::logic_error("실제 Carrier 도착 미연결");return effects_.carrierArrival(sid,first,second); }ValidateConstruction(sid);if (!effects_.constructionArrival) throw std::logic_error("실제 건설 운반 도착 미연결");return effects_.constructionArrival(sid,first,second); };
    hooks.soundId=effects_.soundId;hooks.stepSound=effects_.stepSound;hooks.clearBlocked=effects_.arrivalBlocked;
    hooks.retarget=effects_.retarget;hooks.observationStamp=effects_.observationStamp;
    hooks.balloonFlame=effects_.balloonFlame;
    hooks.bulfRandom=effects_.bulfRandom;hooks.bulfSound=effects_.bulfSound;
    hooks.cleanup=[this](o::RawPathRuntime& runtime,o::Sid parent,std::uint32_t flags) {
        (void)flags;cleanup_.AfterSpatial(runtime,parent);
    };
    hooks=o::MakePathSearchHooks(core_.pool,search_,std::move(hooks));
    return std::make_unique<o::RawPathProcess>(core_.pool,core_.hash,core_.types,core_.frames,core_.shapes,core_.spots,processState_,std::move(hooks),priest,flying,worm,balloon,dude,bulf,walker);
}
// 가상 주소를 호스트에서 실행하지 않고 두 실제 부모의 생성자 표만 허용한다.
void RawCampaignPathBindings::ValidateConstruction(o::Sid parent) const {
    if (core_.pool.Edition()!=o::OriginalEdition::Patch1078 || parent.value<5 || parent.value>=core_.pool.Capacity()) throw std::invalid_argument("건설 운반 경로 판본/SID 오류");
    const auto raw=core_.pool.AllocatedBytes(parent);const auto table=Vtable(raw);
    if ((raw[11]&9) || !((raw[10]==startState_.teleportType && table==0x5070c0 && priests_.MovementProcessType(raw[10])==processState_.teleportProcess) || (raw[10]==startState_.manaBoltType && table==0x509810 && priests_.MovementProcessType(raw[10])==processState_.flyProcess))) throw std::logic_error("건설 운반의 실제 타입/VT/이동 종류 오류");
}
// 사제 목적지 스냅을 적용하지 않고 기존 Start의 실제 종류/flags10 분배를 사용한다.
o::RawPathProcess* RawCampaignPathBindings::StartConstruction(o::RawPathStartRequest request) {
    ValidateConstruction(request.parent);if (!effects_.constructionArrival) throw std::logic_error("건설 운반의 실제 도착 소비자 미연결");return start_->Start(request);
}
// 보호 PE의 부모135/503e50도 같은 Halt/도착/시작 통지를 사용하지만 경로는 고유41이다.
void RawCampaignPathBindings::ValidateCarrier(o::Sid parent) const {
    if (core_.pool.Edition()!=o::OriginalEdition::Patch1078 || parent.value<5 || parent.value>=core_.pool.Capacity()) throw std::invalid_argument("웜40 부모 판본/SID 오류");
    const auto raw=core_.pool.AllocatedBytes(parent);const auto table=Vtable(raw);
    const auto movement=priests_.MovementProcessType(raw[10]);const bool worm=raw[10]==processState_.sunWalkerType && table==0x50e568U && movement==processState_.wormProcess; //기존 골렘의 정확한 부모다.
    const bool balloon=(((raw[10]==111 || raw[10]==141) && table==0x5040c0U) || (raw[10]==146 && table==0x504198U)) && movement==processState_.balloonProcess; // 원본 벌룬/에어십과 전용 애니메이션을 더하는 클라우드 플로터도 같은42 이동이다.
    const bool bulf=raw[10]==135 && table==0x503e50U && movement==processState_.bulfProcess; // 실제42afb0 생성자가 설치한 부모 표다.
    const bool walker=((raw[10]==processState_.windWalkerType && table==0x503d80U) || (raw[10]==processState_.rainWalkerType && table==0x503f20U)) && movement==processState_.walkerProcess; // 실제137/156 부모의38 경로다.
    if ((raw[11]&9) || (!worm && !balloon && !bulf && !walker)) throw std::logic_error("Carrier 실제 부모 타입/가상 표/이동 종류 오류: empty42ac60은 별도 미연결");
}
// 원본 생성/Init/Prepare/Attach와 부모 통지를 생략하지 않는다.
o::RawPathProcess* RawCampaignPathBindings::StartCarrier(o::RawPathStartRequest request) { ValidateCarrier(request.parent);return start_->Start(request); }
// Start의 명령 DWORD/배열/Prepare/기존 경로 교체/부모 통지를 그대로 실행한다.
o::RawPathProcess* RawCampaignPathBindings::StartPriest(o::RawPathStartRequest request) {
    ValidatePriest(request.parent);return start_->Start(request);
}
// 48bab0의 현재 부모 검사와 복원한 두 부모 가상 효과를 같은 풀에 한정한다.
void RawCampaignPathBindings::ValidateDude(o::Sid parent) const {
    if (core_.pool.Edition()!=o::OriginalEdition::Patch1078 || parent.value<5 || parent.value>=core_.pool.Capacity()) throw std::invalid_argument("Dude24 부모 판본/SID 오류");
    const auto raw=core_.pool.AllocatedBytes(parent);
    if (raw[10]!=processState_.dudeType || raw[10]!=startState_.dudeType || Vtable(raw)!=0x508cc8U || (raw[11]&9U) || processState_.dudeProcess!=24 || priests_.MovementProcessType(raw[10])!=processState_.dudeProcess) throw std::logic_error("Dude24 실제 부모/가상 표/이동 종류 오류");
}
// 같은 Start가 기존 경로 취소·현재 metadata·실제 로컬 Form 부착을 담당한다.
o::RawPathProcess* RawCampaignPathBindings::StartDude(o::RawPathStartRequest request) {
    ValidateDude(request.parent);if (!effects_.dudeHalt || !effects_.dudeArrival) throw std::logic_error("Dude24 실제 부모 효과 미연결");return start_->Start(request);
}
// lookup callback이나 삭제 뒤 오래된 포인터를 저장하지 않고 매 호출 현재 Form을 다시 찾는다.
o::RawPathProcess* RawCampaignPathBindings::CurrentPath(o::Sid parent) const {
    if (parent.value<5 || parent.value>=core_.pool.Capacity()) throw std::out_of_range("캠페인 현재 Path 부모 SID 범위");
    const auto raw=core_.pool.Slot(parent);std::uint32_t movement=kActualPriestProcess;
    if (raw[10]!=startState_.priestType) { if (raw[10]==startState_.dudeType) ValidateDude(parent);else if (raw[10]==processState_.sunWalkerType || raw[10]==processState_.emptyType || raw[10]==111 || raw[10]==141 || raw[10]==146 || raw[10]==135 || raw[10]==processState_.windWalkerType || raw[10]==processState_.rainWalkerType) ValidateCarrier(parent);else ValidateConstruction(parent);movement=priests_.MovementProcessType(raw[10]); } // 40h 송수신도 같은 클라우드 플로터의 실제42 경로를 사용한다.
    const auto form=lookup_.Find(parent,movement);if (!form.value) return nullptr;
    auto* process=dynamic_cast<o::RawPathProcess*>(core_.processes.FromForm(form));
    if (!process || &process->Pool()!=&core_.pool || process->Parent()!=parent || process->Form()!=form)
        throw std::logic_error("캠페인 현재54 Form/실제Host Path 계약 오류");
    return process;
}
// 동일 Path 객체의 저장소만 빌리고 명령/좌표 사본은 만들지 않는다.
o::RawPathRuntime* RawCampaignPathBindings::CurrentRuntime(o::Sid parent) const {
    auto* process=CurrentPath(parent);return process ? &process->Runtime() : nullptr;
}
// 기존 Carrier setter의 같은 두 DWORD 쓰기를 공유한다.
bool RawCampaignPathBindings::SetTargets(o::Sid parent,std::uint32_t first,std::uint32_t second) const {
    auto* runtime=CurrentRuntime(parent);if (!runtime) return false;runtime->SetTargets(first,second);return true;
}
// callback이 현재 Path를 삭제할 수 있으므로 호출 뒤 포인터를 다시 읽지 않는다.
bool RawCampaignPathBindings::RetargetCurrent(o::Sid parent) const {
    auto* runtime=CurrentRuntime(parent);if (!runtime) return false;effects_.retarget(*runtime,parent);return true;
}
// 실제 Host WriteData가 원래 buffer에 쓴 뒤 caller의 같은 전송 경계를 실행한다.
o::ProcessFormTransmission RawCampaignPathBindings::Transmission(o::RawPathDataHeader& header,std::span<std::uint8_t> buffer,
    std::function<void(std::int32_t)> transport,o::ProcessFormTransmission other) const {
    return o::MakePathFormTransmission(core_.pool,core_.types,header,buffer,std::move(transport),std::move(other));
}
// root가 소유한 모든 도착/운반 모듈은 이 풀 참조로 혼합 연결을 확인할 수 있다.
const o::SidPool& RawCampaignPathBindings::Pool() const { return core_.pool; }
// 원문은free Form만 새 경로로 복원하며 live Form은 상세 진단 뒤 그대로 반환한다.
void RawCampaignPathBindings::ReceivePacket(std::span<const std::uint8_t> packet) {
    const auto storage=world_.Storage();if (core_.pool.Edition()!=o::OriginalEdition::Patch1078) throw std::invalid_argument("Path 수신의 실제 PATCH 연결 필요");
    if (packet.size()<6) throw std::out_of_range("Path 수신의 실제 Form WORD 부족");if (core_.processState.Boss()) storage->AssertDisplay("!boss","\\Ns\\O\\Pathprocess.cpp",0x224);
    const auto word=[&](std::size_t at) { if (at>packet.size() || 2U>packet.size()-at) throw std::out_of_range("Path 수신의 선택된 WORD 부족");return static_cast<std::uint16_t>(std::uint16_t(packet[at])|(std::uint16_t(packet[at+1])<<8)); }; // 현재 실제 WORD만 읽으며 헤더 길이를 대신 사용하지 않는다.
    const o::Sid form{word(4)};if (!form.value || form.value>=core_.pool.Capacity()) throw std::out_of_range("Path 수신의 명시 Form 물리 범위");const auto raw=core_.pool.Slot(form);
    if (!(raw[11]&1U)) { const auto type=raw[10];if (type>=core_.types.size()) throw std::out_of_range("Path 기존 Form의 현재 TYPE 범위");std::array<char,256> detail{};std::snprintf(detail.data(),detail.size(),"Receiving an invalid update for a path process. Client thinks the form is a %s.\n",core_.types[type].name.c_str());storage->RecordAssertionDetail(detail.data());storage->AssertDisplay("0","\\Ns\\O\\Pathprocess.cpp",0x24f);return; }
    const o::Sid parent{word(6)};if (parent.value>=core_.pool.Capacity()) throw std::out_of_range("Path 수신의 부모 물리 범위");const auto movement=priests_.MovementProcessType(core_.pool.Slot(parent)[10]);if (movement>=core_.types.size()) throw std::out_of_range("Path 수신의 현재 이동 TYPE 범위");auto pending=CreateByConstructor(core_.types[movement].constructorAddress);if (!pending) throw std::logic_error("Path 수신의 실제 TYPE 생성자 미연결");
    o::RawPathDataReadHooks hooks;hooks.stagePoint=effects_.stagePoint;hooks.metadata=[this](o::Sid sid) { const auto type=effects_.metadata(sid);return std::array<float,2>{type.speed,type.turningSpeed}; };hooks.now=[this] { return core_.processState.Now(); };
    hooks.assertion=[storage](auto expression,auto file,auto line) { storage->AssertDisplay(expression,file,line); };hooks.trace=[storage](auto text) { storage->TraceGraph(storage->GameAux()->PathTraceLevel(),text); }; // 현재99와 실제 공통 로그 임계값을 각각 소비한다.
    hooks.clearAnimation=[this](o::Sid sid) { if (auto* process=core_.processes.FindEvent(sid,0x5105)) core_.processes.Kill(*process,0x40); }; //426d90의 첫 해당 Regular만 삭제한다.
    o::RawPathData(core_.pool.Edition()).Read(pending->Runtime(),packet,core_.pool,core_.types,hooks);
    const auto count=pending->Runtime().Count();const auto first=count ? pending->Runtime().Point(0) : o::RawPathPoint{}; // 부착이 부모 free/dead로 거절되면 원문의 미소유 객체 값만 보존한다.
    auto* attached=o::RawPathProcess::Attach(core_.processes,std::move(pending),parent,0,form);if ((attached ? attached->Runtime().Count() : count)!=0) { world_.Unpop(parent,0);const auto point=attached ? attached->Runtime().Point(0) : first;world_.Pop(parent,point.x,point.y,0); } // 기존 가상 Attach의 종류별 효과 뒤 현재 첫점으로 부모를 재배치한다.
}
// 의식의 추가 Prepare/끝점/깊이 처리가 기존 현재 시작 몸체를 재사용한다.
o::RawPathStart& RawCampaignPathBindings::Starter() const { return *start_; }
// 기존 경로를 지우거나 새 경로를 부착하지 않고 원본 후보용 임시 Path만 채운다.
bool RawCampaignPathBindings::SearchCandidate(o::RawPathRuntime& temporary,o::Sid parent,o::RawPathPoint start,o::RawPathPoint destination,std::uint32_t flags) { return search_.Search(temporary,parent,start,destination,flags); }
}
