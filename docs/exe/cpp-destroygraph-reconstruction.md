# cpppj 공통 삭제의 Graph 분할·Free·주변 표면 재등록

2026-10-07. **1차 기준은 10.78**이다. [공통 삭제 장부](cpp-destroylifecycle-reconstruction.md)에 preDestroy의 Graph 분할/직접 Free와 postDestroy의 발자국→일반 finder→표면 Add를 연결했다. 호스트 **DESKTOP-HJOW에서 원본/복사본 게임·업데이터/설치 도구·클론 창을 실행하지 않았다.** CD/추가 10.37은 비교 판본이다. 10.82의 업데이터/본체 구분과 DevLog 비교는 [기존 판본 기록](cpp-reference-versions.md)을 유지한다.

소스: [SquidDestroyLifecycle](../../cpppj/src/o/SquidDestroyLifecycle.cpp), [RawGraph](../../cpppj/src/o/RawGraph.cpp), [Graph](../../cpppj/src/o/Graph.cpp). 검사: [DestroyGraphTests.cpp](../../cpppj/tests/DestroyGraphTests.cpp). 독립 원본 출력: [destroygraph-x86.tsv](../../cpppj/tests/fixtures/destroygraph-x86.tsv), [근거 기록](../../cpppj/recovery-destroygraph-evidence.json), [생성/감사 도구](../../tools/decomp_destroygraph_oracle.py).

## 원본 주소와 읽기 전용 내보내기

| 역할 | 10.78 | CD / 추가 10.37 |
|---|---|---|
| 공통 pre / postDestroy | `004b0950` / `004b0840` | `004add20` / `004adbe0` |
| 삭제 위치 정보 | `0045ae60` | pre 몸체 안의 복사 + `00443110` / `004aef30` |
| raw Graph / 현재 FrameCode 방향 조회 | `004ad390` / `004ad680` | `004adbc0` / `004ae3f0` |
| 직접 Graph Free / 삭제 분할 | `00462e50` / `004637b0` | `0045c460` / `0045bfd0` |
| 발자국 사각형 | `004ade70` → `0049ae80` | `004aeec0` → `004449d0` |
| 일반 finder Begin / Next | `004b16d0` / `004b1810` | `004eae20` / `004eafe0` |
| 표면 Graph Add | `004633c0` | `0045b7d0` |
| post 영역 mask 전역 | `005424b8` | `005395cc` |
| 특수 타입 / 치환·9 감소 타입 전역 | `005412cc` / `005412e0` | `0051cbb8` / `0051cbcc` |

[새 함수 목록](../../tools/ghidra/destroygraph-functions.json)으로 패치 **15개**, CD/10.37 각각 **17개**를 `extracted/destroygraph/<판본>/creation.c`·`functions.tsv`에 내보냈다. 세 read-only 완료 로그를 확인했으며 기존 Ghidra 프로젝트와 과거 독립 산출물을 변경하지 않았다. 새 도구는 기존 SID/생성/Graph/공간/장부 몸체와 새 불연속 함수 범위만 허용한다. 함수 사이 미검토 코드·OS 호출로 실행 범위를 넓히지 않는다.

두 실제 PE의 초기화 데이터에서 영역 mask **`0x50444200`**, 특수 타입 **157**, 치환 및 9 감소 타입 **162**를 확인했다. `SquidDeletionState`의 기본값으로 두되 런타임 전역을 공급할 수 있게 했다. 이 번호의 게임 내 이름은 이번 근거로 단정하지 않는다.

## 삭제 전 Graph 순서

선택/배치 복구 뒤, 장부 억제 검사 전에 Graph를 처리한다. `graphsEnabled`·타입 flags1의 Surface(`0x800`)·buried(extra & 8 == 0)를 먼저 확인한다. abstract(extra & 1)는 Graph를 억제하지 않는다.

1. 삭제 flags & `0x800`이면 **raw root의 Graph byte**를 직접 Free한다. 같은 위치에서 조회한 다른 SID의 번호를 사용하지 않는다. `0x2000000`(noGraph)을 함께 지정해도 Free가 먼저다. 254는 자연 반환, 정상 번호는 레코드의 앞 두 WORD만 0으로 만들며 reserved/raw 번호/스택은 보존한다.
2. Free가 아니고 noGraph가 없으면 삭제 위치 정보를 만들어 기존 `RawGraph::Detach`로 분할한다. Graph 번호는 기존 0단계 위치 조회의 머리에서 고르며 root 타입/프레임과 구별한다. [같은 위치 조회](cpp-reference-versions.md)의 계약을 유지한다.
3. root 타입이 특수 타입 157이면 현재 FrameCode 방향이 `H`일 때만 분할한다. 삭제 위치 정보의 타입을 162로 바꾸며 **raw 슬롯의 type 바이트는 유지**한다. 치환 타입의 발자국·프레임·flags2로 연결과 마지막 bit 8 판단을 하고 9를 감소시킨다. 일반 타입도 실제 유효 타입이 162이면 9, 나머지는 1을 감소시킨다.

치환 정보는 지역 연결 계산에만 적용한다. 전체 풀 재구성의 객체/타입 입력까지 바꾸지 않는다. 분할 정책(`rebuildGraph`)과 번호 소진 복구 계약(`GraphRecovery::FullPool`)은 별개다. 새 독립 입력은 소진 전이며 기존 전체 풀 복구 회귀를 유지한다.

## 공간 해제 뒤 주변 표면 Add

공통 destroy는 dead 표시→pre Graph/장부→종속/전파→실제 Unpop→post Graph/비용→SID Release 순서로 이어진다. post Graph 조건은 `graphsEnabled`, buried 없음, 삭제 flags & `0x800` 없음, **현재 실제 타입 flags2 & (regionMask | 8)**다. **noGraph는 post Add를 억제하지 않는다. 장부 억제도 pre/post Graph와 post 비용을 억제하지 않는다.**

현재 타입의 발자국으로 사각형을 만들고 두 번째 모서리를 지도 1..255에 맞춘다. 기준점이 무효인 원본 경계 처리도 기존 계약대로 보존한다. `RawSquidFinder`가 네 해시 단계의 y/x/next 순서로 교차 후보를 찾는다. flag 8 표면 탐색의 이웃 순서로 대체하지 않는다. 일반 finder는 buried만 필터하고 dead/void도 반환할 수 있으며, 각 후보의 **현재 타입**이 Surface이면 Add를 호출한다. Add의 inactive 자연 반환과 앞선 Add가 바꾼 번호/표/스택을 다음 Add에 이어 준다.

`RawGraph::PostDestroy`는 풀 사본에서 이 순서를 계산하고 성공한 graph byte·표·스택만 원본 풀에 반영한다. 해시/spot은 읽기만 하며 Unpop/Release는 담당하지 않는다. `SquidDestroyLifecycle` 생성 시 같은 풀·해시·spot·표면 타입/발자국 연결을 확인한다. 크기만 같은 별도 지도 연결도 거부한다.

## 독립 원본 대조와 검증 한계

| 공개 입력 | 세 실제 PE × 두 x87 정밀도 |
|---|---:|
| 직접 공통 preDestroy | 576 |
| 직접 공통 postDestroy | 576 |
| 실제 공통 destroy → Unpop → Graph post → Release | 576 |
| 합계 | **1,728** |

교차/선형/다중 칸·1/255 지도 경계·다른 기존 무리·254·같은 위치의 다른 SID·여러 해시 단계/머리 순서·dead/void/buried 후보·interior spot·Graph/장부 억제·Free/noGraph 우선순위·특수 H/다른 방향·치환 발자국/9 감소·원본 raw 타입 유지·직접 깊이 underflow/통합 균형을 대조한다. root/주변 일곱 슬롯과 **255개 레코드의 세 WORD 전체**를 직접 비교한다. 전체 32768슬롯 풀·해시·spot·삭제 기록·목록/통계·스택 꼬리는 Adler-32, 선택/좌표·비용/깊이/머리/꼬리는 값/비트를 비교한다. 외부 훅의 합성 vtable만 원본 기본 주소로 정규화한다.

각 PE 내부 호출은 공통 Pre/Post 각각 **384**, Destroy/Release 각각 **192**, Unpop **180**, Detach **220**, Free **20**, Add **360**, 일반 Begin **480** / Next **1,280**, 발자국 helper **2,238**, Allocate **504**, Flood **512**, AI null 래퍼 **168**이다. 준비 Reset/Create·비용 인코딩 접두와 내부 호출을 상위 입력 수에 더하지 않는다. 장부 전용의 소유자/목록/보상/소리 변형은 [이전 1,728개](cpp-destroylifecycle-reconstruction.md)에서 계속 검사한다.

**Graph/위치/발자국/finder와 실제 pre/post·장부·destroy/Unpop/Release·CRT 기록 이동은 원본 명령이다.** 선택 UI 조회/해제와 로그는 명시적 대체다(각 PE 선택 조회 576·해제 192·공통/Graph 로그 1,064). 보상/SP·실제 소리·전파/종속 파생 효과는 기존 외부 경계를 유지하며 이번 고정 입력에서 발생하지 않는다. 합성 타입/FrameCode/SHP·정수 좌표·확보한 client 풀·동결 시계·표시 억제·AI null·비전투 null 큐·Graph 소진 전 범위다. 원본 assert 0, 정상 반환/ESP/x87 제어 워드/TOP를 확인하며 별도 FS 감사·게임/OS 실행은 없다.

C++의 직접 Graph 계산은 실패 시 풀/표/스택을 보존하고, lifecycle 사전 검사는 미연결/잘못된 공간·치환 프레임/발자국·후보 오류·누락 외부 효과를 해당 쓰기 전에 거부한다. post는 비용/깊이까지 검사 뒤 반영한다. **상위 Destroy의 이미 설정한 dead·선택 콜백·pre 이후 Unpop/post 오류까지 전체 롤백하는 구현은 아니다.** 외부 효과가 raw 상태를 바꾸는 경우 해당 경계의 책임은 호출자에게 있다.

```powershell
$env:PYTHONPATH = 'extracted/oracle-python'
python -X utf8 tools/decomp_destroygraph_oracle.py
python -X utf8 tools/decomp_destroygraph_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

기대값 생성에는 원본 PE/읽기 전용 내보내기/Python 의존성이 필요하다. `--verify`는 저장된 SHA·고정 행 수·실제/대체 호출 경계만 감사한다. C++ 콘솔 검사에는 저장 fixture만 필요하다. 최신 빌드/회귀와 보호 파일 결과는 [LEFT_JOBS.md](../../LEFT_JOBS.md)를 따른다. 새 로그는 `extracted/destroygraph/build.log`·`ctest.log`·`oracle.log`다.

## 남은 연결

공통 pre/post Graph는 완료했지만 실제 보상 비율/SP/누적 수입·AI 부착 통지·종속 form/process 파생 release/destroy·참조 수명·Flyingshrapnel 파편·walker 낙하·0x2692 실행/취소·끝 칸 변환과 raw GameWorld는 남았다. 다리 전용 훅/장부/Graph/실제 효과를 전부 결합한 월드나 미션 완주를 검사하지 않았다. 다리 배치/Construction·SID 소진·건설/경제/전투/승패·V12 비교 도구 확장도 후속이다.

원본/복사본·업데이터/설치 도구 실행과 모든 창 검사는 호스트 **DESKTOP-HJOW에서 금지**되어 있다. 다른 허용 PC에서 raw 월드 연결 후 TEST01/1-1 생성→선택→삭제/반납→재진입, 그래프 연결/파편/소리/낙하·전체화면을 검사한다. 해당 PC의 AGENTS.md/사용자 지시를 먼저 확인한다.
