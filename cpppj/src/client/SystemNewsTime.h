// 서버 이름별 뉴스 시각의 원문 키/읽기/쓰기 순서를 실제 Client 저장소에서 실행한다.
#pragma once
#include <cstdint>
#include <functional>
#include <span>
#include <string_view>

namespace netstorm::client { //0042e470/0042e4d0/0042e4f0의 좁은 문자 경계다.
struct SystemNewsTimeGlobals { //원문 전역의 장기 실제 물리를 빌리며 별도 캐시를 만들지 않는다.
    std::uint32_t& value; //00548748의 현재 뉴스 시각 DWORD다.
    std::span<char> key; //00548790부터 다음 완성 목록 전까지의 실제128BYTE다.
};
struct SystemNewsTimeHooks { //원문 현재 서버 조회와 같은 Config 장치다.
    std::function<std::string_view()> serverName; //0047c430의 현재 공유 서버 이름을 매번 조회한다.
    std::function<std::int32_t(std::string_view)> read; //00441090의 누락0/현재 signed 정수 읽기다.
    std::function<void(std::string_view)> configure; //00441580의 같은 인용 설정문 해석/변경 알림이다. 디스크 저장을 강제하지 않는다.
};
std::string_view BuildSystemNewsTimeKey(SystemNewsTimeGlobals globals,const SystemNewsTimeHooks& hooks); //현재 이름+NewsTime을 같은 공유 버퍼에 NUL까지 복사한 뒤 '.'만 '_'로 바꾼다.
void ReadSystemNewsTime(SystemNewsTimeGlobals globals,const SystemNewsTimeHooks& hooks); //키 작성/현재 Config 조회 뒤 실제 DWORD를 덮어쓴다.
void WriteSystemNewsTime(SystemNewsTimeGlobals globals,const SystemNewsTimeHooks& hooks); //키 조회 전에 보관한 시각을 signed%d로 서식화해 같은 Config 해석기에 넘긴다.
}
