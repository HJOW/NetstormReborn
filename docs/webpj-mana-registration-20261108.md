# webpj 마나 공급원 등록(00472f70) 이식 — WB04-E10

2026-11-08, HJOW-Athlon. [LEFT_JOBS.webpj.md](../LEFT_JOBS.webpj.md) 3.2절 순서3의 WB04-E 중 열 번째 부분이다. cpppj 기준선 `1f00a25a`의 `o/RawManaRegistration`(10.78 경로만)을 옮겼다. 공통 postPop 본체가 부르는 작은 선행 단위다. 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다(Node 단위 검사만). 기준선 뒤 cpppj 변경은 없다.

## 1. 옮긴 것

`www/core/raw-mana-registration.js` — `registerManaProvider(list, state, value, diagnostics)`.

1. 최초 개수를 signed로 **한 번** 저장한다. 양수면 첫 원소부터 같은 값을 찾는다(첫 검사는 개수 재조회보다 먼저이고, 이후 반복 조건은 저장한 개수).
2. 중복이고 현재 `checking`(005e4794) DWORD가 비0이면 원문 진단(`!alreadyThere`, `\Ns\O\Mana.cpp`, 0x44)을 요청한다. 진단이 정상 복귀해도 **저장한 중복 판단은 다시 검색하지 않는다**(진단이 목록·checking·카운터·중복 항목을 바꿔도 그대로 이어진다). 진단 장치가 없으면 그 위치에서 거부한다.
3. 중복이 아니고 저장한 개수가 용량(signed 비교)보다 작으면 그 위치에 값을 쓰고 **현재 개수를 읽어** 하나 더한 값을 개수에 쓴다(꼬리 값은 지우지 않음).
4. 중복·가득 참·음수 용량 어느 경우든 `state.productionDirty`를 32비트 감김으로 한 번 증가한다.

`value`는 원문 DWORD 전체이고 포인터 범위 밖 접근은 `RangeError`(호스트 장치 오류)다. `state`는 `productionDirty`를 가진 객체(`SquidPostPopState`)다. 목록은 이미 옮긴 `SquidPostPopList`(`bindCurrent`로 Player 현재 배열 연결)를 쓴다.

## 2. 검증 (`tests/mana-registration.test.mjs`, 3건)

| 검사 | 근거 | cpppj 대응 |
| --- | --- | --- |
| 542행: 포인터·개수·용량·카운터·checking·두 물리 배열·진단 이벤트 (진단에서 목록 교체/checking 해제/카운터 변경/중복 삭제 시나리오 포함) | `mana-registration-hardware-x86.tsv`의 `originals` 행(같은 파일의 `originalCD` 542행은 10.72라 제외) | ManaRegistration_PatchNativeDuplicateResumeAndCurrentCounter |
| 첫 중복 검사가 다음 현재 개수 읽기보다 먼저 끝남 | (독립 입력) | ManaRegistration_FirstSearchPrecedesNextCurrentCountRead(패치 쪽) |
| 진단 장치 누락/중단은 앞선 효과 보존, 선택하지 않은 배열 장치는 읽거나 쓰지 않음 | (독립 입력) | ManaRegistration_SelectedDevicesAndPartialEffects |

최초 실패와 교정: 구현 직후 542행과 계약 검사가 처음부터 통과했다. 기대값은 원문 열만 썼다.

## 3. 경계

- cpppj `ManaRegistration_ActualClientWorldCurrentOwnerAndPostPopContinuation`(실제 Client checking·장기 공급원 목록을 두 World 세대가 같은 물리로 공유하며 공통 postPop이 이어지는 통합 검사)는 공통 `SquidPostPop` 본체와 World 조립 단위에서 대조한다.
- CD 10.72 경로(진단 없음, 반복 조건에서 현재 개수 재조회)는 범위 밖이다.

## 4. 다음 시작점

`RawGraph`/`RawGraphLive`(Graph 소속 관리; 공통 PostPop의 선행), `SquidPostPop` 공통 본체, 그리고 Player 나머지(`RawPlayerWorld`·`RawPlayerTechnology`·`RawPlayerRegular`·`RawPlayerTemplateState` 등).
