// NSInfo가 읽는 실제 현재 전역과 서버 이름 버퍼를 Client 수명에 보관한다.
#pragma once
#include <array>
#include <cstdint>
#include <functional>
#include <span>
#include <string>
#include <string_view>
#include <vector>
namespace netstorm::client {
struct ClientNsInfoGlobals {
    std::uint32_t current{},zone{}; //593150/5467b8의 현재 NSInfo 포인터 토큰과 zone DWORD다.
    std::array<std::uint32_t,40> rosterOrder{}; //005930b0..314f의실제정렬40DWORD이며목록갱신은1..39만쓴다.같은행입력도이표를빌린다.
    std::int32_t serverSelection{}; //54db4c의 BSS0이며 Interpret Options의 curServer가 갱신한다.
    std::int32_t passedDiags{}; //0054db70의 BSS0/현재 passedDiags 옵션이며 메타 요청의 진단 정보 비트를 쓴다.
    std::int32_t rootServerBroadcastPort{}; //0054db54의BSS0/InterpretOptions현재DWORD이며검색은Config문자열을다시파싱하지않고이값을읽는다.
    std::array<char,80> serverName{}; //594970의 실제 공유80BYTE이며 무효 서버 조회는 기존 바이트를 보존한다.
    std::vector<std::string> servers; //532714/71c의 같은 현재 서버 행이며 Root0Fh의 갱신/저장도 이 목록을 사용한다.
    std::uint32_t serverListChecksum{}; //00532724의 현재 바이트 합이며 기존 행 교체는 원문처럼 재계산하지 않는다.
    std::uint8_t nsKind{6},playerKind{37},battleKind{60}; //540421/440/457의 보호10.78 초기 종류 BYTE다.
    std::uint32_t zoneFormat{0x50c894}; //540bac의 실제 초기 형식 문자열 포인터 토큰이다.
    void LoadServerNames(std::span<const std::uint8_t> file); // 시작 서버 파일의 현재 행을 읽는다. 보호 파일에 쓰지 않는다.
    std::string_view ResolveServerName(const std::function<std::string(std::string_view)>& translate); //47c430→유효 행58f90/58e70의 현재 이름 출력 경계다.
    void ReadServerFields(std::int32_t index,std::span<char> endpoint,std::span<char> name,std::int32_t* subscriber) const; //00458f90의 유효 인덱스에서만 실제 행 parser를 실행한다. 무효이면 기존 출력도 보존한다.
    void UpsertServer(std::string_view row,bool preserveValue); //00458fe0의 소문자 endpoint 부분검색/기존 값 보존·새 행 추가 시에만 전체 체크섬 갱신이다.
    void SetServerValue(std::int32_t index,std::uint32_t value); //004594a0의 유효 현재 행 값을 signed DWORD 십진 문자열로 교체한다.
    void RecalculateServerChecksum(); //00458de0의 각 unsigned 문자+1 합계를 같은 현재 DWORD에 쓴다.
};
}
