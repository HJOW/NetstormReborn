# 경로 단축 작업 결과 및 최종 검사 인수인계

2026-10-14, 현재 PC **VM-W11-Codex**. 경로 정리 전 기준은 `9d7acad`, 중단 상태는 `364d4a4`다. 사용자가 경로 정리 재개와 두 프로젝트의 빌드를 요청했고, 이후 인수인계와 로컬 커밋을 요청했다. **마지막 지시로 최종 검사는 생략한다.** 아래 완료 결과와 미검증 항목을 구분해 다음 작업에서 이어간다.

## 완료한 작업

- 중단된 이름 이동을 다시 시작하지 않고 기준 Git 객체와 현재 파일을 대조해 복구했다. [path-map.json](path-map.json)에 11,451개 기준 추적 파일, 7,718건의 이전/현재 이름 대응, 원래 Git blob 및 SHA, 적용 전 SHA, 계산된 적용 후 SHA를 기록했다. 최초 SHA 계산은 37단계에서 수렴했다.
- 보고서·도구·문서·CMake 및 보존 이력의 경로 참조를 교정했다. LF/CRLF와 같은 원문을 복제한 이력을 구분해 JSON의 파일별 SHA와 pin 바이트 수를 추가로 교정했다. 기존 원본 PE와 실제 게임 입력/기대값을 새로 생성하지 않았다.
- `path_fix.py`에 읽기 전용 기본 계획, 바이너리 대조, 계산 후 변경 감지, 짧은 복구 캐시 및 `--resume`을 추가했다. 실행 중 계속 변하는 `extracted/path-*` 로그는 고정 원문 장부에서 제외했다.
- `path_audit.py`에 기존 이름 예외의 상한과 명시 신규 후보의 40자 제한을 적용했다. CI의 경로 검사 연결은 유지한다. 새 파일명 40자, 새 디렉토리명 24자, 상대 경로 160자, Windows 절대 경로 200자 기준은 [경로 기준](path-rules.md)을 따른다.
- `path_compat.py`는 기존 도구 원문을 다시 편집하지 않고 과거 Git 이름과 이관된 당시 보존본을 연결한다. 호출자의 `HISTORY/git`를 우선한다. 사용 예: `python -X utf8 tools/path_compat.py tools/analyze_genus_at_public_lineage.py --verify`.
- `path_refs.py`에 파일명과 이전 SHA가 함께 맞는 JSON 참조 교정 및 같은 원문 보존본의 일관성 검사를 구현했다. 기본은 읽기 전용, `--apply`는 명시 교정이다. 알 수 없는 옛 SHA를 모두 현재 값으로 치환하는 방식은 수렴하지 않아 적용하지 않았다.
- `path_setup.py`는 기존 `.git/info/attributes`를 유지하면서 이력 전체의 `-text` 보존 규칙을 설치한다. 이력 내부의 과거 `.gitattributes`가 루트 규칙보다 우선하는 문제를 피한다. 이 PC에는 설치했다. 다른 PC에서는 분석/스테이징 전에 `python -X utf8 tools/path_setup.py`를 실행한다. 체크아웃에서 이력의 줄바꿈만 변한 경우 `--restore`는 HEAD의 SHA와 내용 대조가 모두 맞을 때만 복원한다.
- `assets/README.md`의 C# 테스트 자료 링크를 실제 `dotnetpj/tests/...` 경로로 수정했다. LEFT의 삭제된 임시 coverage 패치 링크는 당시 파일명 텍스트로 남겼다.

## 실제 수행한 검증

| 항목 | 결과 |
| --- | --- |
| dotnetpj 솔루션 Release 빌드 | 성공, 경고 0·오류 0, 35.31초 |
| cpppj CMake 구성 및 Release 빌드 | 성공, 컴파일 경고/오류 없음 |
| cpppj CTest | 래퍼 1개 통과, 내부 **1,088개·실패 체크 0**, 194.72초 |
| 경로 검사 | 당시 11,461개, 최장 **180자**, 위반 0 |
| 신규 후보 검사 | 짧은 이름 허용, 42자 이름 거부 |
| Git 긴 경로 옵션 비활성화 | `git -c core.longpaths=false status` 성공 |
| 공백 검사 | 당시 `git diff --check` 통과 |
| 참조 조사 | 당시 11,936개 대상의 Python 구문 오류 0·파일 누락 0 |
| GenusAt Git 호환 소비자 감사 | 선언의 정확 역이동·기존 C++ 몸체/Git/작업 원문·기존 4,412 입력의 불변 검사 통과, 새 원본 관찰 0 |

위 검증은 **최종 문서·SHA 마감 후 다시 실행한 최종 검사 결과가 아니다.** dotnetpj 단위 테스트, 실제 원본/복사본 게임, 클론 GUI 및 원격 CI는 실행하지 않았다. 자료는 `extracted/path-{cpp-build,cpp-tests,cpp-all,dotnet-build,refs,context*,close}.log`와 `path-check.json` 등에 있다. `extracted/` 자료는 Git 제외다.

## 마지막 디컴파일 PC

AGENTS의 다른 PC 재디컴파일 규칙에 따라 **VM-W11-Codex, 2026-10-14**에서 두 판본의 정적 전체 디컴파일을 수행했다.

- 10.78: `extracted/decomp/Netstorm.c`, 성공 4,506함수·실패 0.
- 10.72: `extracted/originalCD/decomp/NETSTORM.c`, 성공 3,711함수·실패 0.
- 10.78은 출력과 함수 덤프 생성 및 저장 성공 로그 뒤 프로젝트 종료 시 `versioned/~index.dat` 인덱스 백업/flush 오류가 남았다. 출력 생성과 Ghidra 프로젝트의 완전한 저장 상태를 동일시하지 않는다. 정밀 프로젝트·개별 함수 내보내기는 이번에 재생성하지 않았다.
- 보호 PE SHA는 작업 전후 일치했다. 10.78은 `A305414CFE0B94494BB32FEF2B359E920E9DABA225E0821B76AEA1F9F8E7D236`, CD판은 `613500A394B3062E914604EB7D09896D678192BE24545D0E575082A87C851233`다.

## 다음 작업: 사용자가 생략하도록 요청한 최종 검사

1. AGENTS 및 양 LEFT를 읽고 호스트와 작업 트리를 확인한다. 다른 PC에서 cpppj/디컴파일 분석을 재개할 때는 AGENTS의 재디컴파일 규칙을 따른다. 이번 커밋은 `20261014-09`로 계획했다. 커밋 SHA는 `git log`에서 확인한다.
2. **최초 이관을 재실행하지 않는다.** `path-map.json`의 `pending`은 최초 적용 상태이며 최종 감사 완료를 의미하지 않는다. 문서와 새 도구 및 마지막 보고서 마감으로 대응표의 일부 `after`가 최신 파일과 다를 수 있다. 이를 손상으로 단정하거나 예전 캐시를 덮어쓰지 않는다. 기준 Git blob·`before`·`current_before`·`migration_after`·`sha_history`와 현재 커밋을 함께 사용해 후속 편집의 SHA를 먼저 연결한다.
3. `path-close.py` 첫 직접 덮어쓰기는 보존 GenusAt 보고서에서 OSError 22를 받았다. 기존 파일은 읽기 가능했고 10,300바이트였다. 같은 폴더의 임시 파일 원자 교체로 재개해 성공했고 GenusAt 전체 검증도 통과했다. 첫 시도의 일부 쓰기는 대응표 저장보다 먼저 완료됐을 수 있으므로 **실제 파일과 대응표의 최종 일치 검사**가 필요하다. 원문 입력·기대값과 보호 원본은 교정 대상이 아니다.
4. `python -X utf8 tools/path_audit.py`, `tools/path_refs.py` 기본 검사 및 최종 전체 SHA 검사를 수행한다. `path_move.py --verify`는 후속 편집 SHA가 장부에 반영된 뒤 실행한다. 현재 `path_refs.py`의 모든 과거 스키마/바이트 수 형식 처리까지 완전하다고 주장하지 않는다. 코드 검토와 소비자별 검사를 함께 수행한다.
5. 과거 보고서의 현재 파일 비교에서 뜨는 SHA 불일치는 원래의 이전 구현/역변환 경계, 보존본의 줄바꿈, 또는 이관 참조 누락인지 구분한다. 광범위한 무조건 치환은 피한다. 당시 정적 조사에서는 일부 과거 필드와 로컬 자료 참조가 남았다. `path-check.json`은 정밀 소비자 실행을 대신하지 않는다.
6. 이전 단위15의 엄격한 전체 감사는 이 PC에 없는 `cpppj/hist/campaign-run-092de/cpppj/build/bin/Release/{NetstormCpp,netstorm_tests}.exe`에서 중단됐다. 기존 빌드 바이너리 SHA를 새 PC의 새 빌드 SHA로 바꾸거나 빈 파일로 채우지 않는다. 새 빌드는 별도 검증 근거다. 로컬 원본 함수 export/과거 캡처에도 없는 자료가 있으며 문서의 과거 녹화 프레임 9개는 이 PC에 없다.
7. 로컬 이력 `-text` 속성을 설치한 뒤 스테이징된 이력 blob의 전체 바이트 보존을 확인한다. **이번에는 사용자의 최종 검사 생략 지시에 따라 스테이징 blob 전체 대조도 수행하지 않는다.** Git 공백·이름 충돌·남은 옛 이름/동적 import/문서 링크·CI 경로 검사까지 마감하고 필요할 때만 두 빌드를 다시 실행한다. AGENTS와 보호 원본은 수정하지 않으며 원격 push는 하지 않는다.

실제 캠페인 월드 설치, Player/Asset/Path/Graph 공유 연결, 게임 플레이 동일성 및 dotnetpj 튜토리얼·시계 후속은 기존 LEFT의 미완료 항목이다. 이번 작업은 경로/메타데이터 정리와 빌드 검증이며 게임 구현 목표의 완료가 아니다.
