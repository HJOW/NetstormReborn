// 6차 한국어 지원: 클론이 추가한 한국어 번역표(xlat.korean)와 D2Coding 글꼴 파일을 찾는 보조 함수 모음이다.
// 원본 10.78에는 없는 파일이므로 설치(휴대 배포) 폴더와 저장소 개발 폴더를 모두 탐색한다.
#pragma once
#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace netstorm::client { // 클론 데이터 파일 탐색은 Client와 독립 검사가 같은 규칙을 공유한다.
// 실행 파일 폴더와 게임 폴더에서 위로 최대 maxLevels단계까지 올라가며 만든 탐색 시작 폴더 목록이다(중복 제거, 가까운 순서).
// 휴대 배포(exe 옆 fonts, lang)와 개발 환경(저장소 루트의 fonts, assets/lang)을 같은 함수로 찾기 위해 추가했다.
std::vector<std::filesystem::path> CloneDataRoots(const std::filesystem::path& gameDirectory, const std::filesystem::path& executableDirectory, int maxLevels = 6);
// 탐색 폴더 아래 fonts 폴더에서 D2Coding*.ttc 중 이름순 가장 마지막(가장 최신 버전) 파일을 돌려준다. 없으면 빈 경로다.
std::filesystem::path FindKoreanFontFile(const std::vector<std::filesystem::path>& roots);
// 탐색 폴더 아래 lang/xlat.korean, assets/lang/korean/xlat.korean 순서로 첫 파일을 UTF-8 텍스트(BOM 제거)로 읽는다. 없으면 빈 값이다.
std::optional<std::string> ReadKoreanTranslationFile(const std::vector<std::filesystem::path>& roots);
}
