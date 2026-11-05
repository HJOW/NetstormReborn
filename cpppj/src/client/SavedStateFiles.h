// State1/3이 같은 저장상태 이름과 실제 파일 정리를 사용한다.
#pragma once
#include <cstdint>
#include <filesystem>
namespace netstorm::o { class RawFileName; } // 현재 Client/VFS가 함께 소유한 원문 경로 고리다.

namespace netstorm::client { // 원문 고정 root/d/!state.dat와 현재 설치 폴더를 연결한다.
// 004b5470의 빈 root·d·!state.dat를 현재 게임 폴더 아래 절대 이름으로 돌려준다. 파일을 만들지 않는다.
// 5차: userFile이 비어 있지 않으면 같은 경로 고리 작성 뒤 결과 위치만 사용자 홈의 Saves\!state.dat로 바꾼다(설치 폴더에 쓰지 않기 위한 호환 처리).
std::filesystem::path SavedStateFilePath(const std::filesystem::path& gameDirectory,o::RawFileName& formatter,const std::filesystem::path& userFile={});
// 원문 unlink/오류 매핑을 수행하고0/-1을 돌려준다. Native State3는 이 반환을 원문처럼 무시한다.
// userFile 규칙은 SavedStateFilePath와 같다. 사용자 파일이 없으면 원문처럼 ENOENT(-1)이며 설치 폴더의 옛 파일은 지우지 않는다.
std::int32_t DeleteSavedStateFile(const std::filesystem::path& gameDirectory,o::RawFileName& formatter,const std::filesystem::path& userFile={});
}
