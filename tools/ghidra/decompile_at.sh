#!/usr/bin/env bash
# 이미 만든 Ghidra 프로젝트(extracted/ghidra)에서 지정한 주소의 함수만 디컴파일한다 (decompile_at.ps1 의 Linux 판).
# 전체 디컴파일이 함수로 인식하지 못한 코드를 보고 싶을 때 쓴다. 프로젝트는 읽기 전용으로 열어 변경하지 않는다.
#
# 사용 예 (저장소 루트에서):
#   bash tools/ghidra/decompile_at.sh 484ab0 4c2b20
#   GHIDRA_DIR=~/Tools/ghidra_12.1.4_PUBLIC OUT_FILE=/tmp/at.c bash tools/ghidra/decompile_at.sh 4b1e80
set -euo pipefail

# 저장소 루트 (이 스크립트 기준 두 단계 위)
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/../.." && pwd)"

# 디컴파일할 함수 시작 주소 목록 (16진수, 0x 없이)
if [ "$#" -lt 1 ]; then
    echo "사용법: $0 <16진수 주소>..." >&2
    exit 2
fi

# Ghidra 설치 폴더: 환경 변수가 없으면 ~/Tools 의 최신 버전을 찾는다
GHIDRA_DIR="${GHIDRA_DIR:-$(ls -d "$HOME"/Tools/ghidra_* 2>/dev/null | sort -V | tail -n 1 || true)}"
if [ -z "$GHIDRA_DIR" ] || [ ! -x "$GHIDRA_DIR/support/analyzeHeadless" ]; then
    echo "Ghidra 를 찾지 못했습니다. GHIDRA_DIR 환경 변수로 설치 폴더를 지정하세요." >&2
    exit 1
fi

# 프로젝트 폴더와 이름 (run_decomp.ps1 의 originals 판본과 같다)
PROJECT_DIR="$ROOT/extracted/ghidra"
if [ ! -f "$PROJECT_DIR/Netstorm.gpr" ]; then
    echo "Ghidra 프로젝트가 없습니다. 먼저 전체 디컴파일을 실행하세요: $PROJECT_DIR" >&2
    exit 1
fi

# 결과 파일 기본 위치 (Git 제외 경로)
OUT_FILE="${OUT_FILE:-$ROOT/extracted/decomp-at/originals.c}"
mkdir -p "$(dirname "$OUT_FILE")" "$ROOT/extracted/ghidra-settings" "$ROOT/extracted/ghidra-cache"

# Ghidra 설정과 캐시도 Git 제외 경로에 둔다
export XDG_CONFIG_HOME="$ROOT/extracted/ghidra-settings"
export XDG_CACHE_HOME="$ROOT/extracted/ghidra-cache"

"$GHIDRA_DIR/support/analyzeHeadless" "$PROJECT_DIR" Netstorm \
    -process Netstorm.exe -noanalysis -readOnly \
    -scriptPath "$ROOT/tools/ghidra" -postScript DecompileAt.java "$OUT_FILE" "$@" >/dev/null
echo "결과: $OUT_FILE"
