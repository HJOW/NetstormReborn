"""게임 실행 없이 C++의 확정된 타입 표와 영역 패턴을 dotnetpj 회귀 자료로 내보낸다."""

from pathlib import Path
import hashlib
import json
import re
import subprocess
import sys
import tempfile

# 저장소 루트. 도구와 결과는 dotnetpj 안에만 보관하며 cpppj와 원본을 수정하지 않는다.
ROOT = Path(__file__).resolve().parents[2]
# 창·게임·OS 초기화가 없는 순수 자료 판독 소스만 임시 콘솔로 빌드한다.
SOURCES = ("RiftTypeTable.cpp", "TypeLoadOrder.cpp", "TypeParser.cpp", "RiftType.cpp", "OriginalText.cpp")
# 생성자 주소는 원본 주소 표에서 읽는 메타데이터다. 게임의 생성자나 Win32 API를 호출하지 않는다.
DRIVER = r'''
// 순수 타입 자료 판독을 위한 임시 콘솔. 게임과 창을 실행하지 않는다.
#include "o/RiftType.h"
#include <fstream>
#include <iostream>
#include <iomanip>
#include <iterator>
namespace netstorm::o {
namespace {
#include "o/TypeConstructors.inc"
}
// 기존 생성자 주소 표를 그대로 조회하며 해당 주소를 실행하지 않는다.
std::uint32_t TypeConstructorAddress(OriginalEdition edition, std::size_t number) {
    return edition == OriginalEdition::Patch1078 ? kPatchConstructors.at(number) : kCdConstructors.at(number);
}
}
// C++ 복원 로더로 타입 표를 구성하고 자산 116개를 출력한다.
int main(int argc, char** argv) {
    using namespace netstorm::o;
    const auto order = TypeLoadOrder(OriginalEdition::Patch1078);
    std::vector<RiftTypeDefinition> definitions;
    definitions.reserve(order.size());
    // Linux의 경로 대소문자를 피하려고 추출 파일 이름은 소문자로 저장한다.
    for (auto name : order) {
        std::string lower(name);
        // 자산 이름은 ASCII이다.
        for (auto& c : lower) if (c >= 'A' && c <= 'Z') c += 'a' - 'A';
        std::ifstream file(std::string(argv[1]) + "/" + lower + ".type", std::ios::binary);
        if (!file) throw std::runtime_error("타입 파일이 없습니다");
        std::string raw((std::istreambuf_iterator<char>(file)), {});
        definitions.push_back(RiftTypeDefinition::Parse(raw));
    }
    std::vector<RiftTypeSource> sources;
    // 정의 배열을 완성한 뒤 포인터를 등록하여 재할당으로 무효화되지 않게 한다.
    for (std::size_t i = 0; i < order.size(); ++i) sources.push_back({order[i], &definitions[i]});
    RiftTypeTable table(OriginalEdition::Patch1078, sources);
    std::cout << std::setprecision(9);
    // 내장·빈 타입은 C# 월드의 번호 체계가 복원된 뒤 별도로 연결한다.
    for (const auto& t : table.Types()) if (t.fromAsset)
        std::cout << t.assetName << '\t' << t.flags1 << '\t' << t.flags2 << '\t' << t.zOrder << '\t'
                  << t.cost << '\t' << t.level << '\t' << t.containerListFlags << '\t'
                  << t.contentListFlags << '\t' << RiftTypeTable::NameHash(t.name) << '\n';
}
'''


def export_types():
    """읽기 전용 C++ 소스와 클론 아카이브를 사용해 정적 기대값을 재생성한다. g++가 필요하다."""
    sys.path.insert(0, str(ROOT / "tools"))
    from taff import TaffArchive

    with tempfile.TemporaryDirectory(prefix="netstorm-dotnet-reference-") as folder:
        temporary = Path(folder)
        types = temporary / "types"
        types.mkdir()
        archive = TaffArchive(str(ROOT / "assets/game-data/netstorm.tarc"))
        # 보호 원본 대신 배포용 클론 데이터에서 .type만 임시 폴더에 추출한다.
        for entry in archive.entries:
            if entry.name.lower().endswith(".type"):
                name = entry.name.replace("\\", "/").split("/")[-1].lower()
                (types / name).write_bytes(archive.read(entry))
        driver = temporary / "dump.cpp"
        driver.write_text(DRIVER, encoding="utf-8")
        binary = temporary / "dump"
        command = ["g++", "-std=c++20", "-O1", "-I", str(ROOT / "cpppj/src"), str(driver)]
        command += [str(ROOT / "cpppj/src/o" / source) for source in SOURCES]
        subprocess.run(command + ["-o", str(binary)], check=True)
        result = subprocess.run([str(binary), str(types)], check=True, capture_output=True, text=True).stdout
    if len(result.splitlines()) != 116:
        raise ValueError("10.78의 자산 타입 116개가 아닙니다.")
    header = "# x86 직접 출력이 아닌, 읽기 전용 C++ RiftTypeTable + 클론 10.78 자산의 정적 대조값.\n"
    header += "# 자산명 flags1 flags2 zorder cost level containerListFlags contentListFlags nameHash (탭 구분)\n"
    # 판독 자료와 핵심 복원 코드의 SHA를 남겨 x86 기대값과 출처를 구분한다.
    for name in ("assets/game-data/netstorm.tarc", "cpppj/src/o/RiftTypeTable.cpp", "cpppj/src/o/TypeParser.cpp"):
        header += f"# SHA256 {name} {hashlib.sha256((ROOT / name).read_bytes()).hexdigest()}\n"
    output = ROOT / "dotnetpj/tests/Netstorm.Assets.Tests/Fixtures/type-metadata-1078.tsv"
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(header + result, encoding="utf-8", newline="\n")


def export_patterns():
    """원본에서 정적으로 추출한 C++ 포함 표의 68개 영역을 C# 내장 자료로 옮긴다."""
    source = (ROOT / "cpppj/src/o/CanonDecoderTables.inc").read_text(encoding="utf-8")
    section = source.split("kTerritoryPatterns{{", 1)[1].split("}};", 1)[0]
    rows = []
    # 한 패턴에 들어 있는 원본 4바이트 셀을 그대로 읽는다.
    for line in section.splitlines():
        if "// " not in line:
            continue
        _, width, height = map(int, re.search(r"\{(\d+), (\d+), (\d+),", line).groups())
        cells = [tuple(int(value, 16) for value in values) for values in re.findall(
            r"\{0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2}),0x([0-9a-f]{2})\}", line)][:width * height]
        rows.append({"Width": width, "Height": height, "Cells": "".join(chr(cell[1]) for cell in cells),
                     "Variations": [cell[0] - 48 if cell[1] != 46 else 0 for cell in cells],
                     "Labels": [cell[3] - 97 for cell in cells]})
    if len(rows) != 68:
        raise ValueError("영역 표는 68개여야 합니다.")
    blocks = ["  {\n" + ",\n".join("    " + json.dumps(key) + ": " + json.dumps(value)
                                   for key, value in row.items()) + "\n  }" for row in rows]
    (ROOT / "dotnetpj/src/Netstorm.Assets/TerritoryPatterns.json").write_text(
        "[\n" + ",\n".join(blocks) + "\n]\n", encoding="utf-8", newline="\n")


def main():
    """임시 콘솔만 빌드·실행하고 dotnetpj의 두 회귀 자료를 갱신한다."""
    export_types()
    export_patterns()
    print("정적 타입 116개·영역 패턴 68개 갱신. 원본 게임 실행 없음.")


if __name__ == "__main__":
    main()
