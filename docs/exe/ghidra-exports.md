# 기계어 대조 입력(Ghidra 내보내기) 준비

2026-10-07~08 정리. `tools/decomp_*_oracle.py`는 원본 PE의 함수를 Unicorn에서 실행할 때 **Ghidra가 내보낸 함수 몸체 범위**(`functions.tsv`)만 실행을 허용하고, 감사(`--verify`)에서 그 파일과 디컴파일 C(`creation.c`)의 SHA를 확인한다. 이 파일들은 `extracted/`(Git 제외)에 있어 **PC마다 한 번 만들어야 한다.**

## 만드는 방법

```powershell
# 목록이 있는 모든 묶음(약 10분, 읽기 전용 — Ghidra 프로젝트를 바꾸지 않는다)
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -All
# 일부만
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name bridgedecay,finder
# 모든 감사 실행
Get-ChildItem tools/decomp_*_oracle.py | ForEach-Object { python -X utf8 $_.FullName --verify }
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

`derived`·`lifecycle`(unpop)·`pop`·`postpop`·`display`·`graph`·`regiongraph`는 아직 목록 파일이 없고 각 복원 문서의 명령으로 만든다(이 PC에는 이미 있고 감사가 통과한다).

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
