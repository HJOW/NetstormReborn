# 건설 그래프 기준점 연결과 소유 사제 조회 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md·LEFT_JOBS.md·LEFT_JOBS.dotnetpj.md를 읽고 [밀어내기/섬 정리](cpp-construction-effects-reconstruction.md)의 다음 조회 항목을 진행했다. 시작 작업 트리는 깨끗했고 현재/마지막 디컴파일 PC가 같았다. `priestlookup` **1/1/1개**를 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본/복사본 게임·analyzeManager·클론 창은 실행하지 않았다.

## 두 조회의 의미

확정의 `findBuilder`가 호출하는 함수는 이미 [Player 배치 기준점](cpp-player-anchor-reconstruction.md)으로 복원한 작업장/그래프 조회다. 기존 확정 문서와 인계의 “확정의 실제 사제 선택” 표현을 바로잡았다. 실제 소유 사제를 찾는 함수는 배치 통지의 `findPriest`에서 따로 호출한다.

| 호출 지점 | 10.78 | CD/10.37 | 이번 연결 |
|---|---|---|---|
| 확정 `findBuilder` | `0048fdb0` | `00406f80` | 기존 `RawPlayerPlacementAnchor::Locate` |
| 배치 통지 `findPriest` | `004921f0` | `0040d940` | 새 `RawPriestLookup::Find` 전체 몸체 |
| 공유 사제 목록 포인터 / 개수 | `005954c4` / `005954cc` | `00549188` / `00549190` | 기존 `PriestPostPopState.priests` |
| 현재 소유자 raw BYTE | +34 | +32 | 매 조회마다 읽음 |

[`RawConstructionSelection`](../../cpppj/src/o/RawConstructionSelection.h)의 `MakeConstructionAnchorHooks`와 `MakeConstructionPriestLookupHooks`는 같은 풀인지 검사하고 각각의 원본 호출 지점에 조회를 연결한다. 기존 `findBuilder` 이름은 API 호환을 위해 유지하고 주석을 수정했다. factory·확정/통지·정리/배치 어댑터와 합성할 수 있다. 풀·조회·공유 상태의 수명은 반환 훅보다 길어야 한다.

확정의 `skipBuilderCheck`와 기준점의 `ignoreRestrictions`는 원본에서 **동일한 전역** `0054db14`/CD `0052e8a8`이다. 현재 C++의 두 상태 뷰를 공급하는 호출자는 같은 값을 유지해야 한다. 이 단계는 전체 월드 전역 호스트를 새로 만든 것이 아니다.

## 소유 사제 조회 계약

[`RawPriestLookup`](../../cpppj/src/o/RawPriestLookup.h)은 공유 목록을 복사하지 않는다.

1. 목록 개수를 **signed DWORD**로 보아 0 이하이면 바로 0을 반환한다.
2. 활성 DWORD SID를 목록 순서로 읽는다. 정렬·중복 제거·거리 비교를 하지 않는다.
3. raw의 **현재 owner BYTE**가 인자 **player DWORD 전체**와 같고 raw 상태 +11의 **dead bit 2**가 없으면 그 SID를 반환한다. 예를 들어 3과 `0x103`은 다르다.
4. 첫 일치에서 멈추며 없으면 0이다. SID 0이 먼저 일치하면 그 자리에서 0을 반환한다.

조회 몸체는 **free bit 1·void bit 4·타입·genus·extra·원래 소유자 WORD를 검사하지 않는다**. 통지 처리기는 조회 뒤 첫 사제의 free/dead·genus·이동 불가를 별도로 검사한다. 목록에 잘못된 타입이 섞인 경우 이 두 단계의 동작을 합쳐서는 안 된다.

C++ 안전 계약은 양수 개수가 저장 용량을 넘거나 실제 읽을 DWORD SID가 풀 밖이면 예외로 거부한다. 범위 검사를 하기 전에 SID를 WORD로 자르지 않는다. 첫 일치 뒤의 잘못된 SID와 signed 음수 개수의 목록 저장소는 읽지 않는다. 이 검증이 원본 함수에 있다는 뜻은 아니다.

## 연결 검사

[`PriestLookupTests.cpp`](../../cpppj/tests/PriestLookupTests.cpp)의 새 검사 **8개**는 세 PE 관찰 재생과 다음 효과를 확인한다.

- 실제 사제 factory 생성자 → `RawPriestPostPop::Prefix` 등록/중복 억제 → 실제 `SquidOwner::Set` → 사망 상태 변경 → `RawPriestPreDestroy`의 목록 압축을 같은 조회 객체가 즉시 본다. 회복 예약/Carrier/보호막의 하위 효과는 명시 경계다.
- 확정이 실제 기준점의 그래프 불일치/빈 작업장 목록에서는 서버 SID를 만들지 않는다. 일치하면 실제 factory로 SID를 만들고 통지한다. 플래그 bit 0/생략 전역의 우회도 확인한다.
- 실제 확정/factory → 실제 통지 처리기에 소유 사제 조회를 연결한다. 10.78의 contained 타입 경계가 첫 사제의 소유자를 바꾸면 두 번째 조회와 제거는 다음 사제를 쓰고, 이동은 첫 조회에서 보관한 사제를 유지한다. CD는 한 번만 조회한다. 두 사제가 모두 사망한 경우 환불 → 삭제 → 갱신 순서를 확인한다.
- 다른 풀 연결, 손상된 개수, 상위 비트 SID, 첫 일치 뒤의 손상 항목, 예약 SID 0의 조기 종료를 확인한다.

이 연결 검사의 정리/배치·contained·이동 불가/접근점/이동·환불/삭제·표시는 관찰 경계다. 기존 실제 정리/배치와 사제 상태 어댑터는 유지한다. **건설 사제 생성/이동·건설 시작/진행/완료·게임 입력/GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.**

## 독립 원본 관찰과 검증

[`decomp_priestlookup_oracle.py`](../../tools/decomp_priestlookup_oracle.py)는 세 실제 PE의 **조회 전체를 대체 함수 없이** 정상 반환까지 실행한다. 합성 128슬롯의 현재/원래 소유자·타입·상태·extra, signed 개수, 목록 순서/중복/비활성 꼬리/예약 0, player DWORD 폭을 교차했다. 두 x87 정밀도에서 스택·보존 레지스터·x87 균형·허용 실행/쓰기·raw/목록 입력 불변을 확인했다. 원본 assert·게임·OS 호출은 0이다.

새 저장 입력 **4,851개**(각 PE **1,617개**), 정상 반환 **9,702회**, 누적 저장 입력 **479,352개**(474,501 + 4,851). [fixture](../../cpppj/tests/fixtures/priestlookup-x86.tsv)와 [근거 JSON](../../cpppj/recovery-priestlookup-evidence.json)의 SHA·입력 순서·DWORD 결과 폭·진입/정상 반환 수를 감사한다. 기존 기준점의 독립 원본 **4,794개**를 재사용하며 새 합계에 중복으로 세지 않는다. 확정/기준점/통지의 합성은 C++ 통합 검사이지 원본 합성 실행의 새 관찰이 아니다.

Release 경고/오류 **0**·CTest 내부 **614개·실패 0**(153.60초, 전체 153.64초), 근거 감사 **96종 모두 통과**. 새 변이 8개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과).

## 다음 순서와 재실행

2026-10-11 후속: [건설용 객체 생성·이동과 공급원 소비](cpp-construction-builder-reconstruction.md)의 전체 몸체와 로컬/통지 이동 어댑터를 완료했다. 아래는 이 절 작성 시점의 다음 순서다. 현재는 건설 시작/진행/완료와 하위 경로·접근점·자원 정책·공간 효과를 이어서 진행한다.

다음은 건설 사제 생성/이동 `00443000`/CD `004d1120`, 건설 시작 `00443fe0`/CD `004d06a0`와 진행/완료다. `00443000`은 건설용 사제 객체를 만드는 별도 흐름이므로 이번 소유 사제 조회와 구분한다. 접근점/contained 효과, 경로와 타입별 공간 Pop/Unpop·파생 삭제/추락/알림·Form 부착/이벤트 0/최종 삭제, 메시지 수신과 실제 배치 판정/미리보기·건설 GUI도 남는다. 한국어 3차 → outpost/LAN 4차 → 요구사항/MCP 5차를 유지한다.

```powershell
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name priestlookup
python -X utf8 tools/decomp_priestlookup_oracle.py --verify
cmake --build cpppj/build --config Release
cpppj/build/bin/Release/netstorm_tests.exe --filter PriestLookup_
ctest --test-dir cpppj/build -C Release --output-on-failure
python -X utf8 tools/cpp_mutation_check.py --focused --only priest-lookup-owner-width
```

로그는 Git 제외 `extracted/priestlookup-*.log`다. 변이는 저장소 밖 사본에만 적용한다. AGENTS.md·보호 원본·dotnetpj 변경과 커밋/푸시 없이 진행했다.
