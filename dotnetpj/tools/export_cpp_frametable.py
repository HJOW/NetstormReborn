"""C++ 파서로 실제 isle.type 프레임 테이블을 덤프해 DN 테이블과 대조한다.

게임을 실행하지 않으며 순수 파서·표 모듈만 임시 콘솔로 빌드한다.
Windows에서는 MSVC cl.exe를 자동으로 찾아 쓰고, 없으면 PATH의 g++로 시도한다.
DN 쪽 기대값은 `GroundTypeTableTests.IsleTableMatchesFileBytes`와 같은 triple이다.
"""

import glob
import os
import shutil
import subprocess
import sys
import tempfile
from pathlib import Path

# 저장소 루트. 도구와 결과는 dotnetpj 안에만 보관하며 cpppj와 원본을 수정하지 않는다.
ROOT = Path(__file__).resolve().parents[2]
# 창·게임·OS 초기화가 없는 순수 판독 소스만 임시 콘솔로 빌드한다.
SOURCES = ("TypeParser.cpp", "RiftType.cpp", "OriginalText.cpp")
# 순수 타입 정의 판독·프레임 코드 나열용 임시 콘솔. 게임과 창을 실행하지 않는다.
DRIVER = r'''
#include "o/RiftType.h"
#include <fstream>
#include <iostream>
#include <iterator>
#include <string>

int main(int argc, char** argv) {
    if (argc < 2) return 2;
    std::ifstream f(argv[1], std::ios::binary);
    if (!f) return 3;
    std::string raw((std::istreambuf_iterator<char>(f)), {});
    if (raw.size() >= 3 && (unsigned char)raw[0] == 0xEF
        && (unsigned char)raw[1] == 0xBB && (unsigned char)raw[2] == 0xBF)
        raw = raw.substr(3);
    auto def = netstorm::o::RiftTypeDefinition::Parse(raw);
    auto table = def.FrameTable();
    auto codes = table.Codes();
    std::cout << "clusters " << codes.size() << "\n";
    auto run = table.Run('J');
    std::cout << "jfirst " << run.first << " jcount " << run.count << "\n";
    for (std::size_t i = 0; i < codes.size(); ++i) {
        std::cout << i << ':' << (char)codes[i].side << ',' << (char)codes[i].variant << ','
                  << (int)codes[i].number << ',' << (int)codes[i].flags << '\n';
    }
    return 0;
}
'''

# DN `GroundTypeTableTests`와 공유하는 기대값이다. tarc 독립 파싱과 L2 덤프 잔차 분석에서 확정했다.
EXPECTED_CLUSTERS = 317
EXPECTED_JFIRST = 172
EXPECTED_JCOUNT = 1


def find_msvc_root():
    """설치된 최신 MSVC 루트를 찾는다. 없으면 None이다."""
    program_files = os.environ.get("ProgramFiles", r"C:\Program Files")
    pattern = os.path.join(program_files, "Microsoft Visual Studio", "*", "*",
                           "VC", "Tools", "MSVC", "*")
    candidates = sorted(glob.glob(pattern))
    for root in reversed(candidates):
        if os.path.isfile(os.path.join(root, "bin", "Hostx64", "x64", "cl.exe")):
            return root
    return None


def find_sdk_version():
    """설치된 최신 Windows SDK 버전을 찾는다. 없으면 None이다."""
    base = r"C:\Program Files (x86)\Windows Kits\10\Include"
    if not os.path.isdir(base):
        return None
    for version in sorted(os.listdir(base), reverse=True):
        if os.path.isfile(os.path.join(base, version, "ucrt", "stdio.h")):
            return version
    return None


def dump_with_cpp_parser(type_path):
    """임시 콘솔을 빌드·실행하고 isle 프레임 테이블 전체 텍스트를 돌려준다."""
    with tempfile.TemporaryDirectory(prefix="netstorm-dotnet-frametable-") as folder:
        temporary = Path(folder)
        driver = temporary / "dump.cpp"
        driver.write_text(DRIVER, encoding="utf-8")
        binary = temporary / ("dump.exe" if os.name == "nt" else "dump")
        sources = [str(ROOT / "cpppj/src/o" / name) for name in SOURCES]
        env = None
        if os.name == "nt":
            root = find_msvc_root()
            sdk = find_sdk_version()
            if root is None or sdk is None:
                raise RuntimeError("MSVC 또는 Windows SDK를 찾을 수 없습니다.")
            msbin = os.path.join(root, "bin", "Hostx64", "x64")
            sdk_inc = os.path.join(
                r"C:\Program Files (x86)\Windows Kits\10\Include", sdk)
            sdk_lib = os.path.join(
                r"C:\Program Files (x86)\Windows Kits\10\Lib", sdk)
            env = dict(os.environ)
            env["INCLUDE"] = ";".join([
                os.path.join(root, "include"),
                os.path.join(sdk_inc, "ucrt"), os.path.join(sdk_inc, "shared"),
                os.path.join(sdk_inc, "um"), str(ROOT / "cpppj/src")])
            env["LIB"] = ";".join([
                os.path.join(root, "lib", "x64"),
                os.path.join(sdk_lib, "ucrt", "x64"),
                os.path.join(sdk_lib, "um", "x64")])
            env["PATH"] = msbin + os.pathsep + env.get("PATH", "")
            command = [os.path.join(msbin, "cl.exe"), "/std:c++20", "/EHsc",
                       "/utf-8", "/nologo", str(driver)] + sources + [
                           "/Fe:" + str(binary), "/Fo:" + temporary.as_posix() + "/"]
        elif shutil.which("g++") is not None:
            command = ["g++", "-std=c++20", "-O1", "-I", str(ROOT / "cpppj/src"),
                       str(driver)] + sources + ["-o", str(binary)]
        else:
            raise RuntimeError("C++ 컴파일러(cl.exe 또는 g++)를 찾을 수 없습니다.")
        subprocess.run(command, check=True, capture_output=True, env=env)
        result = subprocess.run([str(binary), str(type_path)], check=True,
                                capture_output=True, text=True)
        return result.stdout


def main():
    """tarc에서 isle.type을 꺼내 C++ 파서로 덤프하고 DN 기대값과 대조한다."""
    sys.path.insert(0, str(ROOT / "tools"))
    from taff import TaffArchive

    archive = TaffArchive(str(ROOT / "assets/game-data/netstorm.tarc"))
    raw = None
    for entry in archive.entries:
        if entry.name.lower().replace("\\", "/").endswith("/isle.type"):
            raw = archive.read(entry)
            break
    if raw is None:
        raise RuntimeError("tarc에서 isle.type을 찾을 수 없습니다.")
    with tempfile.TemporaryDirectory(prefix="netstorm-dotnet-frametable-") as folder:
        type_path = Path(folder) / "isle.type"
        type_path.write_bytes(raw)
        output = dump_with_cpp_parser(type_path)
    lines = output.splitlines()
    clusters = int(lines[0].split()[1])
    numbers = lines[1].split()
    jfirst = int(numbers[1])
    jcount = int(numbers[3])
    print(f"clusters {clusters} (기대 {EXPECTED_CLUSTERS})")
    print(f"jfirst {jfirst} (기대 {EXPECTED_JFIRST}) jcount {jcount} (기대 {EXPECTED_JCOUNT})")
    if clusters != EXPECTED_CLUSTERS or jfirst != EXPECTED_JFIRST or jcount != EXPECTED_JCOUNT:
        raise SystemExit("C++ 파서 테이블이 DN 기대값과 다릅니다.")
    print("C++ 파서 테이블이 DN 테이블과 일치합니다.")


if __name__ == "__main__":
    main()
