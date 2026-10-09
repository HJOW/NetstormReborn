# CanonDecoder 타입별 패턴 선택과 전체 순회

마지막 디컴파일 수행 PC: **HJOW-Athlon**, 2026-10-09. 같은 PC에서 `canontype` 목록을 읽기 전용으로 내보냈다(10.78 10개, CD/추가 10.37 각각 9개). 기존 Ghidra 프로젝트를 재사용했으며 원본 게임/OS 호출을 실행하지 않았다.

[`CanonTypeDecoder`](../../cpppj/src/o/CanonTypeDecoder.h)는 생성자의 실제 타입 전역 비교와 프레임 인자 해석을 기존 `CanonDecoder` 반복자에 연결한다. 앞 단계의 실제 자산 자료와 같은 타입 번호/프레임 코드를 사용한다.

## 선택 규칙과 표

| 현재 전역에 해당하는 타입 | 번호 | 패턴 표 10.78 / CD·10.37 | 레코드 수 |
|---|---:|---|---:|
| puzzlePiece | 107 | `005300f0` / `005151d8` | 68 |
| bridge | 82 | `0052f998` / `00514a80` | 26 |
| island | 94 | `00531410` / `005164f8` | 2 |
| noIsland | 157 | `005314a0` / `00516588` | 1 |
| thunderCannon / rainCannon / windArcher / windBlocker | 131 / 129 / 140 / 142 | `005314e8` / `005165d0` | 공유 1 |
| 그 밖의 타입 | — | 패턴 없음, 기본 또는 명시 프레임 | 한 칸 |

생성자 `00425c20` / CD `0041fcf0`는 앞 네 전역을 위 순서로 비교한 뒤 추가 타입 helper `00425830` / CD `0041fcc0`를 호출한다. 전역 값이 겹치면 먼저 비교한 패턴 표가 우선한다. 네 전투 타입은 모두 같은 표에서 **argument 번호 × 72바이트**로 선택한다. 패치 디컴파일의 `extraout_ECX`는 생성자에 보관한 프레임/패턴 인자가 helper 호출 뒤에도 ECX에 남아 있는 결과다. 원본 역어셈블과 실제 실행으로 확인했다.

패턴 타입은 `explicitFrame`이 참이어도 argument를 패턴 번호로 해석한다. 비패턴 타입에서만 기본 프레임 또는 명시 프레임을 고른다. 회전은 signed 방향 / 2다. 10.78은 정상 회전 범위의 홀수 값과 -1도 받으며 CD는 홀수 방향을 assert한다. C++은 CD 홀수/회전 범위 밖/패턴 번호 범위 밖/비유한 좌표를 예외로 진단한다.

[`cpp_canon_type_tables.py`](../../tools/cpp_canon_type_tables.py)는 누락된 네 레코드 전체를 세 PE에서 읽고 바이트 일치를 확인하여 [`CanonTypePatterns.inc`](../../cpppj/src/o/CanonTypePatterns.inc)를 만든다. 기존 영역/다리 생성기와 포함 파일은 변경하지 않았다. 섬은 1×1의 P1/P3, 받침은 3×3의 F/C/G/B/A/D/I/E/H 각 번호 1, 공유 전투 패턴은 1×1의 L0이다. 첫 정수 10과 사용하지 않는 후행 셀도 그대로 보존한다.

## 연결 범위

`DecodeCanonType`는 현재 타입 전역 8개와 프레임/defaultFrame을 입력으로 받는다. 반환 반복자는 기존 `CanonDecoder`의 셀 해석/회전/빈 칸 진행/첫 프레임 검색/라벨/좌표/끝 상태/Bounds를 사용한다. 표는 정적 배열이므로 반환된 반복자의 패턴 참조가 임시 객체에 매달리지 않는다. 프레임 표의 수명은 호출자가 유지한다.

기존 `RawPriestPlacementGeometry`와 `RawPriestPlacementShape`도 공통 decoder를 사용한다. 두 모듈의 **사제 genus/비패턴 종류 계약은 유지**한다. 실제 비사제 특수 타입의 전체 MayPlace·지역/관계·픽셀 getter 연결을 완료한 것은 아니다. `PriestPlacementAssets`의 현재 전역 번호/프레임 자료는 새 decoder에 그대로 공급된다.

새 읽기 전용 CLI `--inspect-canon-patterns <game-dir> [--cd]`는 실제 자산의 모든 패턴을 네 짝수 방향으로 순회한다. 출력 한 줄은 타입/argument/direction·처음 Bounds·전체 프레임/좌표 비트/라벨/방향·끝 상태/Bounds의 JSON이다. 일반 사제 비패턴 경로도 포함하며 창을 만들지 않는다.

## 독립 대조

[`decomp_canontype_oracle.py`](../../tools/decomp_canontype_oracle.py)는 생성/전체 Advance·셀 해석·실제 프레임 검색·방향 getter·타입/발자국 조회·Bounds/CRT를 **대체 없이 정상 반환까지** 실행한다. 입력은 각 PE의 실제 패턴 바이트와 정상 프레임 표/역순·중복 표다. 선택한 실제 패턴 포인터·각 유효 칸의 프레임/라벨/좌표/방향·끝 상태/범위를 기록한다. 전역 별칭의 우선순위, 모든 패턴 번호/유효 회전, 음수/소수/2^24 좌표, 서로 다른 발자국, 패턴 explicit 모드 무시, 비패턴 기본/명시/빈 경로를 포함한다.

10.78 **2,079개**, CD/추가 10.37 각 **924개**, 총 **3,927개**가 두 x87 정밀도에서 일치한다. 모든 실제 호출의 ESP/보존 레지스터/x87 균형과 허용 몸체/decoder·출력·스택 쓰기, assert/OS 0회, PE/내보내기/도구/표/fixture SHA를 검사한다. 새 [`근거 기록`](../../cpppj/recovery-canontype-evidence.json), [`fixture`](../../cpppj/tests/fixtures/canontype-x86.tsv), [`C++ 검사`](../../cpppj/tests/CanonTypeDecoderTests.cpp)는 이전 감사 자료와 분리되어 있다. 누적 독립 fixture 입력은 **257,196개**다.

프레임이 없는 패턴 검색은 원본 `0049a940` / CD `004442f0`의 assert 보고 경로다. 새 독립 대조에서는 필요한 프레임을 모두 공급하여 assert를 대체하지 않는다. C++의 누락 프레임 -1/건너뛰기 검사는 별도로 두고, 원본 assert 보고를 명시 대체한 과거 `bridge` fixture를 기존 근거로 유지한다. 원본 생성 초기값의 미러 0/진행 간격 1과 프레임 디버그 전역 0 범위이며, 사용자 지정 미러/간격이나 모든 잘못된 입력의 원본 assert 동작을 복원한 근거는 아니다.

[`cpp_canontype_smoke.py`](../../tools/cpp_canontype_smoke.py)는 실제 타입 로딩 순서와 .type를 독립 Python parser로 읽어 현재 코드/defaultFrame/발자국을 원본 전체 decoder에 공급한다. **두 판본 각각 408개 조합·2,028개 유효 칸**의 모든 출력이 C++과 일치하고 두 x87 정밀도도 같다. 원본 PE/타입 자료 SHA와 호출 수는 Git 제외 `extracted/cpp-canontype-assets-report.json`에 기록했다. 실제 자산 검사의 개수는 고정 합성 fixture 누적 개수와 별도로 기록한다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name canontype
python -X utf8 tools/cpp_canon_type_tables.py
python -X utf8 tools/decomp_canontype_oracle.py
python -X utf8 tools/decomp_canontype_oracle.py --verify
cmake --build cpppj/build --config Release
python -X utf8 tools/cpp_canontype_smoke.py
ctest --test-dir cpppj/build -C Release --output-on-failure
```

최종 Release 경고/오류 **0**, CTest 내부 **362개·실패 0**(111.86초), 전체 근거 감사 **51종 모두 통과**, 두 판본 실제 패턴과 기존 사제 픽셀 자산 검사도 통과했다. 검증 로그는 [LEFT_JOBS.md](../../LEFT_JOBS.md)의 이번 단계 기록을 따른다. 다음은 패턴의 일반 자산 픽셀/지형·관계 연결과 일반 사제 Pop·보호막/회복 예약·Carrier 상태 검사다. 건설/경제/전투/승패와 비표면 raw GUI 플레이는 계속 남아 있다.
