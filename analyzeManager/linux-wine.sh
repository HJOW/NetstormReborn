#!/usr/bin/env bash
# Linux에서 Windows 전용 분석 도구(analyzeManager)를 Wine으로 준비·호출하는 보조 스크립트.
#
#   bash analyzeManager/linux-wine.sh setup            # win-x86 배포, Wine 접두 경로 생성, 렌더러 설정
#   bash analyzeManager/linux-wine.sh test             # 단위 테스트(가짜 원본, 게임 실행 없음)
#   bash analyzeManager/linux-wine.sh smoke            # MCP 기본 검사(게임 실행 없음)
#   bash analyzeManager/linux-wine.sh call 도구 [JSON] # CLI 한 번 호출
#
# 주의: start_session 등 실제 게임 실행은 AGENTS.md의 실행 확인 규칙을 따른다.
# 산출물은 모두 git 제외 경로 extracted/wine/ 아래에 만든다.
set -euo pipefail

# 저장소 루트 (이 스크립트의 상위 폴더)
REPO="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
# 분석 도구와 테스트의 win-x86 배포 폴더, 전용 Wine 접두 경로
TOOL_DIR="$REPO/extracted/wine/analyzeManager"
TEST_DIR="$REPO/extracted/wine/analyzeManagerTests"
export WINEPREFIX="${WINEPREFIX:-$REPO/extracted/wine/prefix}"
# 원본 게임은 32비트이므로 접두 경로도 32비트로 만든다.
export WINEARCH="${WINEARCH:-win32}"
# Wine 진단 출력이 CLI JSON 출력과 섞이지 않게 끈다.
export WINEDEBUG="${WINEDEBUG:--all}"
# Wine에서 본 저장소 경로 (리눅스 루트 = Z: 드라이브)
WIN_REPO="Z:${REPO//\//\\}"

# 도구·테스트 배포, 접두 경로 생성, DirectDraw 렌더러를 gdi로 설정한다.
setup() {
    # restore를 RID 지정으로 먼저 해야 --no-restore publish가 자체 포함 런타임을 찾는다.
    for project in analyzeManager/AnalyzeManager.csproj analyzeManager/tests/AnalyzeManager.Tests.csproj; do
        dotnet restore "$REPO/$project" -r win-x86 -p:NuGetAudit=false
    done
    dotnet publish "$REPO/analyzeManager/AnalyzeManager.csproj" -c Release -r win-x86 --self-contained true \
        --no-restore -o "$TOOL_DIR" -p:NuGetAudit=false
    dotnet publish "$REPO/analyzeManager/tests/AnalyzeManager.Tests.csproj" -c Release -r win-x86 --self-contained true \
        --no-restore -o "$TEST_DIR" -p:NuGetAudit=false
    wineboot -i >/dev/null 2>&1
    # 기본 OpenGL(wined3d) 렌더링은 GDI 화면 복사에 잡히지 않아 캡처가 검게 나온다(2026-09-29 확인).
    # gdi 렌더러로 바꾸면 캡처된다는 것은 아직 실제 실행으로 확인하지 못한 가설이다.
    wine reg add 'HKCU\Software\Wine\Direct3D' /v renderer /d gdi /f >/dev/null
    echo "준비 완료: WINEPREFIX=$WINEPREFIX"
}

# 인자 수에 따라 하위 명령을 실행한다.
case "${1:-}" in
    setup) setup ;;
    test) wine "$TEST_DIR/AnalyzeManager.Tests.exe" </dev/null ;;
    smoke) python3 "$REPO/analyzeManager/tests/mcp_smoke.py" --wine --exe "$TOOL_DIR/Netstorm.AnalyzeManager.exe" </dev/null ;;
    call)
        [[ $# -ge 2 ]] || { echo "사용법: $0 call 도구 [JSON]" >&2; exit 1; }
        # Wine에서 실행된 게임이 도구의 표준 출력을 물려받아, 파이프로 받으면 게임이 끝날 때까지
        # 셸이 기다린다(2026-09-29 확인). 그래서 결과를 임시 파일로 받은 뒤 출력한다.
        out="$(mktemp)"
        status=0
        wine "$TOOL_DIR/Netstorm.AnalyzeManager.exe" --repo "$WIN_REPO" call "$2" --json "${3:-{\}}" >"$out" 2>&1 </dev/null || status=$?
        cat "$out"
        rm -f "$out"
        exit "$status"
        ;;
    *) sed -n '2,10p' "${BASH_SOURCE[0]}"; exit 1 ;;
esac
