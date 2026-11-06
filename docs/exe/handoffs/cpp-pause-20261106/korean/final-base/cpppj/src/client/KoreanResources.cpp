// 6차 한국어 지원: 한국어 번역표와 D2Coding 글꼴 파일 탐색 구현이다.
#include "client/KoreanResources.h"
#include <algorithm>
#include <fstream>
#include <iterator>

namespace netstorm::client { // 클론 데이터 파일 탐색이다.
// 시작 폴더 목록을 만든다. 각 시작점에서 위로 올라가며 이미 포함한 폴더는 건너뛰어 가까운 순서를 유지한다.
std::vector<std::filesystem::path> CloneDataRoots(const std::filesystem::path& gameDirectory, const std::filesystem::path& executableDirectory, int maxLevels) {
    std::vector<std::filesystem::path> roots;
    for (const auto& start : {executableDirectory, gameDirectory}) { // 실행 파일 위치를 게임 폴더보다 먼저 본다(배포판은 exe 옆에 데이터를 둔다).
        auto current = start;
        for (int level = 0; level <= maxLevels && !current.empty(); ++level) { // 위로 한 단계씩 올라간다.
            if (std::find(roots.begin(), roots.end(), current) == roots.end()) roots.push_back(current);
            const auto parent = current.parent_path();
            if (parent == current) break; // 루트에 도달하면 멈춘다.
            current = parent;
        }
    }
    return roots;
}
// D2Coding 글꼴은 업데이트 때 같은 폴더에 새 버전이 추가되므로 이름순 마지막을 최신으로 본다.
std::filesystem::path FindKoreanFontFile(const std::vector<std::filesystem::path>& roots) {
    for (const auto& root : roots) { // 가까운 폴더의 fonts를 먼저 본다.
        std::error_code error;
        const auto directory = root / "fonts";
        if (!std::filesystem::is_directory(directory, error)) continue;
        std::filesystem::path newest;
        for (const auto& entry : std::filesystem::directory_iterator(directory, error)) { // 폴더 안의 D2Coding*.ttc 중 이름순 마지막을 고른다.
            const auto name = entry.path().filename().string();
            if (name.starts_with("D2Coding") && entry.path().extension() == ".ttc" && entry.path() > newest) newest = entry.path();
        }
        if (!newest.empty()) return newest;
    }
    return {};
}
// 번역표는 UTF-8 텍스트이며 BOM이 있으면 제거해 첫 줄의 구분 문자가 줄 첫 글자가 되게 한다.
std::optional<std::string> ReadKoreanTranslationFile(const std::vector<std::filesystem::path>& roots) {
    const std::filesystem::path relatives[] = {std::filesystem::path("lang") / "xlat.korean", std::filesystem::path("assets") / "lang" / "korean" / "xlat.korean"};
    for (const auto& root : roots) for (const auto& relative : relatives) { // 가까운 폴더, 같은 폴더 안에서는 배포 위치 우선이다.
        std::error_code error;
        const auto path = root / relative;
        if (!std::filesystem::is_regular_file(path, error)) continue;
        std::ifstream stream(path, std::ios::binary);
        if (!stream) continue;
        std::string text((std::istreambuf_iterator<char>(stream)), std::istreambuf_iterator<char>());
        if (text.starts_with("\xEF\xBB\xBF")) text.erase(0, 3); // UTF-8 BOM 제거다.
        return text;
    }
    return std::nullopt;
}
}
