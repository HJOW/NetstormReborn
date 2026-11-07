// R 프로세스의 현재 로그/진단 전역을 같은 서버 상태와 사용자 홈 장치에 연결한다.
#pragma once
#include "root/Configuration.h"
#include "platform/UserDataRoot.h"
#include <cstdio>

namespace netstorm::root { //004800e0/4800b0과485cf0/485e42의 실제 프로세스 장치다.
using ProcessLogHook=std::function<void(std::int32_t,std::string_view)>; // 완성된 원문 메시지를 가변인자 서식으로 다시 해석하지 않는 로그 경계다.
using ProcessAssertionHook=std::function<void(std::string_view,std::string_view,std::uint32_t)>; // 실제 원문 표현식/파일·줄 진단이며 정상 대화상자 복귀 뒤 caller 실행을 계속한다.
class ProcessTrace final { // 같은 현재 옵션/연결 상태와 이 장치는 모든 등록 callback보다 오래 살아야 한다.
public:
    ProcessTrace(ConnectionState& connections,Options& options,platform::UserDataRoot user); // BSS0의 버퍼/대상/깊이/진단 상태만 구성하며 파일·창/OS 시각을 읽거나 만들지 않는다.
    ProcessTrace(const ProcessTrace&)=delete;ProcessTrace& operator=(const ProcessTrace&)=delete; // 현재 출력 버퍼/FILE·재진입 상태와 callback this를 복제하지 않는다.
    void Log(std::int32_t level,std::string_view message); //0<=level<=현재traceLevel일 때만 ~코드/들여쓰기·개행정리→모드/OS시각 접두·선택stderr복제/현재출력·선택flush다.
    void Flush(); //4800b0의 현재 출력 대상이 이미 있을 때만 fflush한다. 새 stdout/파일을 열지 않는다.
    void Assertion(std::string_view expression,std::string_view file,std::uint32_t line); // stderr→사용자 Diagnostics/assert.html→현재로그flush→실제호스트스택/11.01경고대화상자→현재진단 상태 해제다. throwOnAssert옵션은 원문도 이 경로에서 소비하지 않는다.
    ProcessLogHook LogDevice();ProcessAssertionHook AssertionDevice(); // 최종 서버 owner가 같은 장기 장치를 모든 현재 hook에 연결하도록 this만 대여한다.
    std::int32_t& Indentation() { return indentation_; } //004d891c의 signed 현재 공백 수이며 음수면 원문 반복처럼 공백0개다.
    std::optional<std::string>& ExtraInformation() { return extra_; } //004d8ac8의 NULL/소유 진단 추가문자열을 구별한다. 진단/재진입 복귀는 원문 위치에서 반납한다.
private:
    ConnectionState& connections_;Options& options_;platform::UserDataRoot user_; // 원문 현재 모드/옵션과 모든 자동 출력의 사용자 홈 정책이다.
    std::array<char,1024> buffer_{};std::FILE* output_{};std::int32_t indentation_{}; //004d8518의1024BYTE/004d8918 NULL/004d891c BSS0이며 출력은 첫 허용 로그에서만 stdout이 된다.
    std::uint32_t asserting_{};std::optional<std::string> extra_; //004d8acc BSS0/004d8ac8 NULL이다. 이미 진단 중이면 추가정보만 지우고 복귀한다.
    static std::string Timestamp(); //실제 _time32/localtime/asctime의 앞19BYTE만 만들며 게임 Clock을 사용하지 않는다.
    static std::string Stack(); //현재 Windows의 실제 최대20개 반환주소를 포인터 폭/4개마다CRLF로 작성한다. 구형 EBP 사슬이나 원본 주소를 합성하지 않는다.
    void Report(std::string_view diagnostic); //원문 HTML본문을 같은 Unicode 사용자 Diagnostics/assert.html의 실제 w+b/무버퍼 FILE에 쓰고 닫는다.
};
}
