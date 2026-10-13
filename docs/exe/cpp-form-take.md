# Form 풀 재확보·지정 Take·공통 Unpop 복원

작업일: **2026-10-13**, 마지막 디컴파일 수행 PC: **VM-W11-CODEX**(`10.0.0.17`). AGENTS.md와 두 인수인계를 확인했으며, 같은 PC에서 사전 재생성한 정밀 프로젝트를 사용했다. 10.78을 복원 기준으로 삼고 CD 10.72는 비교 자료로 사용했다.

## 구현

`SidPool::Reset(capacity)`에 원본 초기화의 양수 인자를 연결했다. 양수이면 새 크기로 저장소를 확보하고 전체 raw 풀·세 자유 목록·머리/꼬리·freeCount·예측 커서를 다시 초기화한다. 0이나 음수이면 기존 크기를 유지하며 초기화한다. 두 삭제 기록과 서버/client 권한은 유지한다. 잘못된 크기와 메모리 확보 실패는 기존 풀을 보존하는 호스트 예외 계약이다.

**상위 호출자가 기존 UI/Kernel 객체를 먼저 정리해야 한다.** 초기화 자체는 프로세스를 삭제하거나 UI에 통지하지 않는다. 양수 재확보 후 빌린 raw span은 무효이며, 현재 월드의 상위 정리/재구성 연결은 후속이다.

새 `SquidForms`는 네 내장 Form의 실제 생성자·빈 postCreate/postTake·지정 Take와 공통 가상 Unpop을 처리한다.

| 타입 | 이름 | 10.78 가상 표 | CD 가상 표 |
| --- | --- | --- | --- |
| 1 | DependForm | 00512110 | 00505a08 |
| 2 | ProcessForm | 00512098 | 00505990 |
| 3 | GumpForm | 00512188 | 00505a80 |
| 6 | ContentForm | 00512208 | 00505b00 |

생성은 같은 `SidPool::Allocate` 뒤 가상 표 DWORD→타입 BYTE만 쓴다. 자산의 owner/HP/zOrder 초기화는 적용하지 않는다. 지정 Take는 서버 범위부터 마지막 번호까지 받으며 원본 범위에 포함된 예측 머리도 수신할 수 있다. 슬롯 전체를 지우거나 풀 목록/카운터를 고치지 않는다.

free 슬롯은 free/dead만 끄고 생성자·타입·void를 적용한다. 사용 중인 슬롯은 같은 타입의 실제 공통 Form Unpop을 먼저 호출한다. 소유 ID·부모·이전/다음 링크·나머지 payload는 해당 해제와 생성자가 실제로 쓰는 필드 외에는 보존한다. ContentForm 알림이 바꾼 payload도 Take의 후속 생성 뒤에 유지한다. 타입 불일치·미지원 생성자/가상 표·void인 사용 중 Form의 assert 이후 실행은 호스트 계약 밖이다.

공통 Unpop은 다음 순서를 유지한다.

1. void를 켠다. 부모가 free이면 반환한다.
2. contained를 확인하고 부모가 dead이면 링크/contained를 유지한 채 반환한다.
3. previous.next→next.previous→필요한 부모 head→contained 해제 순서로 갱신한다.
4. ContentForm이면 부모 가상 +0x24의 `(form,2)` 제거 통지를 요청한다. 이 통지 경계가 비었으면 살아 있는 부모의 해제는 시작하지 않는다.

Form의 previous는 **+0xe WORD**, 부모는 **+0x10 WORD**다. contained 자산 이웃의 previous는 **+0x10 WORD**여서 이웃의 현재 타입으로 쓰기 위치를 구분한다. 자신의 부모/next/previous는 별도로 0으로 지우지 않는다. ProcessForm이 실제 프로세스 타입으로 바뀐 경우에도 같은 생성자 가상 표를 확인해 공통 Unpop을 사용한다.

## 기존 모듈 연결

- `SquidFactory::CreateContained`와 `CreateGumpForm`은 같은 `SquidForms::Create`를 사용한다.
- `SquidProcessHost::Attach`의 일반 생성/지정 free Form Take와 프로세스 Form Unpop도 공통 처리기를 사용한다. Kernel 등록/타입 순환/실제 프로세스 타입·PID 기록/부모 부착·통지 순서는 유지한다. 사용 중인 프로세스의 중복 Kernel 재등록은 아직 지원하지 않는다.
- `GumpFormHost::Take`를 추가하고 기존 Unpop을 공통 처리기로 연결했다. 소유 Gump 등록 ID DWORD와 기존 메뉴/삭제 연결은 유지한다.

## 원본 근거

`tools/ghidra/formtake-functions.json`에 지정한 함수 **17개/18개**를 정밀 프로젝트에서 읽기 전용으로 내보냈다. 자료는 `extracted/formtake/<판본>/creation.c`, `functions.tsv`, 로그는 `extracted/formtake-export-<판본>.log`다.

| 역할 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 풀 초기화/양수 재확보 | 004abb10 | 004aaad0 |
| 생성 / 지정 Take | 004af530 / 004af610 | 004ab390 / 004ab440 |
| 네 생성자 | 004ae760 / 004ae3d0 / 004ae9f0 / 004aea40 | 004af920 / 004af5c0 / 004afd60 / 004afe00 |
| Form Unpop / 공통 contained 해제 | 004ae140 / 004ac6b0 | 004af270 / 004ac160 |
| 자산 이웃 previous 쓰기 | 004ad040 | 004accc0 |

`tools/decomp_formtake_oracle.py`는 C++를 호출하지 않고 두 실제 PE를 별도 Unicorn 메모리에서 정상 반환까지 실행한다. 생성·지정 Take·Unpop·초기화의 **판본별 300개, 합계 600개** 관찰을 `cpppj/tests/fixtures/formtake-x86.tsv`에 저장했다. 생성 56개, Take 336개, Unpop 192개, 초기화 16개다. 초기 기준 풀을 만든 별도 준비 호출은 이 600개에 포함하지 않는다.

네 내장 타입·server/client 권한·일반/예측 할당·free/dead/추가 상태·서버 머리/예측 머리/마지막 번호·머리/중간 해제·Form/자산 이웃·부모 free/dead·양수 성장/축소와 0/음수 초기화를 포함한다. 원본의 생성자/빈 가상 함수/가상 getter/setter/공통 해제/SID 할당은 실제 명령이다. Ghidra 허용 범위 밖 실행과 풀/지정 전역/가상 스택 밖 쓰기를 거부하고 정상 EIP/ESP 및 보존 레지스터를 확인했다. 두 PE의 assert 도달은 **0회**다.

**명시 경계는 CRT malloc과 부모 가상 +0x24 통지다.** malloc은 요청 바이트 수를 검사하고 격리된 풀 주소를 반환한다. 실제 OS 메모리 확보/실패나 원본 누수 동작을 실행한 검증이 아니다. 부모 통지는 이유 2와 콜백 당시 Form/부모 전체 바이트를 저장한다. 부모의 실제 생산/UI 효과는 실행하지 않는다.

## 검증 및 다음 작업

라이브러리/`NetstormCpp`/검사의 **Release 빌드 경고 0·오류 0**, Form 검사 **4개·실패 0**, Gump Draw 검사 **11개·실패 0**, 최종 전체 CTest **859개·실패 0**(83.98초)다. SOURCE_MAP 생성/내용 차이는 없다. 로그는 `extracted/formtake-build*.log`, `formtake-focused.log`, `formtake-gumpdraw-focused.log`, `formtake-ctest-final.log`다.

첫 전체 검사에서는 앞선 ScrollGump/VFX 대각선 지원 뒤 남아 있던 `GumpDraw_Contracts`의 예외 기대 1건이 실패했다. 해당 기대를 없애고 대각선 양 끝의 실제 픽셀과 현재 클립을 확인하는 검사 1개를 추가했다. 그리기 구현은 바꾸지 않았다. 첫 실패 로그는 `extracted/formtake-ctest.log`에 보존했으며 수정 뒤 전체 검사를 다시 통과했다.

새 C++ 검사는 600개 실제 PE 관찰의 watched 슬롯 전체 바이트·콜백 당시 전체 바이트·카운터/머리/꼬리를 직접 대조하고, watched 슬롯 밖은 전체 풀 Adler-32로 비교한다. 이 체크섬은 전체 바이트의 암호학적 동일성 증명이 아니다. 별도 검사에서 삭제 기록 보존·잘못된 크기의 기존 상태 보존·같은 처리기의 재확보 후 사용·실제 GumpForm 부착/Take/재부착/삭제·ContentForm 통지 후 payload 변경을 확인한다.

다음 작업은 **DependForm watcher/watched 쌍의 생성·삭제/통지·ContentForm 정렬 부착/나머지 가상 수명**, 풀 초기화 전 UI/Kernel 정리와 실제 전체 Client/월드 연결, 사용 중인 지정 프로세스 재등록·서버 Pop 전송이다. 다른 파생 Gump의 객체 삭제/중첩 분배·동적 메뉴·현재 전역/명령 큐/변경 영역·OS 입력·자동 raw UI도 남았다. 건설 GUI와 미션 완주는 아직 미완료다.

새 원본 관찰/C++ 대조는 각각 **600개**다. 이전 누적 523,634/523,374에 더하면 524,234/523,974다. 이 수치는 위 입력/명시 경계에 대한 관찰 수이며 전체 Form 수명이나 전체 UI의 독립 대조 완료를 뜻하지 않는다. 검증 기록은 `cpppj/recovery-formtake-evidence.json`이다.

원본 게임/GUI를 실행하지 않았고 보호 파일은 읽기만 했다. 두 PE의 SHA-256은 기존 기록과 동일하다. AGENTS.md와 dotnetpj는 수정하지 않았으며 커밋/푸시는 수행하지 않았다.
