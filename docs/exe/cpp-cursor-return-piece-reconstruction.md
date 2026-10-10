# 생산 칸 조각 반환과 contained Form 생성 복원

2026-10-11, `HJOW-Athlon`. AGENTS.md·두 LEFT_JOBS를 읽고 현재 PC와 마지막 디컴파일 PC가 같은지 확인했다. 시작 트리는 깨끗했다. 원본 게임·복사본·analyzeManager·클론 창은 실행하지 않았다. Ghidra는 `-readOnly` 내보내기만 사용했다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.**

`RawCursorReturnPiece`는 로컬 배치/GUI가 사용한 생산 칸 조각을 다시 만드는 함수다. `RawContainedCreate`는 그 조각의 Form 생성과 raw 필드 초기화를 맡는다. 기존 `SquidFactory`에는 contained 타입 6의 전용 생성 경로를 추가했다. 실제 입력/화면 연결과 가상 Form 부착·이벤트·삭제는 후속이며, 게임에서 건물을 지을 수는 아직 없다.

## 원본 주소와 복원 계약

| 역할 | 10.78 | CD/10.37 |
| --- | --- | --- |
| 생산 칸 조각 반환 전체 | `004468b0` | `0043e930` |
| contained 생성 보조 함수 | `004afa70` | `004ac4d0` |
| 게임 난수 범위 `[0,10000)` | `004558f0` | `0048cc10` |
| 다리 가중치 추첨 | `004257c0` | `0041fc70` |
| 생성 factory | `004af530` | `004ab390` |
| contained 타입 6 생성자 | `004aea40` | `004afe00` |
| 타입 6 postCreate(빈 몸체) | `004437f0` | `004b0470` |
| 타입 조회 / 패턴 WORD 쓰기 | `0049a840` | `004abb00` |
| 타이머용 Regular 생성(기존 복원 모듈) | `00496e80` | `0048efd0` |

반환 함수는 플래그·연관 SID·품질을 전체 DWORD로 받는다. 현재 cursorType은 패치 `0055a284`, CD `0051b630`의 **타입 번호**다. 앞선 커서 내려놓기의 타입 구조체 포인터 전역과 구별된다.

다리 타입과 일치하면 기존 플래그 bit `0x100`을 지우고 실제 공유 게임 난수를 한 번 소비한다. 기존 `GameRandom::Next(10000)`과 `SelectBridgePattern`을 재사용하며, 원본의 누적 가중치 `<=` 비교와 판본별 표를 유지한다. 새 패턴·품질 0·부착 플래그 `0x10`으로 contained Form을 만들고 반환한다. 다리 경로는 유지 옵션과 소멸 타이머를 사용하지 않는다.

일반 조각은 플래그 bit `0x100`을 먼저 지우고, 유지 옵션(`005318f4` / `0052e918`)이 0일 때 다시 켠다. 품질은 **DWORD에서 1 감소 → signed 음수이면 0 → 생성 helper에서 BYTE로 절단**한다. 예를 들어 `0x80000000`은 감소 후 `0x7fffffff`가 되어 품질 BYTE `0xff`로 저장된다. BYTE로 먼저 좁히거나 unsigned 제한을 사용하면 원본과 달라진다. 일반 패턴 인자는 0이다.

contained 생성 뒤 유지 옵션을 다시 읽는다. 현재 값이 0이면 `operator_new(0x28)` 성공/실패 경계를 호출한다. 실패하면 Regular를 만들지 않는다. 성공하면 할당 뒤의 요새 모드와 시간 번호를 읽어 10/5/1초 또는 요새 모드 비트 `0x38d1b717`을 payload로 이벤트 0 Regular를 붙인다. 현재 상태를 읽는 시점을 저장 관찰에서 검증했다.

## contained Form 생성

helper는 플래그 **DWORD 전체가 0일 때만** `0x20`을 기본값으로 삼는다. 부착 플래그 `0x20`이면 factory 플래그 1(예측), 그렇지 않고 `0x10`이면 2(클라이언트), 둘 다 없으면 0(서버)이다. 두 비트가 함께 있으면 예측이 우선한다.

factory 생성 뒤 `+18 WORD`에 표시 타입을, `+12 WORD`에 패턴을 쓴다. 실제 raw 타입 BYTE `+10`과 현재 bridgeType DWORD가 같으면 표면 알림 경계를 호출한다. 다음으로 품질 BYTE와 플래그 WORD를 쓰고 가상 `+0x50` 부착에 부모 SID·전체 부착 플래그를 전달한다. 패치판은 품질(`+30`) → 플래그(`+20`), CD는 플래그 → 품질(`+26`) 순서다. 알림 시점의 raw 슬롯 전체도 독립 관찰에 포함했다.

기존 `SquidFactory::Create`는 자산 타입만 지원한다. 그 보호 조건을 유지하고 `CreateContained`를 추가했다. 원본 타입 6 생성자는 가상 표 기록값(`00512208` / `00505b00`)만 쓴다. factory가 타입 BYTE 6을 쓰고 빈 postCreate를 부르며, 자산의 owner/HP/zOrder 초기화를 적용하지 않는다. 새 원본 관찰에서 서버·클라이언트·예측·두 비트 동시 생성의 SID/전체 raw를 대조했다. 예측 머리 자체는 예약 슬롯이며 첫 할당은 다음 번호다.

## 연결과 미복원 경계

`MakeContainedCreateHooks`는 같은 풀의 실제 `SquidFactory::CreateContained`를 연결한다. `MakeCursorReturnPieceHooks`는 같은 풀의 `SquidProcessHost::AddRegular`를 연결한다. 두 `MakeReturnPieceHooks` 오버로드는 각각 `RawConstructionLocal`과 `RawCursorDrop`의 반환 경계를 실제 반환 몸체에 연결한다. 다른 풀 참조는 생성 단계에서 거부한다.

전체 건설 통합 검사에서는 생산 칸 연관 부모를 만들고 미리보기 품질 3/플래그 8을 공급한다. 커서 내려놓기 → 미리보기 삭제 → 생산 칸 Form 재생성(품질 2) → 아홉 예측 조각 → 전체 MayPlace → 서버 확정/통지 → 예측 교체까지 실행한다. 생산 칸 반환 조각은 예측 교체 뒤에도 살아 있고, SP는 기존과 같이 1000 → 967.25로 한 번만 감소한다. 별도 GUI 검사는 두 번째 연관 SID와 detach 전에 캡처한 품질/플래그를 실제 반환 함수에 전달한다.

타이머 통합 검사는 실제 Form SID·Regular·ProcessForm·Kernel 등록과 10/5/1초·요새 payload를 확인한다. 가상 부착 경계에서 void를 끄고 부모 이벤트 처리기가 양수 payload를 반환하면 실제 Regular가 다음 시각을 예약하고 그 전에는 실행하지 않는다. **Form 이벤트 0의 자체 처리, 최종 조각 삭제, 일반 Form의 가상 부착/Unpop/화면 갱신은 아직 복원하지 않았다. 따라서 화면의 자동 소멸까지 구현한 것은 아니다.** 할당 경계는 성공 여부를 공급하며 호스트의 40바이트 메모리 배치를 복제하지 않는다.

원본 assert/풀 밖 산술/잘못된 시간 번호는 호스트 예외로 거부한다. 원본 SEH의 정상 머리 복구만 검사하며 예외 처리기 자체는 실행하지 않는다. 효과를 일부 수행한 뒤 계약 오류가 발생하면 롤백하지 않는다.

## 독립 근거와 검증

- `returnpiece` 읽기 전용 Ghidra 내보내기 **8/8/8개**. 저장 파일은 `extracted/returnpiece/<판본>/creation.c`, `functions.tsv`다.
- 새 관찰 **1,899개**: 판본마다 Return 528 + Create 101 + Factory 4 = 633개. 세 실제 PE × 두 x87 제어값 639/895에서 정상 루트 반환 **3,798회**, 누적 관찰 **470,967개**.
- Return/Create의 factory·표면 알림·가상 부착·할당·Regular 생성은 명시 경계다. Factory 직접 검사는 factory·contained 생성자·빈 postCreate를 실제 명령으로 실행하고 SID 할당만 경계로 공급한다. 기존 SID 모듈의 독립 원본 검사는 그대로 유지한다.
- 명령/쓰기 범위·스택·보존 레지스터·x87·SEH 머리 복구·사건/슬롯 주변 바이트·현재 전역·난수 상태·SHA/행 수/진입/정상 반환 집계를 감사한다. 원본 assert/게임/OS 호출 0.
- fixture: [returnpiece-x86.tsv](../../cpppj/tests/fixtures/returnpiece-x86.tsv). 근거: [recovery-returnpiece-evidence.json](../../cpppj/recovery-returnpiece-evidence.json). 실행기: [decomp_returnpiece_oracle.py](../../tools/decomp_returnpiece_oracle.py).
- Release 경고/오류 **0**·CTest 내부 **600개·실패 0**(147.79초, 전체 147.88초), 근거 감사 **94종 모두 통과**. 새 변이 12개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

재현 명령:

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name returnpiece
python -X utf8 tools/decomp_returnpiece_oracle.py
python -X utf8 tools/decomp_returnpiece_oracle.py --verify
cmake --build cpppj/build --config Release
ctest --test-dir cpppj/build -C Release --output-on-failure
```

새 변이 12개는 품질 감소/음수 제한·소멸 플래그·유지 옵션·난수 범위·현재 옵션 재조회·시간 번호·이벤트 번호·DWORD 기본값·예측 우선·품질 위치·contained 가상 표 오류를 대상으로 한다. `tools/cpp_mutation_check.py --focused --only <이름>`으로 저장소 밖 사본에만 적용한다. 변이 전 사본은 항상 전체 검사를 통과해야 한다.

## 다음 순서

1. 요청의 유닛 밀어내기/실패 정리와 확정의 실제 사제 선택.
2. 사제 이동 `00443000` / CD `004d1120`와 건설 시작/진행/완료, 소유 사제/접근점/contained 처리.
3. 일반 Form 부착·이벤트 0·최종 삭제/표시, 같은 조각 플래그 `0x2000000`, 실제 공간 Pop/삭제.
4. 건설 메시지 `0x2f`/`0x6c` 수신, 실제 법적 위치/미리보기·건설 메뉴/커서 GUI와 게임 실행 연결.

한국어 3차 → outpost/LAN 4차 → 요구사항/MCP 5차 우선순위를 유지한다. AGENTS.md·보호 원본·dotnetpj 변경, 커밋/푸시는 없다.
