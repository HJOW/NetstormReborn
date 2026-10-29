// State1/3이 같은 저장상태 이름과 실제 파일 정리를 사용한다.
#pragma once
#include <cstdint>
#include <filesystem>

namespace netstorm::client { // 원문 고정 root/d/!state.dat와 현재 설치 폴더를 연결한다.
// 004b5470의 빈 root·d·!state.dat를 현재 게임 폴더 아래 절대 이름으로 돌려준다. 파일을 만들지 않는다.
std::filesystem::path SavedStateFilePath(const std::filesystem::path& gameDirectory);
// 원문 unlink/오류 매핑을 수행하고0/-1을 돌려준다. Native State3는 이 반환을 원문처럼 무시한다.
std::int32_t DeleteSavedStateFile(const std::filesystem::path& gameDirectory);
}
