# cpppj 돌 버튼 생성·부착·수명 및 기본 도움말 복원

작업 PC: **VM-W11-CODEX**(`10.0.0.17`), 기록일: **2026-10-12**. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 현재 PC와 마지막 디컴파일 PC가 같은 것을 확인했다. [앞 단계 버튼 검증](cpp-button-gump-validation.md)의 다음 구현 항목을 진행했다.

새 `stonebuttongump-functions.json`으로 **16/19/19개 함수**를 읽기 전용으로 내보냈다. 기존 `buttongump` 내보내기/fixture/근거 기록은 유지한다. **마지막 디컴파일 수행 PC: VM-W11-CODEX, 2026-10-12.**

이번 결과는 **정적 복원과 Release 컴파일 완료**다. 새 테스트 추가/실행·독립 PE 실행 관찰·GUI/원본 게임 실행은 수행하지 않았다. 기존 **827개 테스트 통과**, 독립 원본 누적 **520,756개**는 앞 단계 기록이며 이번 변경의 실행 검증 결과로 사용하지 않는다.

## 원본 대응

| 처리 | 10.78 패치판 | CD 10.72 / 추가 10.37 |
| --- | --- | --- |
| 돌 버튼 전체 생성 | `004254b0` | `00441310` |
| 돌 버튼 가상 부착 | `00424f00` | `00441520` |
| 텍스트 측정 후 크기 지정 | `00424f90` | `004415c0` → `004415a0` |
| 라벨 변경 후 측정 | 별도 대응 함수 미확정 | `00441610` |
| 기본 Gump 텍스트 측정 위임 | `00465020` | `00493ea0` |
| 배경 Gump 생성 | `004655d0` | `00494640` |
| 돌 버튼 scalar 소멸자 | `004d4c40` → `004d0110`(thunk) → `00464420` | `00441cd0` → `00441d29` → `00403c60` → `00403ca1` → `00492ef0` |
| 기본 도움말 사건 | `00464990` | `00493720` |

실제 PE의 가상 표 **`00503898` / `00501828`**와 x86 명령을 읽어 부착 함수의 네 인자, 생성자의 다섯 인자, 소멸자 슬롯을 보완했다. Ghidra 부착 출력의 `unaff_ESI`/반환 주소와 CD의 일부 정수 필드 표기는 그대로 옮기지 않았다. 실제 부착 명령은 float 크기 쓰기와 `(x,y,parent,mode)` 전달이다. 두 PE의 뺄셈 상수는 2다.

## 생성과 텍스트

`StoneButtonGumpNode`는 기존 `ButtonGumpNode`의 상태/배경 토큰과 같은 기본 노드를 공유한다. 별도 필드가 라벨 **64바이트(+c0..+ff)**, 세 글자 문맥 **+104/+124/+144**, 측정값 **+164/+168**에 대응한다. C++ 객체를 원본 물리 배치로 재해석하지 않는다.

`Construct`는 기존 `ButtonGump::ConstructBase(width,height,0)`에 위임한다. 원본 다섯 인자 중 네 번째 `unused`는 읽지 않으므로 버튼 플래그로 사용하지 않는다. 돌 버튼 vtable/kind와 signed BYTE 색을 지정하고 원본이 쓰는 문맥 DWORD만 초기화한다.

- 각 문맥의 앞 네 DWORD만 0으로 지정한다. 정상/눌림 문맥은 검정 글자·흰 그림자·flags 2·그림자 (1,1)이고 비활성 문맥의 글자색은 흰색이다.
- 세 문맥의 `extra`, 비활성 문맥의 그림자색/좌표는 원본처럼 보존한다. 안전 출력용 `ButtonTextContexts`의 전체 0 초기화로 대체하지 않는다.
- 기본 측정 문맥은 인자 또는 현재 전역의 **32바이트**를 복사한다.
- null 라벨은 첫 바이트만 0으로 쓴다. non-null 라벨은 `strncpy(63)`의 짧은 문자열 0 채우기를 유지하며 **마지막 label[63]은 쓰지 않는다**. 긴 문자열에 자동 종료 문자를 추가하지 않는다.
- 생성자가 측정값을 초기화하거나 자동 측정하지 않는 순서를 유지한다.

`SetLabel`은 CD `00441610` 근거로 non-null 라벨 복사 뒤 측정만 수행한다. 부모 크기·배경 크기·재그리기를 자동 변경하지 않는다. 패치판의 독립 함수 대응/사용처는 아직 미확정이다.

`FitToText`는 현재 기본 문맥으로 실제 측정 경계를 호출해 두 출력 필드에 쓰고, 콜백 후 현재 값을 읽어 폭 **+12**·높이 **+6**을 지정한다. DWORD 정수 감김과 기본 float 저장/배율 적용 크기를 구분했다. 임의 x87 극단 입력의 동일성은 아직 실행 대조하지 않았다.

`DrawPlan`은 실제 저장한 세 문맥과 측정값을 기존 `PlanButtonDraw`에 전달한다. 실제 글리프 출력·폰트 측정 몸체는 이 단계의 범위에 포함되지 않는다.

## 배경 생성과 부착

`GumpBackgroundNode`의 **+a0/+a4/+a8/+ac/+b0/+b4**는 SHP·signed 타일 크기·타입·프레임·배경 플래그다. `GumpBackgroundCreate::Construct`는 기존 기본 생성/등록에 위임하고 원본 배경 vtable/색/필드를 지정한다. 타입 0이면 shape/타일 크기를 보존한다. 실제 타입/SHP 헤더는 명시한 자산 조회 경계에서 공급한다.

패치판은 타입 조회→프레임 조회→타일 크기→현재 타입 SHP 쓰기이며, CD판은 SHP 쓰기→프레임/타일 크기 순서다. 헤더 폭/높이는 signed WORD에서 부호 확장한다.

돌 버튼 생성자는 **new(0xb8)**을 요청하고 현재 타입/프레임 전역, 원본 정수 크기 **-2**, 배경 플래그 **0x03e00000**, 기본 Gump 플래그 1을 전달한다. 결과를 부모의 background에 쓰고 자식 `secondaryDepth(+34)=1`을 지정한 뒤 가상 Attach `(1,1,this,0)`을 호출한다. 할당 실패 뒤 원본은 null 접근에 도달한다. 호스트는 background=0을 쓴 뒤 예외로 거부하며 이미 등록한 부모/필드를 되돌리지 않는다.

돌 버튼 가상 `Attach`는 다음 순서를 따른다.

1. 현재 배경의 가상 Push.
2. Push 후 현재 배경/부모 기본 크기를 읽어 배경 폭·높이를 각각 **-2**로 조정.
3. 배경 가상 Attach `(1,1,this,0)`.
4. 부모의 실제 기본 `GumpTree::Attach(x,y,parent,mode)`.

마지막 기본 부착의 redraw/자식 재배치 순서는 기존 GumpTree가 처리한다. 배경 secondaryDepth로 발생하는 추가 자식 재배치를 별도로 생략하지 않는다.

## 소멸자 분배와 연결

돌 버튼의 실제 가상 소멸자 슬롯은 기본 Gump 소멸자에 위임한다. 배경 전용 free를 추가하지 않는다. 기존 `GumpLifetime::Destroy`가 자식 우선 파괴·등록 해제를 수행하고 `StoneButtonGump::DeleteScalar`가 기존 기본 scalar 소멸자/선택적 객체 free를 호출한다.

`BindStoneButtonGump`는 기존 트리 `attach`와 수명 `deletingDestructor` 훅을 감싼다. 현재 resolve가 돌 버튼을 반환하는 토큰만 파생 몸체를 호출하며 나머지 분배기를 보존한다. 부모 부착의 마지막 단계는 기본 GumpTree 몸체를 직접 호출하므로 자신의 파생 훅으로 재귀하지 않는다.

조립할 때는 다음을 같은 저장소로 공급한다.

1. `GumpCreateHooks::node`가 기본/버튼/돌 버튼/배경 노드의 같은 기본 주소를 반환하도록 만든다.
2. 기존 `BindButtonGump`와 새 `BindStoneButtonGump`를 처리기 생성 전에 묶는다. resolver는 초기에는 null이고 처리기를 만든 뒤 현재 포인터를 설정한다.
3. 배경 할당은 새 `GumpBackgroundNode` 저장소 토큰을 제공한다. Stone 훅의 `push/attach`는 배경의 기존 기본 GumpTree 처리기로 분배한다.
4. 기존 버튼 `backgroundFlags`는 실제 `GumpBackgroundNode::backgroundFlags` 주소를 반환한다.
5. 가상 scalar 소멸자와 free는 실제 안정된 저장소 수명을 관리한다. 직접 scalar 호출과 자식 우선 Destroy를 같은 작업으로 취급하지 않는다.

## 기본 우클릭 도움말

`GumpHelp`는 원본 기본 도움말 사건 분기의 몸체를 구현한다. **오른쪽 누름·현재 alternateTextPointer 비영**일 때만 진행한다.

1. alternate 문자열에서 첫 NUL까지의 바이트로 `%sHelp` 주제를 만든다.
2. 제어 메뉴 시작 helper에 0을 넘긴다.
3. helper 이후 현재 textPointer를 제목으로 쓰고, 0이면 현재 alternateTextPointer를 쓴다.
4. 제목 콜백 이후 현재 본문/이벤트/주제로 항목을 생성한다. 패치 인자는 `(body,event,topic,1,1,0)`, CD는 마지막 0이 없는 다섯 인자다.
5. 항목 콜백 이후 현재 등록표 **entries[0]**을 부모로 사용해 전달 좌표에 메뉴를 부착한다.
6. `opengump.wav` 경계를 호출하고 1을 반환한다. 조건이 맞지 않으면 0이다.

`BindGumpHelp`로 기존 `ButtonGumpHooks::baseEvent`에 연결한다. MenuGump/Form의 실제 생성·제목 포맷·항목 폰트·장치 몸체는 훅 경계이며 복원 완료로 계산하지 않는다. 버튼의 disabled 조건과 도움말 없는 오른쪽 통지는 기존 ButtonGump가 먼저 처리한다.

## 빌드 및 다음 작업

`NetstormCpp` Release와 게임 라이브러리 컴파일 성공, **컴파일러 경고/오류 0**. 테스트 실행 파일/CTest·GUI는 이번 단계에서 실행하지 않았다. 빌드 로그는 `extracted/stonebuttongump-build.log`, 내보내기는 `extracted/stonebuttongump/{originals,originalCD,original1037}/`에 있다.

가상 표·명령/상수의 읽기 기록과 원본/목록/내보내기 SHA는 `extracted/stonebuttongump/static-pe-notes.json`에 보존했다. 원본 몸체를 실행한 관찰 fixture가 아니다.

| 내보내기 | SHA-256 |
| --- | --- |
| 패치 creation.c | `d81b7ec00f445a604b764ee81575f424b5b7ef5fe62a9ca06ba7f728ba930e0b` |
| 패치 functions.tsv | `e8496c950900558597760d23d7316571a795dd65f65a1e0b0bfaaab6fc0c436a` |
| CD·10.37 creation.c | `88bb0aaddd9afced80c73d11c26d4f664bd64999b1a74a805561baa5f73d2d00` |
| CD·10.37 functions.tsv | `d9017d930e1270938d1c4ad12f77395e1d23ad2c337f012c7d439abb3c3b1867` |

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name stonebuttongump
cmake --build cpppj/build --config Release --target NetstormCpp --parallel 2
```

다음은 **새 생성/부착/수명/도움말 경계의 독립 PE 실행 대조와 C++ 연결 검증**, 실제 폰트 측정/글리프·자산 저장소 공급, MenuGump/Form·OS 큐/폴링·overlay·Client 자동 UI 프레임이다. 현재 실제 Client/UberGump는 기존 메뉴 어댑터를 사용한다. 건설/경제/전투/AI/승패와 **미션 완주는 아직 불가능하다**.

AGENTS.md·원본 파일·dotnetpj/인계는 수정하지 않았고 커밋/푸시하지 않았다. 두 원본 exe 및 추가 10.37 바이너리 SHA는 앞 단계와 같다.
