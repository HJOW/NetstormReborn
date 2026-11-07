// R의 실제 초기 옵션과 재해석을 같은 현재 연결/네트워크/Root 목록 전역에 연결한다.
#pragma once
#include "root/RootDirectory.h"
#include "o/ConfigInterface.h"
#include "o/RawFileName.h"

namespace netstorm::root { // R 전용 설정 수명이며 게임 Client의 시작 부작용을 실행하지 않는다.
struct Options { // 제공된 R PE data 초기값과 BSS0을 구별한 현재 옵션 저장소다.
    std::int32_t useIPAsLanProtocol{},traceLevel{29},throwOnAssert{1},flushAfterTrace{},killLowVersions{},traceToStdErr{}; //004d21d0/cd8b0/cd958/d8920/d21c8/d8924다.
    std::int32_t basePort{},telnetPort{6801},broadcastPort{},autoBanCheaters{1}; //004d21c0/cd7f4/d21c4/cd800다. 기본 포트를 임의 추정하지 않는다.
    std::int32_t spamTime{2},spamCount{3},fakeBattles{},fakePlayers{},battleDeadTime{}; //004cd7ec/f0/d05a8/ac/b0의 현재 값이다.
    std::int32_t badMajorVersion{6},badMinorVersion{},legacyLimitPirates{1},challengeStatesPerProcess{}; //004cd524/d05cc/cd520/d21e4다. legacyLimitPirates는 기록 전용이며 폐기 검증 소비자를 연결하지 않는다.
    std::int32_t multiLineLimit{6},diagnosticTimeout{90},useIPReplace{},keepAliveTimeout{},keepAliveFrequency{15}; //004cd7e8/fc/d21d4/f4/cd7f8다.
    std::int32_t groupByLevel{},maxPlayers{}; //004d21f8/fc이며 재해석 마지막에 maxPlayers를0..39로 제한한다.
    std::optional<std::string> mod,ipReplace,ipFind,adminContact,badVersionMessage,badVersionNoBattle; //004d05b4/b8/bc/c0/d0/d4의 NULL/할당 문자열을 구별한다. 빈 옵션은 기존 할당도 보존한다.
    std::string clientAdmin,password,recordFile; //004d2054/d83a8/d2168의 C문자열 값이다. 호스트 소유 문자열로 보관하며 원문 포인터/배열 ABI로 쓰지 않는다.
    std::uint32_t rootServersChecksum{}; //004d0e8c의 별도 BSS0이며 실제 서버목록 부모가 갱신할 값이다. 옵션으로 덮지 않는다.
};
struct ConfigurationHooks { // 선택된 실제 장치만 요구하며 미연결을 성공한 초기화로 처리하지 않는다.
    std::function<std::optional<std::string>(std::string_view)> environment; // IDENTITY 뒤 USER를 실제 프로세스 환경에서 읽는다.
    std::function<std::uint32_t(std::string_view)> probe; // 정규화한 여섯 setup.cfg 후보의 실제 존재 조회다.
    std::function<void()> reloadModeration; //00477570 선두의 blacklist/ipban/chatlist/guild/guildreq/filter 로드를 실제 관리자가 수행한다.
    std::function<void(std::string_view)> openRecord; //00482720의 실제 기록 파일 열기다. 사용자 홈 쓰기 정책은 이 장치가 지켜야 한다.
    std::function<void(std::int32_t,std::string_view)> log; // 선택된 원문 level/완성된 출력 메시지다.
    std::function<void(std::string_view,std::string_view,std::uint32_t)> assertion; // 실제 선택된 configinterface/r 진단과 복귀 뒤 쓰기를 보존한다.
};
class Configuration final { // 실제 ConfigInterface/고리/전역은 이 부모와 등록 callback보다 오래 살아야 한다.
public:
    Configuration(o::ConfigInterface& config,o::RawFileName& names,ConnectionState& connections,NetworkState& network,ConfigurationHooks hooks,o::FileSpecQueryHooks fileSpec={}); // 생성은 읽기/쓰기·OS/네트워크 작업을 하지 않는다.
    ~Configuration(); // 전용 Config의 자기 변경 callback을 반납해 C++ owner 종료 뒤 호출을 막는다.
    Configuration(const Configuration&)=delete;Configuration& operator=(const Configuration&)=delete; // this를 빌린 변경 callback과 현재 참조를 복제하지 않는다.
    void Startup(std::string_view root); //004700b0→00477570→00477950의 실제 최초 옵션/모드 해석 부모다. root는 호스트의 가상 CWD다.
    void Interpret(); //00477570의 재해석 대상만 현재 Config에서 다시 읽는다. 최초 전용 포트/모드를 재설정하지 않는다.
    Options& Values() { return values_; }const Options& Values() const { return values_; } // 실제 관리자/시계/진단이 같은 현재 옵션을 빌린다.
    RootDirectoryGlobals Directory() { return {values_.groupByLevel,values_.maxPlayers,values_.rootServersChecksum}; } // Root 응답이 재해석 뒤의 같은 DWORD를 읽도록 참조만 빌린다.
    std::string_view DataPrefix() const { return dataPrefix_; } //004d2218의 실제 설정 폴더이며 Load가 끝난 뒤 갱신된다.
private:
    o::ConfigInterface& config_;o::RawFileName& names_;ConnectionState& connections_;NetworkState& network_; // 사본 없는 실제 공용 설정/경로 고리와 R 전역이다.
    ConfigurationHooks hooks_;o::FileSpecQueryHooks fileSpec_;Options values_; // 실제 프로세스 장치와 검색 수명/옵션 저장소다.
    std::string dataPrefix_;bool installed_{}; //004d2218의 최초 BSS 빈 문자열과 C++ callback 등록 수명이다.
    void Load(std::string_view root); // R의 여섯 탐색/환경 이름/경로 작성·읽기 순서를 ConfigInterface::Load에 연결한다.
    std::optional<std::string> Read(std::string_view key,std::uint32_t line); //00470da0/e60의 유효성 진단/현재 치환/512BYTE 지역 물리 경계다.
    void ReadInt(std::string_view key,std::int32_t& value); // 키가 있으면 원문 atoi DWORD 결과가 다를 때만 쓴다.
    bool ReadString(std::string_view key,std::string& value); // 대소문자를 구분해 현재 C문자열이 바뀐 경우만 쓴다.
    void ReadAllocated(std::string_view key,std::optional<std::string>& value); //00470f20의 빈 값/대소문자만 다른 값 보존과 NULL/할당 수명이다.
    void Assert(std::string_view expression,std::uint32_t line); // 원문 configinterface.cpp의 실제 줄을 프로세스 진단에 전달한다.
};
}
