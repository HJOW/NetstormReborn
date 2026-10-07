# cpppj 객체 부착 프로세스(ProcessForm)·Kernel·Regular 지연/주기 이벤트 복원

2026-10-07. **1차 기준은 10.78**이다. [공통 삭제](cpp-destroy-reconstruction.md)가 호출자 계약으로 남겨 둔 "종속 form의 가상 release/destroy" 가운데 **ProcessForm**을 복원하고, 그 위에서 도는 지연/주기 이벤트(RegularProcess)를 연결했다. 다리의 지연 낙하 예약(이벤트 `0x2692`)도 이 계층으로 등록/조회한다. 호스트 **DESKTOP-HJOW에서 원본/복사본 게임·업데이터/설치 도구·클론 창을 실행하지 않았다.** CD/추가 10.37은 비교 판본이다.

소스: [SquidProcess](../../cpppj/src/o/SquidProcess.cpp), [Kernel](../../cpppj/src/o/Kernel.cpp), [RawSquidDestroy](../../cpppj/src/o/RawSquidDestroy.cpp). 검사: [ProcessTests.cpp](../../cpppj/tests/ProcessTests.cpp). 독립 원본 출력: [process-x86.tsv](../../cpppj/tests/fixtures/process-x86.tsv), [근거 기록](../../cpppj/recovery-process-evidence.json), [생성/감사 도구](../../tools/decomp_process_oracle.py).

## 원본 주소와 읽기 전용 내보내기

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| BaseProcess 생성자(form 생성/부착) | `0041bd20` | `0048f590` |
| BaseProcess 종료 요청(form destroy) | `0041bf10` | `0048f7b0` |
| Kernel 등록 / 제거 / 프레임 실행 / 조회 | `00471950` / `004719e0` / `00471a30` / `00471aa0` | `004ed500` / `004ed590` / `004ed5d0` / `004ed640` |
| RegularProcess 생성자 / 실행 / 소멸자 | `00496e80` / `00496cb0` / `00468230` | `0048efd0` / `0048f0e0` / `0048f440` |
| base 이벤트 처리기(vtable +0x5c) | `00496e70` | `0048f430` |
| ProcessForm 생성자 / 가상 표 | `004ae3d0` / `00512098` | `004af5c0` / `00505990` |
| form 부착(vtable +0x74) → 부모 안에 넣기 → Pop | `004b1440` → `004ac800` → `004ac560` | `004af380` → `004ac2e0` → `004abfb0` |
| form Unpop / 종속 체인에서 빼기 | `004ae140` / `004ac6b0` | `004af270` / `004ac160` |
| form preDestroy / postDestroy / 부모 삭제 통지 | `004ae240` / `004ac1b0` / `004ae2b0` | `004af450` / `004aba90` / `004af4c0` |
| 이벤트 조회 / 마스크 조회 | `004afb40` / `004afbe0` | `004ac890` / `004ac9c0` |
| 종속 탐색 Begin / Next | `004b22d0` / `004b1b80` | `004eb960` / `004eb990` |
| 다리 지연 낙하 예약 / 예약 여부 | `00421530` / `00421fe0` | `004490b0` / `00449170` |

[함수 목록](../../tools/ghidra/process-functions.json)으로 읽기 전용 Ghidra를 실행했다. 패치 **73개**, CD/추가 10.37 각각 **71개**를 `extracted/process/<판본>/creation.c`·`functions.tsv`에 내보냈다(SID 초기화/할당/반납·Create·공통 destroy처럼 앞 단계에서 이미 복원한 함수의 몸체 범위도 실행 허용 목록으로 다시 포함했다). 기존 독립 산출물은 보존했다.

## 구조

- **프로세스 = Kernel 슬롯의 객체 + 부모 안의 form SID.** BaseProcess 생성자는 Kernel에 자신을 등록하고(앞에서부터 첫 빈 슬롯), `ProcessForm` 타입(2)의 SID를 만든 뒤 타입 바이트를 **프로세스 타입 번호**(10~62)로 바꾸고 `+0x12`에 Kernel 번호를 적는다. 그 form을 부착 대상 객체의 **종속 체인 머리**에 넣는다.
- **form의 raw 배치:** `+4` 다음, `+6` 자기 종속 머리, `+0xa` 프로세스 타입, `+0xb` 상태, `+0xe` 이전 항목, `+0x10` 부모, `+0x12` Kernel 번호(DWORD). 체인은 양방향이다. contained **자산**은 같은 가상 함수가 부모를 `+0xe`, 이전 항목을 `+0x10`에 둔다(정적 확인; 이번 기계어 입력은 form만 체인에 넣는다).
- **종료:** 프로세스를 지우는 길은 form의 공통 destroy(`004af780`)뿐이다. form의 preDestroy가 프로세스의 가상 통지(+0xc)를 부르고 Kernel에서 제거(스칼라 삭제 소멸자)한다. Unpop은 void를 켜고, 부모가 free가 아니고 dead도 아닐 때만 체인에서 뺀다. 부모가 지워지는 중이면 체인과 contained 비트를 그대로 둔 채 반납된다.
- **부모 삭제:** 공통 destroy의 종속 루프가 각 form에 "부모 삭제 통지(+0x1c)"를 보낸 뒤 `flags | 0x40`으로 destroy한다. 그래서 부모가 사라지면 붙어 있던 프로세스가 모두 정리된다.
- **RegularProcess(타입 46):** `+0x10` 다음 실행 시각(double), `+0x18` 이벤트 번호, `+0x1c` payload, `+0x20` 호출 횟수. 매 Kernel 프레임에 부모가 free/dead/void가 아니고 시각이 됐으면 **부모의 가상 이벤트 처리기(+0x5c)** 를 `(이벤트, 횟수, payload)`로 부른다. 반환값이 **양수면 그 값이 다음 payload이자 지연**이 되고 횟수가 늘며, **0이면 종료**, **음수면 그대로** 둔다(다음 프레임에 다시 호출).
- 생성자는 시각을 **현재 시각**으로 두므로 첫 호출은 다음 프레임이다. base 처리기는 payload를 그대로 돌려주므로, 재정의가 없으면 payload 주기로 반복한다. 호출자가 넘기는 payload(예: 0.5, 8.0)가 사실상 첫 지연이다.
- geyser(타입 122)에 붙은 Regular는 0.5초보다 먼 예약을 현재 시각으로 당긴다.
- **다리 지연 낙하:** `00421530`은 좌표를 `(ftol(x) & 255) | (ftol(y) << 8)`로 포장한 float을 payload로 이벤트 `0x2692`를 예약하고, `00421fe0`은 그 이벤트의 존재 여부를 본다.

## 판본 차이

| 항목 | 10.78 | CD / 10.37 |
|---|---|---|
| free/dead 부모에 부착 | 조용히 건너뜀(등록·form 생성 없음) | assert 보고 뒤 그대로 진행 |
| abstract 부모의 Regular | genus가 factory/vortex/dais가 아니면 실행하지 않음 | genus를 읽기만 하고 실행함 |
| 처리기가 NaN 반환 | 재예약으로 취급 | 아무것도 하지 않음 |
| Kernel 슬롯 | 1~39999 | 1~3999 |
| Kernel 등록의 부수 효과 | 타입 순환 값(`0059af74`)을 하나 전진 | 없음 |
| 프로세스 타입 번호 끝 | 63 | 62 |
| 종료 요청의 client 번호 경계 | 5~14999 | 5~5999 |

## 독립 원본 대조와 검증 한계

| 입력 | 시나리오 | 연산 |
|---|---:|---:|
| 10.78 패치 | 120 | 1,974 |
| CD 10.72 배포본 | 120 | 2,035 |
| 추가 10.37 | 120 | 2,035 |
| 합계 | **360** | **6,044** |

시나리오는 서버/권한 플래그·시작 시각·타입 순환 값과 2~4개의 합성 부모 객체로 시작해, 그때그때의 **실제 상태**를 보고 고른 연산 10~17개를 이어 실행한다. 연산은 부모 번호 할당(P), Regular 생성(A), 임의 flags의 BaseProcess 생성(B), 시각 변경(T), Kernel 프레임 실행(R, 처리기 반환값 목록 포함), 종료 요청(K), 이벤트 조회(F)/마스크 조회(M), 부모 삭제(D), 부모의 void/abstract 비트 뒤집기(S)다. 모든 시나리오를 x87 53/64비트로 두 번 실행해 관찰이 같은 것만 저장했고 **제외는 0건**이다.

연산마다 비교하는 관찰값은 **연산 반환값·freeCount·예측 커서·client/server 머리와 꼬리·32768슬롯 풀 전체 Adler-32·삭제 기록 Adler-32·pre/post/pop 깊이·타입 순환 값·Kernel 슬롯 배치(앞 95개)·살아 있는 프로세스의 form/부모/시각(double 비트)/이벤트/payload/횟수·처리기와 부모 pre/post·삭제 전파의 호출 기록·해제된 프로세스**다. 풀 전체 체크섬이 같으므로 form 슬롯의 모든 바이트와 부모의 종속 머리가 원본과 같다.

패치 기준 도달: 부착 성공 369회·free/dead 부모라 건너뜀 8회, 처리기 호출 165회, 프로세스 해제 119회(처리기 0 반환 41개 프레임·종료 요청 50회·프로세스가 붙은 부모 삭제 17회 등), 삭제 전파 10회. CD/10.37은 부착 410회, 처리기 호출 220회, 해제 135회다. 정밀도별 실제 내부 도달은 패치 BaseProcess 생성자 377·Kernel 등록 369·Regular 생성자 299·Regular 실행 526·Kernel 프레임 463·종료 요청 95·공통 destroy 170·form preDestroy 119·체인에서 빼기 118·이벤트 조회 164/마스크 91이고 CD/10.37은 각각 410·410·330·576·481·98·201·135·135·168/77이다([기록](../../cpppj/recovery-process-evidence.json)의 `native_hits`는 두 정밀도를 합한 값이다). 원본 assert 도달 0·허용 밖 실행/쓰기 0·정상 반환/ESP/x87 제어 워드/TOP·FS:[0] 복구를 확인했다.

**명시적 대체**는 부모 객체의 가상 preDestroy/postDestroy(깊이 감소만)와 이벤트 처리기(vtable +0x5c, 입력 목록의 값을 x87로 반환), 프로세스 객체의 new/free, 삭제 로그, 삭제 전파(인자 기록)뿐이다. BaseProcess/Regular 생성자·Kernel·form 생성/타입/부착/Pop·Unpop/체인·form pre/post·공통 destroy·SID 할당/반납/삭제 기록·종속 탐색은 원본 명령이다. 부모는 실제 `Allocate`로 번호를 받고 합성 가상 표/타입/상태를 쓴 객체이며, **void 상태에서만 삭제**한다(자산의 실제 Unpop은 앞 단계의 범위다).

**이번 입력이 약하게만 덮는 곳(인수인계):** 이벤트 조회가 실제로 찾은 경우는 패치 11회·CD 16회뿐이고, "권한 없음 + 서버 번호 form + flag 0" 종료 요청이 무시되는 분기는 **한 번도 도달하지 않았다**(C++는 정적 판독대로 구현). 이를 보강한 생성기 변경을 [패치 파일](../../tools/decomp_process_oracle.coverage.patch)로 남겼다. 적용 후 재생성(이 PC에서 15분 이상 걸림)과 재검사는 사용자 지시로 다른 PC에 넘겼다. 검사가 구현 차이를 실제로 잡는지 보는 변이 확인([도구](../../tools/cpp_mutation_check.py))도 같은 이유로 실행하지 않았다.

C++의 `Attach`는 프로세스 타입 범위·부모 번호·CD의 free/dead 부모·서버의 비로컬 flags(전송 직렬화 필요)를 **Kernel 등록 전에** 거부한다. 번호 소진은 등록을 되돌린 뒤 다시 던진다. form 루트는 전용 Unpop 훅이 있는 훅 묶음(`SquidProcessHost::Hooks`)으로만 공통 destroy에 넣을 수 있다. 처리기가 실행 중인 자기 프로세스를 지워도 이후 필드를 건드리지 않는다. NaN 반환과 SharedRegular 조회는 기계어 입력에 넣지 않았다.

최종 x64 Release 경고/오류 0·CTest 실행 파일 1개 안의 내부 **200개·실패 0**(97.58초), 누적 제한 x86은 **102,284개**(96,240 + 6,044)다. 새 검사 9개는 세 PE fixture 재생, 처리기 없는 주기 반복, 다리 지연 낙하 예약/종료, 처리기 안 자기 종료, 체인 순서/중간 제거, 부착 거부, form 루트의 훅 요구다. 이전 `--verify` 감사(삭제 보상/장부/Graph/destroy/일반 탐색/다리 효과/삭제 준비)가 모두 통과했고 원본 여섯 디렉터리 **2,782개 파일의 SHA/목록이 이전 기록과 동일**하다. 로그는 `extracted/process/build.log`·`ctest.log`다.

## 재현

```powershell
$env:PYTHONPATH = 'extracted/oracle-python'
python -X utf8 tools/decomp_process_oracle.py
python -X utf8 tools/decomp_process_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기대값 생성에는 원본 PE/읽기 전용 내보내기/Python 의존성이 필요하다. `--verify`는 SHA/행 수를 감사한다. C++ 콘솔 검사에는 저장 fixture만 필요하다. 최신 빌드/회귀와 보호 파일 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다.

## 남은 연결

- 다리 이벤트 `0x2692`의 **처리기 몸체**(다리 vtable +0x5c: 지연 뒤 낙하 실행·취소)와 끝 칸 변환(`004215d0`), walker 낙하, Flyingshrapnel 파편은 아직 없다. 지금은 예약/조회와 실행 틀만 있다.
- SharedRegularProcess(타입 61)와 프로세스 전송 직렬화(vtable +8/+0x1c), 다른 파생 프로세스(이동·건설·전투·애니메이션), DependForm/GumpForm/ContentForm의 가상 함수는 복원하지 않았다.
- Kernel 프레임 실행을 실제 게임 루프와 게임 시각 전진(`0055b4d0`)에 잇고 raw GameWorld에 연결하는 일이 남았다. AI 부착 통지와 파생 vtable +0x80도 그대로다.

원본/복사본·업데이터/설치 도구 실행과 모든 창 검사는 호스트 **DESKTOP-HJOW에서 금지**되어 있다. 다른 허용 PC에서 raw 월드 연결 후 확인한다. 해당 PC의 AGENTS.md/사용자 지시를 먼저 확인한다.
