# 기계어 대조 입력(Ghidra 내보내기) 준비

> **마지막 디컴파일 수행 PC: `HJOW-Athlon`, 2026-10-08** (`VM-W11-CODEX`의 변경을 pull한 뒤 `export_functions.ps1 -All`로 다시 만들었고, 이어 새 `prieststate` 목록 11/12/12개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 `priestregen` 목록 10/11/11개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `priestdestroy` 목록 4/4/4개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `priestforcefield` 목록 12/10/10개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `carrierpredestroy` 목록 7/7/7개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `damageablepredestroy` 목록 9/9/9개도 같은 PC에서 읽기 전용으로 내보냈다. 감사 **41개 모두 통과**. 정밀 디컴파일은 이 PC에서 다시 하지 않았다. 그 앞은 `VM-W11-CODEX`, 2026-10-08.) (AGENTS.md 규칙: 디컴파일 소스를 바꾸면 여기와 [LEFT_JOBS.md](../../LEFT_JOBS.md) 머리말의 호스트명을 갱신한다. 현재 PC가 이 호스트가 아니면 아래 절차로 디컴파일/내보내기를 다시 만든 뒤 작업한다). 그 앞은 `HJOW-Athlon`(2026-10-07~08)이다.

2026-10-07~08 정리. `tools/decomp_*_oracle.py`는 원본 PE의 함수를 Unicorn에서 실행할 때 **Ghidra가 내보낸 함수 몸체 범위**(`functions.tsv`)만 실행을 허용하고, 감사(`--verify`)에서 그 파일과 디컴파일 C(`creation.c`)의 SHA를 확인한다. 이 파일들은 `extracted/`(Git 제외)에 있어 **PC마다 한 번 만들어야 한다.**

## 만드는 방법

```powershell
# 목록이 있는 모든 묶음(약 10분, 읽기 전용 — Ghidra 프로젝트를 바꾸지 않는다)
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -All
# 일부만
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name bridgedecay,finder
# 모든 감사 실행
Get-ChildItem tools/decomp_*_oracle.py | Where-Object { Select-String -LiteralPath $_.FullName -SimpleMatch "add_argument('--verify'", 'add_argument("--verify"' -Quiet } | ForEach-Object { python -X utf8 $_.FullName --verify }
```

[export_functions.ps1](../../tools/ghidra/export_functions.ps1)은 `tools/ghidra/<이름>-functions.json`의 주소 목록을 판본마다 `extracted/<이름>/<판본>/`에 내보낸다. **목록의 순서가 파일 안의 순서이고 SHA가 그 순서에 의존한다.** 선행 조건은 세 판본의 Ghidra 프로젝트(`tools/ghidra/run_decomp.ps1`)다. 내보내기는 결정적이다 — 같은 목록을 두 번 내보낸 86개 파일이 바이트 단위로 같았고, 다른 PC에서 기록한 SHA와도 일치한다.

| 이름 | 쓰는 도구 | 함수 수(패치 / CD / 10.37) | 목록의 출처 |
|---|---|---|---|
| `sid` | sid·creation 계열·destroy 계열 | 7 / 6 / — | 문서의 명령 (`ExportSid.java`, `sid.c`를 만든다) |
| `creation` | creation·derived 등 | 8 / 13 / — | 문서의 명령 |
| `graphremove` | graphremove·graphlookup·destroy 계열 | 95+2 / 90+2+1 / 90+2+1 | 처음부터 있던 목록(base·geometry·lookup 묶음) |
| `graphrebuild` | graphrebuild·graphrecovery | 5 / 7 / 7 | 문서의 명령 |
| `graphrecovery` | graphrecovery | 6 / 6 / 6 | 문서의 명령 |
| `bridgedecay` | bridgedecay·bridgeeffects·rawfinder | 48 / 34 / 34 | **실행 추적으로 다시 정함**(아래) |
| `bridgeeffects` | bridgeeffects·rawfinder·destroy 계열 | 12 / 7 / 7 | **기록 SHA에 맞춘 순서 복원**(아래) |
| `finder` | rawfinder·destroy 계열 | 8 / 6 / 6 | **기록 SHA에 맞춘 순서 복원**(아래) |
| `destroy`·`destroylifecycle`·`destroygraph` | 같은 이름의 도구 | 9/6/6 · 14/20/20 · 15/17/17 | 처음부터 있던 목록 |
| `reward`·`process` | 같은 이름의 도구 | 27/24/24 · 73/71/71 | 처음부터 있던 목록 |
| `bridgeevent`·`owner` | 같은 이름의 도구 | 8/6/6 · 5/4/4 | 처음부터 있던 목록 |
| `bridgepostpop` | bridgepostpop | 1 / 1 / 1 | 다리 vtable +0x20의 실제 함수 |
| `bridgeconnect` | bridgeconnect·islandlifecycle | 10 / 10 / 10 | 2026-10-08 추가([다리/섬 연결](cpp-bridgeconnect-reconstruction.md)) |
| `setframe` | setframe | 3 / 3 / 3 | 2026-10-08 추가([프레임 지정](cpp-setframe-reconstruction.md)) |
| `pathanimation` | pathanimation | 3 / 3 / 3 | 2026-10-08 추가([방향 조회·도착 프레임 접두](cpp-path-animation-reconstruction.md)), `VM-W11-CODEX`에서 내보냄 |
| `priestowner` | priestowner·owner | 2 / 2 / 2 | 2026-10-08 추가([사제 소유자 재정의](cpp-priest-owner-reconstruction.md)), 공통 owner 내보내기 재사용·`VM-W11-CODEX`에서 내보냄 |
| `priestpostpop` | priestpostpop·owner | 5 / 6 / 6 | 2026-10-08 추가([사제 목록·회복 예약 prefix](cpp-priest-postpop-reconstruction.md)), carrier 진입에서 중단·`VM-W11-CODEX`에서 내보냄 |
| `carrierpostpop` | carrierpostpop·owner | 5 / 5 / 5 | 2026-10-08 추가([Carrier/Damageable 직접 호출](cpp-carrier-postpop-reconstruction.md)), 외부 세 효과 대체·전체 몸체 반환·`VM-W11-CODEX`에서 내보냄 |
| `prieststate` | prieststate·owner | 11 / 12 / 12 | 2026-10-08 추가([사제 HP/지면 상태·HP setter](cpp-priest-state-reconstruction.md)), 조회 대체 없음·setter 공간 두 효과 대체·전체 몸체 반환·`HJOW-Athlon`에서 내보냄 |
| `priestregen` | priestregen·prieststate·owner | 10 / 11 / 11 | 2026-10-08 추가([사제 회복 0x25a](cpp-priest-regen-reconstruction.md)), 회복/HP/중립 조건/난수 실제 실행·외부 공간/표시/추적 효과 대체·`HJOW-Athlon`에서 내보냄 |
| `priestdestroy` | priestdestroy·owner | 4 / 4 / 4 | 2026-10-08 추가([사제 삭제 준비](cpp-priest-destroy-reconstruction.md)), 목록 압축/범위 검사 실제 실행·조회/가상 삭제/Carrier 대체·전체 본문 반환·`HJOW-Athlon`에서 내보냄 |
| `priestforcefield` | priestforcefield·owner | 12 / 10 / 10 | 2026-10-08 추가([사제 보호막 실제 조회](cpp-priest-forcefield-reconstruction.md)), 좌표/일반 finder/필터 대체 없이 실행·Pre의 가상 삭제/Carrier만 대체·`HJOW-Athlon`에서 내보냄 |
| `carrierpredestroy` | carrierpredestroy·owner | 7 / 7 / 7 | 2026-10-08 추가([Carrier 삭제 준비·실제 contained 조회](cpp-carrier-predestroy-reconstruction.md)), 직접 조회 대체 0·Carrier의 Damageable/전역 후처리만 대체·전체 본문 반환·`HJOW-Athlon`에서 내보냄 |
| `damageablepredestroy` | damageablepredestroy·owner | 9 / 9 / 9 | 2026-10-08 추가([Damageable 효과/소리 접두·실제 종속 순회](cpp-damageable-predestroy-reconstruction.md)), spot/x87/contained 실제 실행·하위 함수/권한 해방 중간 구간을 명시 대체·`HJOW-Athlon`에서 내보냄 |
| `islandlifecycle` | islandlifecycle | 4 / 2 / 2 | 2026-10-08 추가([섬 삭제 훅](cpp-islandlifecycle-reconstruction.md)) |
| `islandpostpop` | islandpostpop | 5 / 4 / 4 | 2026-10-08 추가([noIsland 최초 등록·받침 소유자](cpp-islandpostpop-reconstruction.md)), `VM-W11-CODEX`에서 내보냄 |
| `regiongraph` | regiongraph | 13+3 / 14+1 / 14+1 | **증거 JSON의 `function_ranges` 순서에서 복원**(아래 "남은 문제") |
| `pop` | pop·display·postpop·rawgraph·regiongraph | 22(+보조 2) / 18 / — | 문서의 명령. 패치판 보조 둘은 `extracted/pop/helpers-originals`(목록의 `aliases`) |
| `display` | display 계열 | 10 / 7 / — | 증거 JSON의 누적 `function_ranges`에서 앞 단계 몫을 뺀 나머지 |
| `postpop` | postpop 계열 | 10 / 13 / — | 같은 방법 |
| `graph` | graph·rawgraph 계열 | 13 / 16 / — | 증거 JSON의 `function_ranges` 그대로 |
| `lifecycle` | unpop | 22 / 17 / — | 문서의 명령(`decomp_unpop_oracle.py`가 `extracted/lifecycle`에서 읽는다) |

2026-10-08 후속의 `neighbor`는 새 내보내기 없이 `bridgeevent`·`graphremove`/`geometry`를 재사용한다. `bridgepostpop`의 접두 대조 240개와 함께 [첫 연결 이웃/끝 칸 통합 근거](cpp-neighbor-reconstruction.md)에 정리했다. 두 새 도구를 포함해 **감사 27개가 모두 통과**했다. `--verify` 옵션이 없는 옛 `config`·`graphics`·`options` 생성기는 위 감사 명령에서 제외한다.

`derived`만 아직 목록 파일이 없다(주소를 스크립트로 계산하는 명령이 [그 문서](cpp-derived-reconstruction.md)에 있다). `pop`·`display`·`postpop`·`graph`·`lifecycle`의 `--verify`는 저장소 안의 파일만 확인하므로 내보내기가 없는 PC에서도 통과하지만, **기대값을 다시 생성하려면** 그 내보내기가 있어야 한다. 2026-10-08에 이 다섯 묶음의 목록을 저장소에 넣었다 — 새로 내보낸 `functions.tsv`의 (진입 주소, 범위)가 **두 판본 모두 각 증거 JSON에 기록된 것과 순서까지 같음**을 확인했다(`rawgraph`는 여기에 더해 정밀 디컴파일의 `extracted/refined/<판본>/functions.tsv`를 읽는다 — `tools/ghidra/refine_all.ps1`, 이 PC에서 약 20분).

## 2026-10-08 `VM-W11-CODEX`에서 처음 준비한 결과

새로 받은 PC에서 위 절차를 그대로 따랐다: `run_decomp.ps1`로 만든 세 판본의 기본 Ghidra 프로젝트(10.78 4,506개·CD/10.37 각 3,711개 함수) → `export_functions.ps1 -All` → CRLF로 남아 있던 도구/증거/스크립트 사본 21개를 지우고 `git checkout`으로 다시 받음 → 감사. 처음에 **27개 가운데 26개가 통과**했고, 이 날 더한 세 도구(`bridgeconnect`·`setframe`·`islandlifecycle`)까지 **30개 가운데 29개가 통과**한다. 내보내기는 이 PC에서도 다른 PC가 기록한 SHA와 일치했다.

알아 둘 것:

- Python 의존성은 `extracted/oracle-python`에 격리 설치한다. 요구사항 파일에 한글 주석이 있어 한국어 Windows에서는 `PYTHONUTF8=1`(또는 `python -X utf8 -m pip ...`)이 필요하다.
- **Ghidra 헤드리스는 같은 설정 폴더(`extracted/ghidra-settings`)로 동시에 두 개를 돌릴 수 없다.** 둘째 실행이 번들 캐시 잠금(`Unable to create bundle cache lock file`)으로 스크립트를 싣지 못하고 멈춘다. `refine_all.ps1`이 도는 동안에는 내보내기를 하지 않는다.
- `powershell -File export_functions.ps1 -Name a,b`처럼 부르면 쉼표 목록이 한 문자열로 넘어와 실패했다. 스크립트가 쉼표를 직접 나누도록 고쳤다.

### 해결한 문제: `regiongraph` (2026-10-08, VM-W11-CODEX)

후속 세션에서 위 준비된 내보내기로 `decomp_regiongraph_oracle.py`를 실제로 재실행했다. **1,536개 fixture는 파일 전체가 바이트 단위로 그대로**이며 `--verify`도 통과한다. 증거 JSON의 디컴파일 C SHA와 누적 함수 경계 메타데이터(CD/10.37 helper 구분)를 현재 PC의 판독에 맞췄다. 새 noIsland 도구를 포함해 감사 **31개 모두 통과**한다. 아래 문장은 재생성 전의 원인과 절차 기록이다.

`regiongraph`는 목록이 없어 이 PC에서 감사가 실패했다. 증거 JSON의 `function_ranges`가 내보낸 순서 그대로 남아 있어 [목록 파일](../../tools/ghidra/regiongraph-functions.json)로 복원했다(CD/10.37의 InsertCD 비교 함수 `00433bf0`의 위치는 `functions.tsv` SHA로 확정 — 맨 끝). 결과:

- `functions.tsv`(실행 허용 범위)는 **세 판본·두 묶음 모두 기록 SHA와 일치**한다.
- `creation.c`(디컴파일 텍스트)는 **10.37만 일치**하고 10.78·CD는 다르다. 기록은 다른 PC의 예전 Ghidra 프로젝트 상태에서 만든 것으로 보인다(앞서 `sid`·`creation`에서 같은 현상이 있었다). 텍스트만 다르고 실행 범위는 같다.

당시 이 PC의 `decomp_regiongraph_oracle.py --verify`는 `creation.c` SHA에서 멈췄다. 앞 단계의 `pop`·`display`·`postpop`·`graph` 내보내기(위 목록)와 `extracted/refined/`가 준비돼 있었고 후속 세션에서 아래 절차로 재생성해 해결했다.

```powershell
python -X utf8 tools/decomp_regiongraph_oracle.py            # 기대값 재생성(소요 시간 미측정)
git status --short cpppj/tests/fixtures/regiongraph-x86.tsv   # 바뀌지 않아야 한다(바뀌면 증거를 되돌리고 원인을 먼저 본다)
python -X utf8 tools/decomp_regiongraph_oracle.py --verify
```

fixture가 바이트 단위로 그대로이고 증거 JSON의 SHA 항목만 바뀌면 30개 감사가 모두 통과한다. 같은 방법으로 `pop`·`display`·`postpop`·`graph`·`rawgraph` 도구의 재생성도 이 PC에서 확인할 수 있다(하지 않았다).

## 2026-10-07~08에 고친 것 (`HJOW-Athlon`)

처음에는 이 PC에서 24개 감사 가운데 11개가 실패했다. 원인은 세 가지였다.

1. **목록이 저장소에 없던 묶음.** `bridgedecay`·`bridgeeffects`·`finder`·`graphrebuild`·`graphrecovery`·`sid`·`creation`의 주소 목록이 문서의 명령이나 다른 PC의 로그에만 있었다. 모두 목록 파일로 만들었다.
   - `bridgeeffects`·`finder`: 진입점에서 직접 호출을 따라간 폐포(`finder`는 여기에 간접 호출되는 기본 true 필터 하나를 더함)가 문서의 함수 수와 같았다. **저장 순서는 기록된 `functions.tsv`의 SHA와 맞는 순열을 찾아 확정했다**(CD 7개·6개는 전수, 패치 12개는 앞 5개를 도구의 진입점 순서로 두고 나머지 7개 전수). 다시 내보낸 `creation.c`의 SHA까지 기록과 같다.
   - `bridgedecay`: 처음 기록(패치 51·CD 35개)의 목록은 어디에도 남아 있지 않았고 SHA에 맞는 구성을 찾지 못했다. 폐포(68/49개)는 실행되지 않는 함수까지 포함한다. 그래서 **기대값 생성 전체를 실제로 돌려 실행된 함수만 추렸다**(대체 진입점 함수 제외, 진입점 먼저 + 첫 실행 순서): 패치 48·CD/10.37 각 34개. 이 목록만으로 다시 생성한 `bridgedecay-x86.tsv`는 저장소의 것과 **바이트 단위로 같다**(입력 14,524개). 처음 기록과 3개·1개가 다른 이유는 확인하지 못했다(실행되지 않는 함수가 들어 있었을 가능성).
2. **줄바꿈에 좌우되던 SHA.** `decomp_bridgeeffects_oracle.py`·`decomp_rawfinder_oracle.py`가 줄바꿈을 지정하지 않고 fixture/증거를 써서 Windows에서 CRLF 파일의 SHA를 기록했고(저장소에는 LF로 들어간다), `decomp_rawgraph_oracle.py`는 `eol` 고정이 없어 CRLF 작업 사본의 SHA가 다른 증거들에 기록돼 있었다. 두 도구가 LF로 쓰도록 고치고, `.gitattributes`에 `tools/decomp_*.py`·`tools/ghidra/*.java`·`tools/ghidra/*-functions.json`·`cpppj/recovery-*.json`을 `eol=lf`로, `*.ps1`을 `eol=crlf`로 고정했다.
3. **이 PC의 오래된 사본.** `extracted/sid/originalCD`·`extracted/creation/originalCD`가 예전 Ghidra 프로젝트 상태에서 만든 것이었다(잘못된 주소 한 줄, 함수 하나 더). 새로 내보낸 파일은 다른 PC가 기록한 SHA와 일치한다.

그 뒤 영향을 받는 11개 도구(`bridgeevent`·`bridgedecay`·`bridgeeffects`·`rawfinder`·`graphremove`·`graphlookup`·`graphrebuild`·`graphrecovery`·`destroy`·`destroylifecycle`·`destroygraph`)를 이 PC에서 **다시 실행해 증거를 다시 기록했다. 모든 fixture는 바이트 단위로 그대로였다**(증거 JSON의 SHA 항목만 바뀜). 지금은 새로 더한 `owner`까지 25개 감사가 모두 통과한다.

## 다른 PC에서 할 일

- 이 변경을 받은 뒤 `export_functions.ps1 -All`로 내보내기를 다시 만든다. 특히 `bridgedecay`는 목록이 바뀌었으므로(51/35 → 48/34) 예전 내보내기로는 `--verify`가 실패한다.
- `.gitattributes`가 바뀌었으므로 CRLF로 체크아웃돼 있던 도구(`tools/decomp_rawgraph_oracle.py` 등)는 작업 사본을 지운 뒤 `git checkout -- <파일>`로 다시 받아야 LF가 되어 SHA가 맞는다.
- `extracted/creation/<판본>/sid.c`처럼 목록에 없는 파일이 남아 있어도 감사에는 영향이 없다.
