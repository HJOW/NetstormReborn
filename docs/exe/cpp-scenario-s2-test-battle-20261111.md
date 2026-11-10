# S2: 새 맵 기본 english·english 부재 Test Battle, 그리고 Test Battle 종료 결함 교정 (제품 1282, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음, 재디컴파일 없음).

## 1. 확인한 동작(실제 실행, 격리 사용자 폴더·복사본 게임 폴더)

| 항목 | 결과 |
| --- | --- |
| 새 맵 만들기(`Edit` → `Create New Map` → 이름 `S2TEST` → OK) | 사용자 작업 폴더에 `CustomMaps/d/S2TEST.fort`와 함께 **`S2TEST.english`(208바이트, BOM+기본 Header/Options/`[A.]` 브리핑 `Go!` 버튼)가 자동 생성**된다. |
| english가 없는 기존 맵의 Test Battle(복사본에서 `TEST01.english` 삭제) | 사용자 작업 폴더에 같은 기본 대본이 `CustomMaps/D/TEST01.english`로 자동 생성되고, 브리핑 `TEST01 / Go!`가 뜬 뒤 전투가 진행되며 `Return to Editing`으로 편집기에 정상 복귀한다. 종료 코드 0, 오류 로그 없음, 게임 폴더 불변. |
| english가 있는 기존 맵의 Test Battle(원본 TEST01) | 사용자 대본이 그대로 쓰인다(브리핑 `EQUIPMENT / Go!` 유지). 기존 수동 대본을 덮어쓰지 않는다. |

원문 10.78의 "english 파일이 없으면 Test Battle에서 튕기는 문제"(AGENTS 9차)는 S1에서 병합한 `EditorScenarioFiles::EnsureEnglish`(새 맵 저장 때, Test Battle 직전)와 `ReadForMission`(사용자 작업 폴더 English 대체 읽기)으로 해결되며, 위 실행으로 확인했다.

## 2. 실행 중 발견·교정한 결함: Test Battle이 항상 종료

english 유무와 무관하게 `Edit → Game → Test Battle`을 누르면 State 8에서 다음 오류로 종료했다(M6 JoinTestBattle 이후 Test Battle 실제 실행은 한 번도 없었다).

`복합 전송이 소유하지 않은 현재 수신 노드: id=-1 …`

- 원인: Editor 세션의 `EditorLocalTransport`는 server 끝을 **pending -1에 계속 둔다**(Editor는 로그인이 없다). Test Battle 로 넘어가며 State 7이 client 끝(slot 9)만 닫지만 Editor 객체(`UberGump::editor_`)는 Test Battle 동안 살아 있다(Return to Editing용). 새 전투의 `CompositeMissionTransport`가 첫 Poll에서 pending -1을 읽을 때, 등록표에는 토큰이 있으나 이 composite의 어떤 backend도 소유하지 않아 `Backend::Unknown`이 되고 `ReceiveOne`이 오류를 던졌다. 헤더 주석("Unknown은 다른 외부 owner여서 dispatch하지 않는다")과 구현이 어긋나 있었다.
- 원문 근거: 원문 `004df560`의 pending 순회는 각 노드가 자기 수신 함수로 스스로 처리하며(`004e29c0`/`004e2a10`은 정상 칸 0..21/1..8만 닫는다) 다른 전송의 노드를 오류로 취급하지 않는다.
- 교정: `CompositeMissionTransport::ReceiveOne`이 등록표가 살아 있다고 인정한(`Owns`) 다른 전송의 노드는 건너뛰고(그 전송의 Poll이 처리), 등록표에도 없는 별칭만 오류로 남긴다. 오류 메시지에 슬롯·토큰·역할·backend 구성·등록표 토큰 목록을 덧붙여 원인 추적에 쓰게 했다(M7 로그 구조).
- 확인: 교정 뒤 Test Battle 진입·300프레임 대기·`Return to Editing` 복귀가 모두 통과했고(로그 없음), Tutorial4 생산/건설 요청/600프레임 대기도 그대로 통과한다.

## 3. 한계

- `CompositeMissionTransport`를 직접 만드는 단위 시험이 저장소에 없다(생성에 Client 전체 장치가 필요). 이 교정은 실제 Test Battle 실행으로만 확인했다. 별도 단위 시험은 M6 정리 때 장치 가짜를 만들어 추가한다.
- Test Battle 중 pending -1의 Editor 노드는 Editor로 복귀한 뒤 Editor 쪽 Poll이 정리한다(원문과 같은 지연 정리).
- 승리/패배/포획/희생 등 전투 결과 재현은 하지 않았다.
