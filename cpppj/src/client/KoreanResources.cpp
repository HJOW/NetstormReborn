// 6차 한국어 지원: 한국어 번역표와 D2Coding 글꼴 파일 탐색 구현이다.
#include "client/KoreanResources.h"
#include "o/BaseFile.h"
#include "o/OriginalText.h"
#include <array>
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
    const std::filesystem::path relatives[] = {std::filesystem::path("lang") / "korean" / "xlat.korean", std::filesystem::path("lang") / "xlat.korean", std::filesystem::path("assets") / "lang" / "korean" / "xlat.korean"};
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

namespace netstorm::client { // 언어번들은원문자료/사용자맵과다른읽기전용namespace다.
namespace { // 고정공식본문목록과논리이름검사만공유한다.
constexpr std::array<std::string_view,43> kBuiltinKoreanNames{{
    "tell.korean",
    "help.korean",
    "config.korean",
    "offical1.korean",
    "offical2.korean",
    "offical2sep.korean",
    "offical3.korean",
    "offical4.korean",
    "offical5.korean",
    "offical5sep.korean",
    "offical6.korean",
    "tutorial1.korean",
    "tutorial2.korean",
    "tutorial3.korean",
    "tutorial4.korean",
    "tutorial5.korean",
    "tutorial6.korean",
    "bc1menu76.korean",
    "bc2bridging76.korean",
    "bc3acid_bar76.korean",
    "bc4bow_rush76.korean",
    "bc5walkers76.korean",
    "bc6mows76.korean",
    "bc7sun_bars76.korean",
    "bc8fury76.korean",
    "bc9ssun_bars76.korean",
    "thewarbegins.korean",
    "masterofwhirligigs.korean",
    "savetheisland.korean",
    "fragilefortune.korean",
    "thunderingpower.korean",
    "dissolvedalliance.korean",
    "thenoose.korean",
    "rainvsrain.korean",
    "runforit.korean",
    "guardmyback.korean",
    "surrounded.korean",
    "breakingthrough.korean",
    "totherescue.korean",
    "vicious.korean",
    "enemyterritory.korean",
    "finalconfrontation.korean",
    "demostormrages.korean",
}}; // 원문대본의공통11/공식32고유파일만허용한다. xlat은기존별도표로읽는다.
// 드라이브/UNC/상위이동/다른디렉터리를거부하고DataDir의선행구분자하나만논리접두로해석한다.
std::optional<std::string> KoreanLeaf(std::string_view input,bool wildcard) {
    auto text=std::string(input.substr(0,input.find('\0')));std::replace(text.begin(),text.end(),'\\','/');text=o::AsciiLower(text);
    if (text.empty() || text.starts_with("//") || text.find(':')!=text.npos) return {};
    if (text.front()=='/') text.erase(0,1);if (!text.starts_with("d/")) return {};text.erase(0,2);
    if (text.empty() || text.find('/')!=text.npos || text=="." || text==".." || !text.ends_with(".korean")) return {};
    if (!wildcard && text.find_first_of("*?")!=text.npos) return {};
    if (!wildcard && std::find(kBuiltinKoreanNames.begin(),kBuiltinKoreanNames.end(),text)==kBuiltinKoreanNames.end()) return {};
    return text;
}
// 아카이브FileFinder와같은ASCII와일드카드만비교하며경로는이미고정된한디렉터리다.
bool KoreanMatches(std::string_view name,std::string_view mask) {
    std::size_t n{},m{},star=std::string_view::npos,retry{}; // 마지막별표에서늘릴소비위치만기억한다.
    while (n<name.size()) { if (m<mask.size() && (mask[m]=='?' || mask[m]==name[n])) { ++m;++n; }else if (m<mask.size() && mask[m]=='*') { star=m++;retry=n; }else if (star!=std::string_view::npos) { m=star+1;n=++retry; }else return false; } // 원문목록순서를정렬하지않는다.
    while (m<mask.size() && mask[m]=='*') ++m;return m==mask.size();
}
// 열거된논리이름의확장자만제외한마지막요소로중복을판정한다.
std::string KoreanStem(std::string_view path) {
    const auto separator=path.find_last_of("\\/");auto name=o::AsciiLower(path.substr(separator==path.npos ? 0:separator+1));const auto dot=name.find_last_of('.');if (dot!=name.npos) name.resize(dot);return name;
}
}
std::optional<std::filesystem::path> FindBuiltinKoreanFile(const o::BaseFileSystem& files,const std::vector<std::filesystem::path>& roots,std::string_view name) {
    const auto leaf=KoreanLeaf(name,false);if (!leaf || files.ProbeExact(name)) return {}; // 사용자/설치/등록TAFF의정확한한국어파일이항상우선한다.
    for (const auto& root:roots) for (const auto& prefix:{std::filesystem::path("lang")/"korean"/"d",std::filesystem::path("assets")/"lang"/"korean"/"d"}) { // 가까운배포번들다음개발저장소를읽기만한다.
        const auto candidate=root/prefix/(*leaf);std::error_code error;if (std::filesystem::is_regular_file(candidate,error)) return std::filesystem::absolute(candidate).lexically_normal();
    }
    return {};
}
std::optional<std::vector<std::uint8_t>> ReadLocalizedGameFile(const o::BaseFileSystem& files,const std::vector<std::filesystem::path>& roots,std::string_view name) {
    if (const auto path=FindBuiltinKoreanFile(files,roots,name)) return o::ReadFileBytes(*path);return files.TryRead(name); // BOM/AI/SP설정은바꾸지않으며부재영어대체도기존VFS가소유한다.
}
std::vector<std::string> MatchLocalizedGameFiles(const o::BaseFileSystem& files,const std::vector<std::filesystem::path>& roots,std::string_view pattern) {
    auto result=files.MatchFinder(pattern);const auto mask=KoreanLeaf(pattern,true);if (!mask) return result; // 영어/절대사용자패턴은기존벡터를그대로반환한다.
    const auto separator=pattern.find_last_of("\\/");const auto prefix=std::string(pattern.substr(0,separator==pattern.npos ? 0:separator+1));
    for (const auto name:kBuiltinKoreanNames) { // 같은영어fallback행의위치만한국어논리이름으로바꾸고새번들만뒤에추가한다.
        if (!KoreanMatches(name,*mask)) continue;const auto logical=prefix+std::string(name);if (!FindBuiltinKoreanFile(files,roots,logical)) continue;
        const auto stem=KoreanStem(name);const auto found=std::find_if(result.begin(),result.end(),[&](const auto& old) { return KoreanStem(old)==stem; });
        if (found==result.end()) result.push_back(logical);else if (o::AsciiLower(*found).ends_with(".english")) *found=logical;
    }
    return result;
}
std::string DecodeQuickHelpDisplaySource(std::string_view bytes) {
    if (!bytes.starts_with("\xEF\xBB\xBF")) return std::string(bytes); // 원문영어의NUL/1252/제어BYTE는새로해석하지않는다.
    const auto decoded=o::DecodeOriginalText({reinterpret_cast<const std::uint8_t*>(bytes.data()),bytes.size()});const auto display=o::EncodeDisplayText(decoded);return {display.begin(),display.end()};
}
}
