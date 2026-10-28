// 객체 덤프의 두 파일 단계와 실제 CRT 로그 출력을 디스크 장치에 연결한다.
#pragma once
#include "client/RawObjectDump.h"
#include <filesystem>

namespace netstorm::client { // 경로와 파일 장치는 한 동기 덤프 호출에서 함께 유지한다.
// 명시 디렉터리에 squids.txt를 쓰며 상위 폴더를 새로 만들지 않는다. 매 호출마다 독립 장치를 구성한다.
ObjectDumpHooks MakeObjectDumpDiskHooks(const std::filesystem::path& directory,std::function<void(std::uint32_t)> trace,std::function<void()> openFailure);
// 실제 stdout/stderr/FILE 출력이며 console은 현재 trace→Chat 조건을 구현한 호출자가 제공한다.
TraceLogHooks MakeObjectDumpDiskLogHooks(std::function<void(std::string_view)> console);
}
