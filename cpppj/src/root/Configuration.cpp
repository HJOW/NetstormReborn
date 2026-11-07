// R의 최초/재해석 설정 순서와 기존 Config/VFS·현재 통신 전역을 접합한다.
#include "root/Configuration.h"
#include <algorithm>
#include <cctype>
#include <stdexcept>

namespace netstorm::root { // 파일 장치와 현재 전역을 바꾸지 않고 R 원문 부모만 실행한다.
namespace { // 실제 선택 장치와 NUL 경계의 소유 복사를 공유한다.
// 선택된 장치가 없으면 초기화 성공으로 넘기지 않는다.
template<class Function> const Function& Need(const Function& function) { if (!function) throw std::logic_error("R 설정의 선택된 실제 장치 미연결");return function; }
// 원문의 C문자열처럼 첫NUL까지만 소유 복사한다.
std::string CString(std::string_view value) { return std::string(value.substr(0,value.find('\0'))); }
}
Configuration::Configuration(o::ConfigInterface& config,o::RawFileName& names,ConnectionState& connections,NetworkState& network,ConfigurationHooks hooks,o::FileSpecQueryHooks fileSpec)
    :config_(config),names_(names),connections_(connections),network_(network),hooks_(std::move(hooks)),fileSpec_(std::move(fileSpec)) {}
Configuration::~Configuration() { if (installed_) config_.onChanged={}; } // 같은 R 전용 Config의 자기 변경 등록만 반납한다.
void Configuration::Assert(std::string_view expression,std::uint32_t line) { Need(hooks_.assertion)(expression,"c:\\ns\\o\\configinterface.cpp",line); }
std::optional<std::string> Configuration::Read(std::string_view key,std::uint32_t line) {
    if (!config_.Configuration().IsValid()) Assert("theConfiguration.isValid()",line);auto value=config_.Configuration().Get(key);
    if (value) { *value=CString(*value);if (value->size()>=512) throw std::length_error("R 설정의 실제512BYTE 지역 문자열 범위"); }return value;
}
void Configuration::ReadInt(std::string_view key,std::int32_t& value) { const auto text=Read(key,462);if (text) { const auto parsed=o::ConfigParseLong(*text);if (value!=parsed) value=parsed; } }
bool Configuration::ReadString(std::string_view key,std::string& value) { const auto text=Read(key,473);if (!text || *text==value) return false;value=*text;return true; }
void Configuration::ReadAllocated(std::string_view key,std::optional<std::string>& value) {
    std::string text;if (!ReadString(key,text)) return;
    if (value && value->size()==text.size() && std::equal(value->begin(),value->end(),text.begin(),[](unsigned char first,unsigned char second) { return std::tolower(first)==std::tolower(second); })) return; // 원문 _stricmp처럼 대소문자만 다른 값은 기존 문자열과 할당을 보존한다.
    value=std::move(text); // 실제 변경에만 기존 소유 문자열을 반납/교체하며 빈 옵션은 앞에서 기존 값도 보존한다.
}
void Configuration::Load(std::string_view root) {
    constexpr std::array<std::string_view,6> paths{{".\\",".\\d\\","..\\d\\","..\\..\\d\\","..\\","..\\..\\"}}; // R PE004cd760의 실제 검색 순서다.
    std::string prefix="d\\";bool found=false;
    for (const auto path:paths) { // 원문처럼 정규화한 지역 복사로 조회하고 성공 뒤 원래 고리 뷰로 FileSpec을 구성한다.
        const std::array<o::FileNamePart,3> parts{{{-2,root},{-2,path},{-3,"setup.cfg"}}};const auto formatted=names_.Format(parts);auto candidate=std::string(formatted);std::replace(candidate.begin(),candidate.end(),'/','\\');
        if (!Need(hooks_.probe)(candidate)) continue;const o::FileSpecQuery spec(formatted,fileSpec_);const auto directory=names_.Directory(spec.Text());const auto device=names_.Device(spec.Text());
        const std::array<o::FileNamePart,2> result{{{-1,device},{-2,directory}}};prefix=names_.Format(result);found=true;break;
    }
    if (!found) Assert("i<COUNTOF(searchPaths)",195); // 진단이 복귀하면 원문처럼 최초 지역 기본 d\를 사용한다.
    auto identity=Need(hooks_.environment)("IDENTITY");if (!identity || CString(*identity).empty()) identity=Need(hooks_.environment)("USER");
    std::string identityName;if (identity && !CString(*identity).empty()) { const std::array<o::FileNamePart,3> parts{{{-2,prefix},{-3,*identity},{-4,".cfg"}}};identityName=names_.Format(parts); }
    const auto name=[&](std::string_view file) { const std::array<o::FileNamePart,2> parts{{{-2,prefix},{-3,file}}};return std::string(names_.Format(parts)); }; // 원문 지역 배열처럼 각 경로를 고리 재사용 전에 즉시 복사한다.
    const auto options=name("options.cfg"),user=name("user.cfg"),dev=name("dev.cfg"),setup=name("setup.cfg"),demo=name("demo.cfg"),guild=name("guild.cfg");static_cast<void>(demo); // demo는 한 칸 작성만 하고 실제 Load 인자로 넘기지 않는다.
    const std::array<std::string,9> arguments{{"","MAJOR_VERSION=10;MINOR_VERSION=71;version=\"v10.71\";","@",identityName,user,options,dev,guild,setup}};
    config_.onChanged=[this] { Interpret(); };installed_=true;config_.Load(arguments);dataPrefix_=prefix; // R은 InstallDir/CDDir을 Set하지 않아 최초 해석 callback도 여기서 추가 호출하지 않는다.
}
void Configuration::Interpret() {
    Need(hooks_.reloadModeration)(); // 실제 blacklist/ipban/chatlist/두 채팅 표/filter 부모가 끝난 뒤 옵션을 읽는다.
    ReadInt("multlinelimit",values_.multiLineLimit);ReadInt("rootServerTraceLevel",values_.traceLevel);ReadInt("rootServerThrowExceptionOnAssert",values_.throwOnAssert);ReadInt("rootServerFlushAfterTrace",values_.flushAfterTrace);ReadInt("rootServerTraceToStdErr",values_.traceToStdErr);
    ReadInt("rootServerRootTransceiverTimeout",network_.mainTimeout);ReadInt("rootServerPendingTransceiverTimeout",network_.pendingTimeout);ReadInt("rootServerChalTransceiverTimeout",network_.challengeTimeout);ReadInt("rootDiagTimeout",values_.diagnosticTimeout);
    ReadInt("useipreplace",values_.useIPReplace);ReadAllocated("chalipreplace",values_.ipReplace);ReadAllocated("chalipfind",values_.ipFind);
    std::optional<std::string> admin;ReadAllocated("clientadmin",admin);if (!admin) throw std::out_of_range("R clientadmin 원문 NULL strcpy의 물리 읽기 거부");values_.clientAdmin=*admin;
    ReadInt("chalServerBattleDeadTime",values_.battleDeadTime);ReadAllocated("chalServerMOD",values_.mod);ReadAllocated("chalServerAdminContact",values_.adminContact);ReadAllocated("chalServerBadVersionMessage",values_.badVersionMessage);ReadAllocated("chalServerBadVerNoBattle",values_.badVersionNoBattle);
    ReadInt("chalServerBadMajorVersion",values_.badMajorVersion);ReadInt("chalServerBadMinorVersion",values_.badMinorVersion);static_cast<void>(ReadString("chalServerPassword",values_.password));ReadInt("chalServerLimitPirates",values_.legacyLimitPirates); // 폐기 클라이언트 검증 소비자는 연결하지 않는다.
    ReadInt("chalServerGroupByLevel",values_.groupByLevel);ReadInt("chalKeepAliveTimeout",values_.keepAliveTimeout);ReadInt("chalKeepAliveFreq",values_.keepAliveFrequency);ReadInt("maxChalPlayersPerZone",values_.maxPlayers);values_.maxPlayers=std::clamp(values_.maxPlayers,0,39);
}
void Configuration::Startup(std::string_view root) {
    Load(root);Interpret();ReadInt("useIPAsLanProtocol",values_.useIPAsLanProtocol);ReadInt("rootServerTraceLevel",values_.traceLevel);Need(hooks_.log)(0,"TraceLevel="+std::to_string(values_.traceLevel)+"\n");
    ReadInt("rootServerThrowExceptionOnAssert",values_.throwOnAssert);ReadInt("rootServerFlushAfterTrace",values_.flushAfterTrace);ReadInt("chalServerKillLowVersions",values_.killLowVersions);Need(hooks_.log)(0,"chalServerKillLowVersions="+std::to_string(values_.killLowVersions)+"\n");Need(hooks_.log)(0,"FlushAfterTrace="+std::to_string(values_.flushAfterTrace)+"\n");
    ReadInt("rootServerTraceToStdErr",values_.traceToStdErr);ReadInt("rootServerBasePort",values_.basePort);ReadInt("rootTelnetPort",values_.telnetPort);ReadInt("rootServerBroadcastPort",values_.broadcastPort);ReadInt("rootServerShutdownTime",network_.emptyExitTimeout);ReadInt("autobancheaters",values_.autoBanCheaters);
    ReadInt("rootServerRootTransceiverTimeout",network_.mainTimeout);ReadInt("rootServerPendingTransceiverTimeout",network_.pendingTimeout);ReadInt("rootServerChalTransceiverTimeout",network_.challengeTimeout);static_cast<void>(ReadString("rootServerZacketRecordFile",values_.recordFile));
    if (values_.recordFile.size()>=80) throw std::length_error("R 기록 파일 이름의 실제80BYTE 전역 범위");if (!values_.recordFile.empty()) Need(hooks_.openRecord)(values_.recordFile);
    ReadInt("ChalSpamTime",values_.spamTime);ReadInt("ChalSpamNum",values_.spamCount);ReadInt("chalServerMakeFakeBattles",values_.fakeBattles);ReadInt("chalServerMakeFakePlayers",values_.fakePlayers);ReadInt("chalServerBattleDeadTime",values_.battleDeadTime);
    ReadAllocated("chalServerMOD",values_.mod);ReadAllocated("chalServerAdminContact",values_.adminContact);ReadAllocated("chalServerBadVersionMessage",values_.badVersionMessage);ReadAllocated("chalServerBadVerNoBattle",values_.badVersionNoBattle);
    ReadInt("chalServerBadMajorVersion",values_.badMajorVersion);ReadInt("chalServerBadMinorVersion",values_.badMinorVersion);static_cast<void>(ReadString("chalServerPassword",values_.password));ReadInt("chalServerLimitPirates",values_.legacyLimitPirates);
    ReadInt("rootServerNumChalStatesWIN32",connections_.challengeStateCount);Need(hooks_.log)(0,"rootServerNumChalStates = "+std::to_string(connections_.challengeStateCount)+"\n");ReadInt("rootServerNumChalStatesPerProcess",values_.challengeStatesPerProcess);
    InterpretMode(connections_,[this](auto key,auto& value) { ReadInt(key,value); }); // 원문 세 임시0 옵션/OR·LAN1/마지막개수+1은 기존 같은 부모를 쓴다.
}
}
