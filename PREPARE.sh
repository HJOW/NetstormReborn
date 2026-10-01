#!/usr/bin/env bash
# ============================================================
# NetStorm 클론 프로젝트 개발 환경 점검 및 설치 스크립트 (GUI 환경의 Linux)
#
# 실행하면 설치 항목 목록을 보여 주고, 사용자가 고른 항목만 점검한다.
# 점검 결과 설치되어 있지 않은 항목은 자동으로 설치한 뒤 다시 점검한다.
# Windows 용 PREPARE.ps1 과 같은 항목·흐름을 따르며, Windows 전용 도구는 Linux 대체 도구로 바꿨다.
#   - x64dbg, Process Monitor → Wine (winedbg 동적 분석, WINEDEBUG=+file 로 파일 접근 관찰)
#   - VS Build Tools 2022(C++) → gcc/g++/make
#
# 사용법:
#   bash ./PREPARE.sh                  # 항목 선택 후 점검·설치
#   bash ./PREPARE.sh --check-only     # 설치하지 않고 점검만
#   bash ./PREPARE.sh --all            # 선택 화면 없이 모든 항목
#   bash ./PREPARE.sh --tools-dir DIR  # Ghidra·vcpkg 설치 폴더 (기본값: ~/Tools)
#
# 지원 패키지 관리자: apt(Debian/Ubuntu 계열), dnf(Fedora/RHEL 계열), pacman(Arch 계열), zypper(openSUSE)
# 시스템 패키지 설치 시 sudo 를 호출한다. 스크립트 자체는 일반 사용자로 실행해야 한다.
# ============================================================

# bash 전용 문법(배열 등)을 쓰므로 sh/dash 로 실행되면 바로 중단한다
if [ -z "${BASH_VERSION:-}" ]; then
    echo 'bash 로 실행하세요: bash ./PREPARE.sh' >&2
    exit 1
fi


# ============================================================
# 상수
# ============================================================

# 게임 빌드에 필요한 .NET SDK 주 버전 (프로젝트 대상 프레임워크 net10.0)
REQUIRED_DOTNET_MAJOR=10

# .NET SDK 를 설치할 사용자 폴더 (Microsoft dotnet-install.sh 사용, sudo 불필요)
DOTNET_INSTALL_DIR="$HOME/.dotnet"

# MonoGame 프로젝트 템플릿 패키지 (버전은 src/ 의 MonoGame.Framework.DesktopGL 참조 버전과 맞춘다)
MONOGAME_TEMPLATE_PACKAGE='MonoGame.Templates.CSharp::3.8.5.1'

# Ghidra 실행에 필요한 최소 JDK 주 버전
MIN_JDK_MAJOR=21

# Python 도구에 필요한 최소 버전
MIN_PYTHON_VERSION='3.10'

# 분석 도구에서 사용하는 Python 패키지 (pip 패키지명, 같은 순서의 import 모듈명)
PYTHON_PACKAGES=(pillow pefile capstone)   # PNG 내보내기, exe·DLL 리소스 추출, x86 역어셈블
PYTHON_MODULES=(PIL pefile capstone)

# 설치할 VS Code 확장 ID 목록
VSCODE_EXTENSIONS=(
    ms-dotnettools.csdevkit   # C# Dev Kit (솔루션·테스트 탐색기)
    ms-dotnettools.csharp     # C# 언어 지원
    ms-python.python          # Python (분석 도구)
)

# 사용자 실행 파일 폴더 (yt-dlp, pip --user 설치 위치)
LOCAL_BIN_DIR="$HOME/.local/bin"

# 패키지로 FFmpeg 를 설치할 수 없을 때(sudo 없음 등) 받는 정적 빌드 (BtbN, ffmpeg·ffprobe 포함).
# 주의: johnvansickle.com 정적 빌드는 호스트 이름 해석(DNS) 때 세그멘테이션 오류로 죽어 원격 YouTube 스트림을 못 읽는다(2026-10-01 확인).
FFMPEG_STATIC_URL='https://github.com/BtbN/FFmpeg-Builds/releases/download/latest/ffmpeg-master-latest-linux64-gpl.tar.xz'

# yt-dlp 가 YouTube 서명·n 값 해석에 쓰는 JavaScript 런타임(Deno) 설치 폴더 (공식 설치 스크립트 기본값)
DENO_INSTALL_DIR="$HOME/.deno"

# 리눅스용 YouTube 영상 분석 도구(analyzeManager 의 youtube_* 전용 빌드) 프로젝트와 실행 파일
YOUTUBE_ANALYZER_PROJECT='analyzeManager/portable/AnalyzeManager.Portable.csproj'
YOUTUBE_ANALYZER_EXE='analyzeManager/portable/bin/Release/net10.0/Netstorm.AnalyzeManager'

# 셸 시작 파일에 추가하는 줄 끝에 붙이는 표식 (중복 추가 방지·식별용)
PROFILE_TAG='# NetstormReborn PREPARE.sh'

# 프로젝트 루트 경로 (이 스크립트가 있는 폴더)
PROJECT_ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

# Ghidra, vcpkg 처럼 패키지 관리자로 설치하지 않는 도구를 설치할 폴더 (--tools-dir 로 변경)
TOOLS_DIR="$HOME/Tools"

# 설치하지 않고 점검만 할지 여부 (--check-only)
CHECK_ONLY=0

# 선택 화면 없이 모든 항목을 대상으로 할지 여부 (--all)
SELECT_ALL=0

# 터미널 출력일 때만 사용할 색상 코드 (파이프·파일로 출력하면 색상 없음)
if [ -t 1 ]; then
    C_RED=$'\e[31m'; C_GREEN=$'\e[32m'; C_YELLOW=$'\e[33m'; C_CYAN=$'\e[36m'; C_GRAY=$'\e[90m'; C_RESET=$'\e[0m'
else
    C_RED=''; C_GREEN=''; C_YELLOW=''; C_CYAN=''; C_GRAY=''; C_RESET=''
fi


# ============================================================
# 공통 함수
# ============================================================

# 색상을 지정해 한 줄을 출력한다 (인자: 색상 코드, 문자열)
say() { printf '%s%s%s\n' "$1" "$2" "$C_RESET"; }

# 오류 메시지를 빨간색으로 출력한다
err() { say "$C_RED" "    $1" >&2; }

# 명령이 PATH 에 있는지 확인한다
has_cmd() { command -v "$1" >/dev/null 2>&1; }

# 명령을 실행해 표준 출력·표준 오류를 합친 첫 줄을 앞뒤 공백 없이 돌려준다
first_line() { "$@" 2>&1 | head -n 1 | sed 's/^[[:space:]]*//; s/[[:space:]]*$//'; }

# 버전 문자열 비교: 첫째 인자가 둘째 인자 이상이면 성공
version_ge() { [ "$(printf '%s\n%s\n' "$2" "$1" | sort -V | head -n 1)" = "$2" ]; }

# 공유 라이브러리가 시스템에 등록되어 있는지 확인한다 (ldconfig 가 PATH 에 없는 배포판 대비)
has_lib() {
    { ldconfig -p 2>/dev/null || /sbin/ldconfig -p 2>/dev/null; } | grep -q "$1"
}

# root 권한으로 명령을 실행한다 (이미 root 면 그대로, 아니면 sudo)
run_root() {
    if [ "$(id -u)" -eq 0 ]; then
        "$@"
    elif has_cmd sudo; then
        sudo "$@"
    else
        err "root 권한이 필요하지만 sudo 가 없습니다: $*"
        return 1
    fi
}

# 사용 중인 패키지 관리자를 찾아 PKG_MANAGER 에 기록한다 (없으면 빈 문자열)
detect_pkg_manager() {
    local pm
    PKG_MANAGER=''
    # 우선순위 순으로 명령 존재 여부를 확인
    for pm in apt-get dnf pacman zypper; do
        if has_cmd "$pm"; then PKG_MANAGER="${pm%-get}"; return; fi
    done
}

# 패키지 목록을 한 번만 갱신한다 (apt 만 명시적 갱신 필요, 나머지는 설치 시 자동 갱신)
PKG_REFRESHED=0
pkg_refresh() {
    [ "$PKG_REFRESHED" -eq 1 ] && return 0
    if [ "$PKG_MANAGER" = apt ]; then run_root apt-get update || return 1; fi
    PKG_REFRESHED=1
}

# 패키지 이름들을 현재 패키지 관리자로 설치한다
pkg_install_names() {
    pkg_refresh || return 1
    case "$PKG_MANAGER" in
        apt)    run_root env DEBIAN_FRONTEND=noninteractive apt-get install -y "$@" ;;
        dnf)    run_root dnf install -y "$@" ;;
        pacman) run_root pacman -S --needed --noconfirm "$@" ;;
        zypper) run_root zypper --non-interactive install "$@" ;;
        *)      err '지원하는 패키지 관리자(apt, dnf, pacman, zypper)를 찾지 못했습니다.'; return 1 ;;
    esac
}

# 패키지 관리자별 패키지 이름을 골라 설치한다
# 인자: apt, dnf, pacman, zypper 용 패키지 목록 (공백 구분 문자열, 빈 문자열이면 해당 배포판 미지원)
pkg_install() {
    local list='' names
    case "$PKG_MANAGER" in
        apt) list=$1 ;; dnf) list=$2 ;; pacman) list=$3 ;; zypper) list=$4 ;;
    esac
    if [ -z "$list" ]; then
        err "이 배포판($PKG_MANAGER)용 패키지 정보가 없습니다. 직접 설치하세요."
        return 1
    fi
    # 목록을 공백 기준으로 나눠 개별 패키지 인자로 전달
    read -r -a names <<< "$list"
    pkg_install_names "${names[@]}"
}

# pip 로 사용자 영역에 패키지를 설치한다 (PEP 668 로 거부되면 --break-system-packages 로 재시도)
pip_install_user() {
    if ! python3 -m pip --version >/dev/null 2>&1; then
        err 'pip 이 없습니다. Python 3 항목(python3-pip)을 먼저 설치하세요.'
        return 1
    fi
    python3 -m pip install --user --upgrade "$@" && return 0
    python3 -m pip install --user --upgrade --break-system-packages "$@"
}

# 셸 시작 파일(~/.profile 과, 있으면 ~/.bash_profile·~/.bashrc·~/.zshrc)에 한 줄을 추가한다 (이미 있으면 건너뜀)
ensure_profile_line() {
    local line="$1 $PROFILE_TAG" f
    local files=("$HOME/.profile")
    [ -f "$HOME/.bash_profile" ] && files+=("$HOME/.bash_profile")
    [ -f "$HOME/.bashrc" ] && files+=("$HOME/.bashrc")
    [ -f "$HOME/.zshrc" ] && files+=("$HOME/.zshrc")
    # 각 시작 파일마다 같은 줄이 없을 때만 끝에 추가
    for f in "${files[@]}"; do
        if ! grep -Fqx -- "$line" "$f" 2>/dev/null; then
            printf '\n%s\n' "$line" >> "$f"
            echo "    $f 에 환경 설정을 추가했습니다."
        fi
    done
}

# 이 스크립트가 설치하는 사용자 폴더들을 현재 세션의 PATH·환경 변수에 반영한다 (설치 직후 명령을 찾기 위함)
update_session_path() {
    local dir
    # PATH 앞에 붙일 후보 폴더를 차례로 검사해 존재하고 아직 없는 것만 추가
    for dir in "$LOCAL_BIN_DIR" "$DOTNET_INSTALL_DIR/tools" "$DOTNET_INSTALL_DIR"; do
        if [ -d "$dir" ]; then
            case ":$PATH:" in *":$dir:"*) ;; *) PATH="$dir:$PATH" ;; esac
        fi
    done
    export PATH
    # 사용자 폴더에 .NET 이 있으면 DOTNET_ROOT 도 지정 (dotnet 전역 도구가 런타임을 찾는 데 필요)
    if [ -x "$DOTNET_INSTALL_DIR/dotnet" ]; then export DOTNET_ROOT="$DOTNET_INSTALL_DIR"; fi
    # 이전 실행에서 설치한 vcpkg 가 있으면 VCPKG_ROOT 도 세션에 반영
    if [ -z "${VCPKG_ROOT:-}" ] && [ -x "$TOOLS_DIR/vcpkg/vcpkg" ]; then export VCPKG_ROOT="$TOOLS_DIR/vcpkg"; fi
    hash -r
}

# 설치된 .NET SDK 중 지정한 주 버전의 최신 SDK 버전 문자열을 출력한다 (없으면 출력 없음)
get_dotnet_sdk() {
    has_cmd dotnet || return 0
    # 'dotnet --list-sdks' 출력 줄('10.0.100 [경로]')마다 주 버전을 비교해 마지막(최신) 것을 남긴다
    dotnet --list-sdks 2>/dev/null | awk -v m="$1" '{ split($1, v, "."); if (v[1] == m) f = $1 } END { if (f) print f }'
}

# TOOLS_DIR 에서 Ghidra 설치 폴더를 찾아 출력한다 (가장 최신 이름 우선)
get_ghidra_dir() {
    [ -d "$TOOLS_DIR" ] || return 0
    local dirs=("$TOOLS_DIR"/ghidra_*) i
    # glob 결과는 이름 오름차순이므로 뒤에서부터 검사해 실행 스크립트(ghidraRun)가 있는 첫 폴더를 고른다
    for ((i = ${#dirs[@]} - 1; i >= 0; i--)); do
        if [ -f "${dirs[$i]}/ghidraRun" ]; then echo "${dirs[$i]}"; return 0; fi
    done
}

# vcpkg 설치 폴더를 찾아 출력한다 (VCPKG_ROOT 우선, 없으면 TOOLS_DIR/vcpkg)
get_vcpkg_dir() {
    local dir
    # 확인할 후보 경로를 차례로 검사
    for dir in "${VCPKG_ROOT:-}" "$TOOLS_DIR/vcpkg"; do
        if [ -n "$dir" ] && [ -x "$dir/vcpkg" ]; then echo "$dir"; return 0; fi
    done
}


# ============================================================
# 설치 항목별 점검(check_*)·설치(install_*) 함수
#   check_*   : 설치되어 있으면 상태 문자열을 출력하고, 아니면 아무것도 출력하지 않는다
#   install_* : 설치를 수행하고, 실패하면 0 이 아닌 값을 돌려준다
# ============================================================

# --- 기본 도구: 다운로드·압축 해제에 쓰는 curl, unzip, tar ---
check_base() {
    local missing=0 c
    # 필요한 명령마다 존재 여부 확인
    for c in curl unzip tar; do has_cmd "$c" || missing=1; done
    [ "$missing" -eq 0 ] && echo 'curl, unzip, tar'
}
install_base() {
    pkg_install 'curl unzip tar ca-certificates' 'curl unzip tar ca-certificates' \
                'curl unzip tar ca-certificates' 'curl unzip tar ca-certificates'
}

# --- Git ---
check_git() { has_cmd git && first_line git --version; }
install_git() { pkg_install 'git' 'git' 'git' 'git'; }

# --- 게임 빌드·실행 라이브러리: .NET 이 쓰는 ICU·OpenSSL, MonoGame DesktopGL 이 쓰는 OpenGL ---
#     (SDL2·OpenAL 은 MonoGame NuGet 패키지에 포함되어 있어 따로 설치하지 않는다)
check_runtime_libs() {
    has_lib 'libicuuc\.so' && has_lib 'libssl\.so' && has_lib 'libGL\.so\.1' && echo 'ICU, OpenSSL, OpenGL'
}
install_runtime_libs() {
    pkg_install 'libicu-dev libssl-dev zlib1g libgl1' \
                'libicu openssl-libs zlib mesa-libGL' \
                'icu openssl zlib mesa' \
                'libicu-devel libopenssl3 libz1 Mesa-libGL1'
}

# --- .NET SDK ---
check_dotnet() { local v; v=$(get_dotnet_sdk "$REQUIRED_DOTNET_MAJOR"); [ -n "$v" ] && echo "SDK $v"; }
install_dotnet() {
    local script
    script=$(mktemp) || return 1
    # 배포판 저장소마다 제공 버전이 달라 Microsoft 공식 설치 스크립트로 사용자 폴더에 설치한다
    curl -fsSL https://dot.net/v1/dotnet-install.sh -o "$script" || { rm -f "$script"; err 'dotnet-install.sh 다운로드 실패'; return 1; }
    bash "$script" --channel "$REQUIRED_DOTNET_MAJOR.0" --install-dir "$DOTNET_INSTALL_DIR"
    local rc=$?
    rm -f "$script"
    [ "$rc" -eq 0 ] || return 1
    # 새 터미널에서도 dotnet 을 찾도록 셸 시작 파일에 등록
    ensure_profile_line 'export DOTNET_ROOT="$HOME/.dotnet"'
    ensure_profile_line 'export PATH="$HOME/.dotnet:$HOME/.dotnet/tools:$PATH"'
}

# --- MonoGame 템플릿 ---
check_monogame() {
    has_cmd dotnet || return 0
    # DesktopGL 템플릿(짧은 이름 mgdesktopgl)이 목록에 있는지 확인
    dotnet new list mgdesktopgl 2>/dev/null | grep -q mgdesktopgl && echo '설치됨 (mgdesktopgl)'
}
install_monogame() {
    has_cmd dotnet || { err '.NET SDK 가 없습니다. .NET SDK 항목을 먼저 설치하세요.'; return 1; }
    dotnet new install "$MONOGAME_TEMPLATE_PACKAGE"
}

# --- Python 3 ---
check_python() {
    has_cmd python3 || return 0
    local line; line=$(first_line python3 --version)
    # "Python 3.13.1" 형식에서 주.부 버전을 뽑아 최소 버전과 비교
    if [[ $line =~ Python\ ([0-9]+\.[0-9]+) ]] && version_ge "${BASH_REMATCH[1]}" "$MIN_PYTHON_VERSION"; then echo "$line"; fi
}
install_python() {
    pkg_install 'python3 python3-pip python3-venv' 'python3 python3-pip' 'python python-pip' 'python3 python3-pip'
}

# 설치되지 않은 Python 패키지(pip 이름)를 공백 구분으로 출력한다
missing_python_packages() {
    local missing=() i
    # 각 패키지를 import 해 보고 실패한 것을 모은다
    for i in "${!PYTHON_PACKAGES[@]}"; do
        python3 -c "import ${PYTHON_MODULES[$i]}" >/dev/null 2>&1 || missing+=("${PYTHON_PACKAGES[$i]}")
    done
    echo "${missing[*]}"
}

# --- Python 패키지 ---
check_pypkgs() { has_cmd python3 && [ -z "$(missing_python_packages)" ] && echo '모두 설치됨'; }
install_pypkgs() {
    has_cmd python3 || { err 'Python 이 없습니다. Python 3 항목을 먼저 설치하세요.'; return 1; }
    # 배포판 패키지를 먼저 시도한다 (PEP 668 로 시스템 Python 에 pip 설치가 막힌 배포판이 많음)
    pkg_install 'python3-pil python3-pefile python3-capstone' \
                'python3-pillow python3-pefile python3-capstone' \
                'python-pillow python-pefile python-capstone' \
                'python3-Pillow python3-pefile python3-capstone' || echo '    배포판 패키지 설치 실패 → pip 로 설치합니다.'
    local missing pkgs; missing=$(missing_python_packages)
    [ -z "$missing" ] && return 0
    read -r -a pkgs <<< "$missing"
    pip_install_user "${pkgs[@]}"
}

# --- C++ 빌드 도구 ---
check_cxx() { has_cmd g++ && has_cmd make && first_line g++ --version; }
install_cxx() { pkg_install 'build-essential' 'gcc gcc-c++ make' 'base-devel' 'gcc gcc-c++ make'; }

# --- CMake ---
check_cmake() { has_cmd cmake && first_line cmake --version; }
install_cmake() { pkg_install 'cmake' 'cmake' 'cmake' 'cmake'; }

# --- Ninja (배포판에 따라 명령 이름이 ninja-build 인 경우도 있음) ---
check_ninja() {
    if has_cmd ninja; then echo "ninja $(first_line ninja --version)"
    elif has_cmd ninja-build; then echo "ninja-build $(first_line ninja-build --version)"; fi
}
install_ninja() { pkg_install 'ninja-build' 'ninja-build' 'ninja' 'ninja'; }

# --- JDK (Ghidra 는 JRE 가 아닌 JDK 가 필요하므로 javac 로 확인) ---
check_jdk() {
    has_cmd javac || return 0
    local line ver major
    line=$(first_line javac -version)   # 'javac 21.0.4', 'javac 1.8.0_392' 형식
    ver=${line#javac }
    major=${ver%%.*}
    # Java 8 이하는 '1.x' 형식이므로 두 번째 숫자가 주 버전
    if [ "$major" = 1 ]; then major=$(echo "$ver" | cut -d. -f2); fi
    if [[ $major =~ ^[0-9]+$ ]] && [ "$major" -ge "$MIN_JDK_MAJOR" ]; then echo "$line"; fi
}
install_jdk() {
    pkg_install "openjdk-$MIN_JDK_MAJOR-jdk" "java-$MIN_JDK_MAJOR-openjdk-devel" \
                "jdk$MIN_JDK_MAJOR-openjdk" "java-$MIN_JDK_MAJOR-openjdk-devel"
}

# --- Ghidra ---
check_ghidra() { get_ghidra_dir; }
install_ghidra() {
    local json url tag tmp dir
    # 패키지 관리자 미제공 → GitHub 최신 릴리스 zip 을 받아 압축 해제
    json=$(curl -fsSL https://api.github.com/repos/NationalSecurityAgency/ghidra/releases/latest) \
        || { err 'GitHub 릴리스 정보를 가져오지 못했습니다.'; return 1; }
    url=$(printf '%s\n' "$json" | grep -o '"browser_download_url": *"[^"]*/ghidra_[^"]*_PUBLIC_[^"]*\.zip"' \
          | head -n 1 | sed 's/.*"\(https[^"]*\)"$/\1/')
    tag=$(printf '%s\n' "$json" | grep -o '"tag_name": *"[^"]*"' | head -n 1 | sed 's/.*"\([^"]*\)"$/\1/')
    [ -n "$url" ] || { err 'Ghidra 릴리스 파일을 찾지 못했습니다.'; return 1; }
    mkdir -p "$TOOLS_DIR" || return 1
    tmp=$(mktemp -d) || return 1
    echo "    $tag 다운로드 중 (수백 MB)..."
    if ! curl -fL --progress-bar -o "$tmp/ghidra.zip" "$url"; then rm -rf "$tmp"; err '다운로드 실패'; return 1; fi
    echo '    압축 해제 중...'
    if ! unzip -q "$tmp/ghidra.zip" -d "$TOOLS_DIR"; then rm -rf "$tmp"; err '압축 해제 실패'; return 1; fi
    rm -rf "$tmp"
    # zip 에서 실행 권한이 빠진 경우를 대비해 실행 스크립트와 Linux 네이티브 도구(디컴파일러 등)에 권한 부여
    dir=$(get_ghidra_dir)
    if [ -n "$dir" ]; then
        chmod +x "$dir/ghidraRun"
        find "$dir" \( -name '*.sh' -o -path '*/os/linux_*' \) -type f -exec chmod +x {} +
    fi
}

# --- Wine: 원본 Netstorm.exe(32bit) 실행·동적 분석 ---
check_wine() { has_cmd wine && first_line wine --version; }
install_wine() {
    if [ "$PKG_MANAGER" = apt ]; then
        # 32bit exe 실행에 필요한 i386 아키텍처를 추가하고 패키지 목록을 다시 받는다
        if ! dpkg --print-foreign-architectures | grep -qx i386; then
            run_root dpkg --add-architecture i386 || return 1
            PKG_REFRESHED=0
        fi
        pkg_install_names wine wine32:i386
    else
        # Arch 는 multilib 저장소가 활성화되어 있어야 한다
        pkg_install '' 'wine' 'wine' 'wine' || { [ "$PKG_MANAGER" = pacman ] && err '/etc/pacman.conf 의 [multilib] 저장소를 활성화했는지 확인하세요.'; return 1; }
    fi
}

# --- git safe.directory 등록 ---
check_safedir() {
    has_cmd git || return 0
    local d
    # 등록된 경로 중 프로젝트 루트(또는 전체 허용 '*')가 있는지 확인
    while IFS= read -r d; do
        if [ "$d" = "$PROJECT_ROOT" ] || [ "$d" = '*' ]; then echo "$d"; return 0; fi
    done < <(git config --global --get-all safe.directory 2>/dev/null)
}
install_safedir() {
    has_cmd git || { err 'Git 이 없습니다. Git 항목을 먼저 설치하세요.'; return 1; }
    git config --global --add safe.directory "$PROJECT_ROOT"
}

# --- vcpkg ---
check_vcpkg() { get_vcpkg_dir; }
install_vcpkg() {
    has_cmd git || { err 'Git 이 없습니다. Git 항목을 먼저 설치하세요.'; return 1; }
    local dir="$TOOLS_DIR/vcpkg"
    # vcpkg 부트스트랩에 필요한 도구 (zip, pkg-config)
    pkg_install 'zip pkg-config' 'zip pkgconf-pkg-config' 'zip pkgconf' 'zip pkg-config' || return 1
    mkdir -p "$TOOLS_DIR" || return 1
    # 이미 받아 둔 저장소가 있으면 clone 을 건너뛴다
    if [ ! -d "$dir" ]; then git clone https://github.com/microsoft/vcpkg "$dir" || return 1; fi
    "$dir/bootstrap-vcpkg.sh" -disableMetrics || return 1
    # CMake 가 vcpkg 를 찾을 수 있도록 환경 변수 등록
    export VCPKG_ROOT="$dir"
    ensure_profile_line "export VCPKG_ROOT=\"$dir\""
}

# --- VS Code 확장 ---
check_vscode() {
    has_cmd code || return 0
    local installed ext; installed=$(code --list-extensions 2>/dev/null)
    # 필요한 확장이 모두 목록에 있는지 확인
    for ext in "${VSCODE_EXTENSIONS[@]}"; do
        grep -Fqxi -- "$ext" <<< "$installed" || return 0
    done
    echo '모두 설치됨'
}
install_vscode() {
    has_cmd code || { err 'VS Code 가 없습니다. https://code.visualstudio.com 에서 먼저 설치하세요.'; return 1; }
    local ext
    # 확장 목록을 순회하며 설치 (이미 설치된 확장은 VS Code 가 건너뜀)
    for ext in "${VSCODE_EXTENSIONS[@]}"; do code --install-extension "$ext" || return 1; done
}

# --- yt-dlp ---
check_ytdlp() { has_cmd yt-dlp && echo "yt-dlp $(first_line yt-dlp --version)"; }
install_ytdlp() {
    has_cmd python3 || { err 'Python 이 없습니다. Python 3 항목을 먼저 설치하세요.'; return 1; }
    # 배포판 패키지는 오래되어 YouTube 변경을 못 따라가는 경우가 많아 공식 최신 실행 파일을 받는다
    mkdir -p "$LOCAL_BIN_DIR" || return 1
    curl -fL --progress-bar -o "$LOCAL_BIN_DIR/yt-dlp" https://github.com/yt-dlp/yt-dlp/releases/latest/download/yt-dlp || return 1
    chmod a+rx "$LOCAL_BIN_DIR/yt-dlp"
    ensure_profile_line 'case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) export PATH="$HOME/.local/bin:$PATH" ;; esac'
}

# --- FFmpeg (Fedora 기본 저장소는 ffmpeg-free 패키지) ---
# ffmpeg·ffprobe 가 모두 있고, ffprobe 가 호스트 이름 해석에서 죽지 않아야 한다(원격 YouTube 스트림 읽기).
# 확인용 주소는 로컬(포트 9, 닫힘)이라 인터넷에 접속하지 않는다. 종료 코드 139 = 세그멘테이션 오류.
check_ffmpeg() {
    has_cmd ffmpeg && has_cmd ffprobe || return 1
    local code=0
    ffprobe -v quiet -rw_timeout 2000000 -i 'http://localhost:9/check.mp4' >/dev/null 2>&1 || code=$?
    if [ "$code" -ge 128 ]; then
        err "ffprobe 가 호스트 이름 해석 중 비정상 종료합니다(코드 $code). DNS 가 동작하는 빌드로 바꿔야 합니다: $(command -v ffprobe)"
        return 1
    fi
    first_line ffmpeg -version
}
# 시스템 패키지를 먼저 시도하고, 실패하면(sudo 없음 등) 정적 빌드를 사용자 실행 파일 폴더에 받는다
install_ffmpeg() {
    if [ "$(id -u)" -eq 0 ] || has_cmd sudo; then
        pkg_install 'ffmpeg' 'ffmpeg-free' 'ffmpeg' 'ffmpeg' && check_ffmpeg >/dev/null && return 0
        say "$C_YELLOW" '    패키지 설치에 실패해 정적 빌드를 사용자 폴더에 받습니다.'
    fi
    install_ffmpeg_static
}
# BtbN 정적 빌드의 ffmpeg·ffprobe 를 $LOCAL_BIN_DIR 에 설치한다 (sudo 불필요)
install_ffmpeg_static() {
    local work
    work="$(mktemp -d)" || return 1
    mkdir -p "$LOCAL_BIN_DIR" || return 1
    if curl -fL --progress-bar -o "$work/ffmpeg.tar.xz" "$FFMPEG_STATIC_URL" \
        && tar -xJf "$work/ffmpeg.tar.xz" -C "$work" --wildcards '*/bin/ffmpeg' '*/bin/ffprobe'; then
        install -m 755 "$work"/*/bin/ffmpeg "$work"/*/bin/ffprobe "$LOCAL_BIN_DIR/" || { rm -rf "$work"; return 1; }
    else
        rm -rf "$work"; return 1
    fi
    rm -rf "$work"
    ensure_profile_line 'case ":$PATH:" in *":$HOME/.local/bin:"*) ;; *) export PATH="$HOME/.local/bin:$PATH" ;; esac'
    # PATH 앞쪽에 DNS 가 안 되는 다른 ffmpeg 가 있으면 사용자 폴더 것을 쓰도록 안내한다
    case ":$PATH:" in *":$LOCAL_BIN_DIR:"*) ;; *) export PATH="$LOCAL_BIN_DIR:$PATH" ;; esac
}

# --- Deno (yt-dlp 의 YouTube JavaScript 해석용 런타임, sudo 불필요) ---
check_deno() {
    if has_cmd deno; then first_line deno --version; return 0; fi
    [ -x "$DENO_INSTALL_DIR/bin/deno" ] && first_line "$DENO_INSTALL_DIR/bin/deno" --version
}
install_deno() {
    # 공식 설치 스크립트가 $DENO_INSTALL_DIR/bin 에 설치한다 (-y: 셸 설정 질문 생략)
    curl -fsSL https://deno.land/install.sh | DENO_INSTALL="$DENO_INSTALL_DIR" sh -s -- -y || return 1
    ensure_profile_line 'case ":$PATH:" in *":$HOME/.deno/bin:"*) ;; *) export PATH="$HOME/.deno/bin:$PATH" ;; esac'
    case ":$PATH:" in *":$DENO_INSTALL_DIR/bin:"*) ;; *) export PATH="$DENO_INSTALL_DIR/bin:$PATH" ;; esac
}

# --- 리눅스용 YouTube 영상 분석 도구 (analyzeManager youtube_* 전용 빌드, 원본 게임 실행 없음) ---
check_ytanalyzer() {
    [ -x "$PROJECT_ROOT/$YOUTUBE_ANALYZER_EXE" ] && echo "$YOUTUBE_ANALYZER_EXE"
}
install_ytanalyzer() {
    has_cmd dotnet || { err '.NET SDK 가 없습니다. .NET SDK 항목을 먼저 설치하세요.'; return 1; }
    dotnet build "$PROJECT_ROOT/$YOUTUBE_ANALYZER_PROJECT" -c Release -p:NuGetAudit=false
}


# ============================================================
# 인자 처리
# ============================================================

# 명령줄 인자를 하나씩 해석
while [ $# -gt 0 ]; do
    case "$1" in
        --check-only) CHECK_ONLY=1 ;;
        --all)        SELECT_ALL=1 ;;
        --tools-dir)  shift; TOOLS_DIR="${1:?--tools-dir 에 폴더를 지정하세요}" ;;
        -h|--help)    sed -n '3,18p' "${BASH_SOURCE[0]}" | sed 's/^# \{0,1\}//'; exit 0 ;;
        *)            echo "알 수 없는 인자: $1 (--help 참고)" >&2; exit 1 ;;
    esac
    shift
done


# ============================================================
# 설치 항목 정의
#   add_item <ID> <분류> <기본 선택(1/0)> <이름> <설명>
#   ID 는 check_<ID>, install_<ID> 함수와 짝을 이룬다.
#   목록 순서가 곧 설치 순서이므로 의존 관계(기본 도구 → 나머지, 라이브러리 → .NET SDK → MonoGame 템플릿,
#   Python → 패키지·yt-dlp, JDK → Ghidra, Git → safe.directory·vcpkg, .NET SDK → YouTube 분석 도구)를 지켜 배치한다
# ============================================================
ITEM_IDS=(); ITEM_GROUPS=(); ITEM_DEFAULTS=(); ITEM_NAMES=(); ITEM_DESCS=()

# 항목 하나를 목록 끝에 추가한다
add_item() {
    ITEM_IDS+=("$1"); ITEM_GROUPS+=("$2"); ITEM_DEFAULTS+=("$3"); ITEM_NAMES+=("$4"); ITEM_DESCS+=("$5")
}

add_item base         '필수' 1 '기본 도구'                  '다운로드·압축 해제 (curl, unzip, tar)'
add_item git          '필수' 1 'Git'                        '소스 관리'
add_item runtime_libs '필수' 1 '빌드·실행 라이브러리'       '.NET 용 ICU·OpenSSL, MonoGame DesktopGL 용 OpenGL'
add_item dotnet       '필수' 1 ".NET SDK $REQUIRED_DOTNET_MAJOR" "게임 빌드 (C# + MonoGame, 대상 net10.0, $DOTNET_INSTALL_DIR 에 설치)"
add_item monogame     '권장' 1 'MonoGame 템플릿'            "dotnet new 용 MonoGame 프로젝트 템플릿 ($MONOGAME_TEMPLATE_PACKAGE)"
add_item python       '필수' 1 'Python 3'                   "포맷 분석·추출 도구 ($MIN_PYTHON_VERSION 이상)"
add_item pypkgs       '필수' 1 'Python 패키지'              "${PYTHON_PACKAGES[*]}"
add_item cxx          '선택' 0 'C++ 빌드 도구'              'gcc, g++, make (C# 확정으로 현재 불필요)'
add_item cmake        '선택' 0 'CMake'                      '빌드 시스템 생성기 (C# 확정으로 현재 불필요)'
add_item ninja        '선택' 0 'Ninja'                      '빌드 실행기 (C# 확정으로 현재 불필요)'
add_item jdk          '필수' 1 "JDK $MIN_JDK_MAJOR+"        'Ghidra 실행용 Java'
add_item ghidra       '필수' 1 'Ghidra'                     "Netstorm.exe 정적 분석 ($TOOLS_DIR 에 설치)"
add_item wine         '권장' 0 'Wine'                       '원본 exe 실행·winedbg 동적 분석·WINEDEBUG=+file 파일 접근 관찰 (x64dbg·Process Monitor 대체)'
add_item safedir      '필수' 1 'git safe.directory 등록'    "'dubious ownership' 오류 해결 (git 전역 설정 변경)"
add_item vcpkg        '선택' 0 'vcpkg'                      "C++ 라이브러리 관리자 ($TOOLS_DIR 에 설치, C# 확정으로 현재 불필요)"
add_item vscode       '권장' 0 'VS Code 확장'               "${VSCODE_EXTENSIONS[*]}"
add_item ytdlp        '선택' 0 'yt-dlp'                     "YouTube 플레이 영상 조회·스트림 주소 (analyzeManager youtube_*, $LOCAL_BIN_DIR 에 설치)"
add_item deno         '선택' 0 'Deno'                       "yt-dlp 의 YouTube 해석용 JavaScript 런타임 ($DENO_INSTALL_DIR 에 설치)"
add_item ffmpeg       '선택' 0 'FFmpeg'                     "영상 프레임 추출·원격 스트림 읽기 (패키지 실패 시 정적 빌드를 $LOCAL_BIN_DIR 에)"
add_item ytanalyzer   '선택' 0 'YouTube 분석 도구 (리눅스)' "analyzeManager 의 youtube_* 전용 빌드 ($YOUTUBE_ANALYZER_PROJECT, .NET SDK 필요)"


# ============================================================
# 선택 화면
# ============================================================

# 항목 목록을 보여 주고 사용자가 번호로 선택을 토글하게 한다.
# 진행하면 선택된 항목 번호(0부터)를 TARGETS 배열에 담고 성공, 종료(q)하면 실패를 돌려준다.
select_items() {
    local i answer token start end n mark tokens
    local selected=("${ITEM_DEFAULTS[@]}")
    local count=${#ITEM_IDS[@]}

    # 사용자가 진행(엔터) 또는 종료(q)를 입력할 때까지 반복
    while true; do
        echo
        say "$C_CYAN" '=== 설치 항목 선택 ==='
        # 각 항목을 번호·선택 상태와 함께 출력
        for ((i = 0; i < count; i++)); do
            mark='[ ]'; [ "${selected[$i]}" = 1 ] && mark='[x]'
            printf '%3d. %s (%s) %s' $((i + 1)) "$mark" "${ITEM_GROUPS[$i]}" "${ITEM_NAMES[$i]}"
            say "$C_GRAY" "  - ${ITEM_DESCS[$i]}"
        done
        echo
        echo '번호를 입력하면 선택/해제됩니다 (여러 개는 공백·쉼표로 구분, 범위는 1-5).'
        # 입력이 끊기면(EOF) 종료로 간주
        read -r -p 'a=전체 선택, n=전체 해제, r=필수만, 엔터=진행, q=종료: ' answer || return 1
        # 앞뒤 공백 제거
        answer="${answer#"${answer%%[![:space:]]*}"}"
        answer="${answer%"${answer##*[![:space:]]}"}"

        case "$answer" in
            '')
                TARGETS=()
                # 선택된 항목 번호만 모은다
                for ((i = 0; i < count; i++)); do [ "${selected[$i]}" = 1 ] && TARGETS+=("$i"); done
                return 0 ;;
            q) return 1 ;;
            # 전체 선택 / 전체 해제 / 필수만 선택
            a) for ((i = 0; i < count; i++)); do selected[$i]=1; done ;;
            n) for ((i = 0; i < count; i++)); do selected[$i]=0; done ;;
            r) for ((i = 0; i < count; i++)); do
                   if [ "${ITEM_GROUPS[$i]}" = '필수' ]; then selected[$i]=1; else selected[$i]=0; fi
               done ;;
            *)
                read -r -a tokens <<< "${answer//,/ }"
                # 입력을 토큰 단위로 나눠 번호 또는 범위(예: 3-6)로 해석
                for token in "${tokens[@]}"; do
                    if [[ $token =~ ^([0-9]+)-([0-9]+)$ ]]; then
                        start=$((10#${BASH_REMATCH[1]})); end=$((10#${BASH_REMATCH[2]}))
                    elif [[ $token =~ ^[0-9]+$ ]]; then
                        start=$((10#$token)); end=$start
                    else
                        say "$C_YELLOW" "  잘못된 입력: $token"
                        continue
                    fi
                    # 범위 안의 번호마다 선택 상태를 뒤집는다
                    for ((n = start; n <= end; n++)); do
                        if ((n >= 1 && n <= count)); then
                            selected[$((n - 1))]=$((1 - selected[n - 1]))
                        else
                            say "$C_YELLOW" "  범위를 벗어난 번호: $n"
                        fi
                    done
                done ;;
        esac
    done
}


# ============================================================
# 메인
# ============================================================

say "$C_GREEN" 'NetStorm 클론 개발 환경 준비 (Linux)'
[ "$CHECK_ONLY" -eq 1 ] && say "$C_YELLOW" '(점검 전용 모드: 설치하지 않습니다)'

# sudo 로 실행하면 ~/.dotnet 등이 root 홈에 설치되므로 막는다 (필요한 곳에서 스크립트가 sudo 를 호출)
if [ "$(id -u)" -eq 0 ] && [ -n "${SUDO_USER:-}" ]; then
    say "$C_RED" 'sudo 없이 일반 사용자로 실행하세요. 시스템 패키지 설치 시 스크립트가 sudo 를 호출합니다.'
    exit 1
fi

detect_pkg_manager
if [ "$CHECK_ONLY" -eq 0 ] && [ -z "$PKG_MANAGER" ]; then
    say "$C_YELLOW" '지원하는 패키지 관리자(apt, dnf, pacman, zypper)가 없습니다. 시스템 패키지 항목은 설치에 실패합니다.'
fi

# 이전 실행에서 사용자 폴더에 설치한 도구도 찾을 수 있도록 PATH 반영
update_session_path

if [ "$SELECT_ALL" -eq 1 ]; then
    TARGETS=("${!ITEM_IDS[@]}")
elif ! select_items; then
    TARGETS=()
fi
if [ ${#TARGETS[@]} -eq 0 ]; then
    echo '선택된 항목이 없어 종료합니다.'
    exit 0
fi

# 결과 요약용 배열 (항목 이름, 결과, 상세)
RES_NAMES=(); RES_RESULTS=(); RES_DETAILS=()

# 결과 한 건을 요약 배열에 추가한다
add_result() { RES_NAMES+=("$1"); RES_RESULTS+=("$2"); RES_DETAILS+=("$3"); }

# 선택된 항목을 순서대로 점검하고, 없으면 설치 후 다시 점검
for i in "${TARGETS[@]}"; do
    id=${ITEM_IDS[$i]}
    name=${ITEM_NAMES[$i]}
    echo
    say "$C_CYAN" "▶ $name"
    status=$("check_$id")

    if [ -n "$status" ]; then
        say "$C_GREEN" "    이미 설치됨: $status"
        add_result "$name" '이미 설치됨' "$status"
        continue
    fi

    if [ "$CHECK_ONLY" -eq 1 ]; then
        say "$C_YELLOW" '    설치되어 있지 않음'
        add_result "$name" '없음' ''
        continue
    fi

    say "$C_YELLOW" '    설치되어 있지 않음 → 설치를 시작합니다.'
    if "install_$id"; then
        update_session_path
        status=$("check_$id")
        if [ -n "$status" ]; then
            say "$C_GREEN" "    설치 완료: $status"
            add_result "$name" '설치 완료' "$status"
        else
            say "$C_RED" '    설치 후에도 확인되지 않습니다. 터미널을 다시 열고 --check-only 로 재점검하세요.'
            add_result "$name" '확인 실패' '재시작 후 재점검 필요'
        fi
    else
        say "$C_RED" '    설치 실패 (위 출력 참고)'
        add_result "$name" '실패' '위 출력 참고'
    fi
done

echo
say "$C_CYAN" '=== 결과 요약 ==='
# 결과를 항목별로 한 줄씩 출력 (한글 폭 때문에 표 대신 목록 형식 사용)
for i in "${!RES_NAMES[@]}"; do
    case "${RES_RESULTS[$i]}" in
        '이미 설치됨'|'설치 완료') color=$C_GREEN ;;
        '없음'|'확인 실패')        color=$C_YELLOW ;;
        *)                          color=$C_RED ;;
    esac
    line="  - ${RES_NAMES[$i]} : ${color}${RES_RESULTS[$i]}${C_RESET}"
    [ -n "${RES_DETAILS[$i]}" ] && line="$line ${C_GRAY}(${RES_DETAILS[$i]})${C_RESET}"
    printf '%s\n' "$line"
done

if [ "$CHECK_ONLY" -eq 0 ]; then
    echo
    echo 'PATH·환경 변수 변경을 반영하려면 터미널(및 VS Code)을 다시 열거나 로그아웃 후 다시 로그인하세요.'
fi
