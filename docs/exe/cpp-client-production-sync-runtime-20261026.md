# World 없는 HUD mode1의 실제 생산 동기화

분석·작성 PC는 **HJOW-Athlon / 2026-10-26**이며 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 원본 PE·디컴파일 산출물·기존 Native fixture는 수정하지 않았다. 이 문서는 단위467의 Client 수명 연결을 기록한다. 작성 시점에는 새 C++ 검사와 통합 빌드가 아직 실행되지 않았다.

## 원문과 기존 중단

State3의 첫 HUD mode1과 tail의 두 번째 mode1은 World/Session이 없는 상태에서도 `0200003d, first=1`을 생산 패널에 전달한다. 생산 입력 `0043ec00`은 현재 기술 부모를 두 격자에 Bind한 뒤 실제 `0043df80`을 호출한다. 이 부모는 `0044f760`의 현재 templateOwner Deck을 먼저 확보하고, `0048fc60`의 현재 기술을 `004b2270/004b1b20` 내용물 finder로 순회한다. PlayerKnown0이 돌려준 SID0도 실제 예약 슬롯의 head를 읽는다.

기존 연결은 first1에서 항상 `RawCampaignSession::SynchronizeProductionContents()`를 요구했다. 실제 Session이 없는 State3에서는 `RequireSession` 예외로 중단했다. 이 호출을 생략하면 실제 Player+68 덱 생성과 SID0 순회가 빠지므로 원문과 다르다.

계산 본체와 독립 원본 관측은 기존 `RawProductionSync`/`ProductionSyncTests` 및 `RawProductionDeck`/`ProductionDeckTests`를 그대로 재사용한다. 이번 변경은 새로운 규칙 계산이나 기대 fixture를 추가하지 않는다.

## 연결한 실제 수명

- `RawProductionDeckStorage`는 실제352BYTE 블록과 opaque 주소만 보관한다. `Get`은 현재 Player+68이 비영이면 같은 생존 블록을 반환하고, 0이면 현재 할당 입력을 보존한 채 원문 다섯 DWORD만 초기화한다. 다음 주소는 Session 생성/반납에서 되감지 않는다.
- `ClientPlayerRecords::ProductionDeckMemory()`는 같은 저장소를 Client 수명으로 소유한다. Session의 기존 `RawProductionDeck`은 그 저장소를 빌리며 실제 `RawPlayerWorld`, `Player()`, 기존 PlayerLinksPass와 반납 관문을 유지한다. 독립 raw 검사는 기존 자체 저장소를 사용한다.
- `ClientProductionSync`는 Runtime과 Storage가 대여 중이면 종전 Session의 `SynchronizeProductionContents`로 간다. 설치 전 일반 접근은 기존 예외를 계속 낸다. 두 대여가 실제로 없을 때만 현재 Player/Pool/TYPE/finder/FullDeck을 같은 `RawProductionSync`에 조립한다.
- World 없는 `deck` 훅은 실제 현재 templateOwner와 Player 기록으로 같은 장기 저장소의 getter를 실행한다. 기술/신전 getter는 호출 시점의 실제 PlayerKnown·battleOwner·Player DWORD를 읽는다. 현재 contained-kind 조회는 기존 `RawCarrierPreDestroy::FindContained`의 원문 WORD 조건 몸체를 사용한다.
- 현재 Cursor 취소, 삭제, 생성이 실제 선택되면 여전히 현재 Runtime이 필요하다. 기본 기술0/빈 Deck 경로에서만 계산 본체가 자연 반환하며, 미연결 효과를 상수 성공으로 대체하지 않는다.
- `ClientGameplayGlobals`의 finder는 World 이전부터 실제 authority.boss 주소를 빌린다. 기존 일반 Acquire의 finder 초기화 뒤에도 같은 주소를 다시 연결한다. 값 사본으로 dead 부모 진단을 통과시키지 않는다.

`RawProductionControls`의 first1 호출만 이 접합으로 교체했다. 기존 Session의 loaded/editorUiInstalling/editorPrefixUiInstalling guard는 변경하지 않았다. Idle은 현재 기술0이면 `UpdateContents` 및 유지 몸체 생성 전의 원문 관문에서 자연 반환한다.

## 다음 Native 정리와 실패 경계

World 없는 HUD도 실제 비영 Player+68을 만들기 때문에 다음 State1/일반 Login의 Native `CleanupPlayerLinks`가 이를 해제해야 한다. `ClientStateRuntimeRetirement::ClearPlayerLinks`의 Session 없는 분기는 이제 같은 장기 저장소의 실제 `Release(token)`을 원문 free 위치에 연결한다. 후속 Player+68/+9c/+8의0쓰기는 기존 원문 부모가 수행한다. 같은 Session이 살아 있는 분기는 기존 이동 전용 증빙을 계속 요구한다.

Player 초기화/Pool reset 순서를 이 모듈에서 새로 실행하지 않는다. ClientPlayerRecords 초기화는 저장소를 재생성하지 않고, 새 Session도 생성 시 블록을 초기화하지 않는다. 일반 C++ Session 소멸의 기존 링크 정리는 같은 실제 블록을 반납한다. Native 반납 증빙을 이미 소비한 RawProductionDeck 뷰는 기존과 같이 접근을 거부한다.

## 준비한 검사와 통합 절차

새 `ClientProductionSyncTests.cpp`의 네 검사는 창·World·Session을 만들지 않은 실제 Client/Player/Pool/공통 UI를 사용한다.

1. 생산 패널 first1 입력의 실제352BYTE 할당, 다섯 DWORD, 두 번째 입력의 같은 포인터/미정 꼬리 보존, 현재 templateOwner2 재조회, 기술0 Idle 및 Pool 불변을 확인한다.
2. 예약 SID0의 손상 head로 실제 finder 실패가 Deck 확보 뒤 발생하는지 확인한다. 실제 내용물의 seen/Deck power를 읽은 자연 반환, 선택 취소의 Runtime 부재 실패, 현재 boss=false의 dead 부모 진단도 확인한다.
3. 비영 noWorld Deck을 실제 ClientStateRuntimeRetirement로 해제하고, 모든9행의 세 DWORD0, 이전 블록 조회 실패, 다음 실제 Player 재할당 뒤 같은 저장소/새 토큰을 확인한다.
4. 미공개 Player 대여 또는 Storage 대여 중에는 World 없는 경로로 우회하지 않고 종전 Runtime 부재 예외가 나는지 확인한다.

`CampaignSaveGoMainTests.inc`에 추가한 `CampaignNoWorldProduction_AdoptsActualClientDeckBeforePrefix`는 Session 생성 전에 실제 getter/동기화를 실행한다. 준비 Session이 같은 포인터/블록을 초기화 없이 빌리는지, 블록이 이미 있어도 설치 전 공개 동기화 두 경로가 모두 거부되는지, 실제 Session 소멸 뒤 블록이 반납되는지 확인한다. 기존 checkpoint TU의 helper와 같은 실제 Session을 재사용한다.

통합 담당은 `ClientProductionSync.cpp`와 `ClientProductionSyncTests.cpp`를 CMake에 추가하고 기존 checkpoint TU 끝의 `CampaignSaveGoMainTests.inc` include를 유지해야 한다. 새5개와 기존 ProductionDeck/ProductionSync/ProductionInitialContext/State1 prefix/Runtime retirement 검사를 실행한 뒤 전체 통합 검사 및 State3→다음 Poll4의 GUI 경로를 검증한다. 본 문서의 검사 목록은 준비 범위이며 통과 보고가 아니다.

## 467 최종 통합 결과

Source6 집중181고유/189관측·신규29·전체C++3,194개·실패0, 같은 게임 GUI8·자연Early2→Tutorial3를 통과했다. 본문의 준비/연결 지침은 이 통합 제품에 적용됐다. [최종 문서](cpp-editor-menu-runtime-integration-20261026.md)와 [보고서](../../cpppj/editor-menu-runtime-integration-20261026.json)를 기준으로 통과 범위와 남은 전체 목표를 구별한다.
