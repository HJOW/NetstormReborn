// 현재 Client 설정과 Windows의 URL/비대기 실행을 원문 도움말 정책에 연결한다.
#pragma once
#include <string_view>
namespace netstorm::client { // 창·메시지 루프와 별개인 동기 OS 요청 경계다.
class Client; // 현재 장기 설정을 빌리고 별도 설정 사본을 만들지 않는다.
bool OpenWindowsHelpUrl(std::string_view utf8); //004dd640의 desktop/open/SHOW5와 성공>32를 W API로 실행한다.
bool StartWindowsHelpCommand(std::string_view utf8); //004dd6b0의 program0/wait0 경로를 W API로 실행하고 자식 핸들을 반납한다.
void OpenClientExternalHelp(Client& client,std::string_view originalBytes); //원본 링크 바이트와 UTF-8 설정 경로를 구별해 같은 현재 정책을 실행한다.
}
