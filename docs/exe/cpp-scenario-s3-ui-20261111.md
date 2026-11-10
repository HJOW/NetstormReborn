# S3: 시나리오 전용 편집 UI의 실제 화면 왕복 확인 (제품 1282 유지, 2026-11-11)

마지막 디컴파일 PC 호스트명: HJOW-Athlon(변경 없음). 이 단위는 S1이 병합한 편집 UI(`UberScenario.inc`, `EditorScenarioController`)를 실제 Editor에서 끝까지 조작한 확인이며 제품 소스 변경은 없다. 격리 사용자 폴더·복사본 게임 폴더(1,430항목 배포본을 푼 것)로 실행했고 종료 코드 0·오류 로그 없음·게임 폴더 불변이다.

## 1. 확인한 흐름

1. 메인 → `Edit`(432맵, 50행 페이지) → `TEST01` → 편집 화면 → `ESC`(메뉴 막대) → `Edit` → `Scenario...` : 패널 표시.
2. 패널 첫 화면: `Player start`, `General`, `Briefing`, `File`(저장 대상 경로 표시), `AI common`, `Player 1`, `AI 2`~`AI 8`, `Save`, `Discard`(영어/한국어 병기).
3. **Player start**: `myStartMoney = 50000`, `myTech = all`, `techAllowed =` 표시 → `myStartMoney` 선택 → 실제 TextEntry 입력창(제목 `myStartMoney / 빈 AI 값은 상속`, 초기값은 현재 원문 값) → 백스페이스·`7777` 입력 → `OK` → 목록이 `myStartMoney = 7777`로 갱신.
4. `Back` → `Save`: 사용자 작업 폴더 `CustomMaps/D/TEST01.english`에 저장. **수정한 줄만 `myStartMoney="7777"`로 바뀌고** 헤더 순서·빈 줄·AI 설정·브리핑·`[Succeeded]`/`[Failed]` 구역 등 나머지 바이트는 원문 그대로다(보존형 문서). 원본 `originals/d/TEST01.english`는 건드리지 않는다.
5. `Game` → `Test Battle`: 브리핑 뒤 전투 HUD의 SP가 **7777**로 시작한다(편집값이 실제 미션 설정에 반영됨). 원본 대본은 50000.
6. **General**(title, missionType, missionNumber, tutorialNumber, moreGeysers, denySalvage, denyAscend, allowAnyCapture, randGeysers), **AI common**(aiName…aiCollectors 14필드, "공통값은 모든 AI에 먼저 적용됩니다"), **AI 2**(실효값과 `[상속]` 표식: Name/StartMoney/Tech/Off/Color/AllyList/Enemy/NoTemple/Ability/BridgeDrawRate/StuffRefreshRate/TimeBetweenMoves/GeyserAttachments/MaxInHand/Collectors) 페이지가 모두 표시된다.
7. **AI 2 Name**을 한글 `바람의 중위`(MCP `text`, IME 확정과 같은 경로)로 바꿔 저장하면 파일에 `ai2Name="바람의 중위"`가 **UTF-8 바이트**로 기록되고 원문의 다른 줄은 그대로다.
8. `ESC`는 패널 전체를 닫고 보류 편집을 버린다(`Discard`와 같음). 메뉴 막대는 `ESC`를 한 번 더 눌러야 다시 나온다.

## 2. 관찰한 UX 한계(결함으로 보지 않음)

- 편집 입력창은 현재 값을 미리 채우고 커서를 끝에 둔다. 새 값을 쓰려면 먼저 지워야 한다(원문 TextEntry의 일반 동작).
- 매우 큰 `myStartMoney`(예: 500007777)는 모델이 정수 파싱을 그대로 받아들이며, 전투 HUD는 그 값을 표시하지 못하고 다른 값(16600)을 보였다. 원문의 float SP/표시 폭 한계로 보이며 별도 상한 검증은 추가하지 않았다. 필요하면 B/후속 단위에서 상한을 정한다.
- `Ability`는 원문 플래그 정수(`62975`)로 표시된다(원문 설정 형식을 그대로 유지).

## 3. 남은 것

- 위 확인은 TEST01 한 맵의 일부 필드(Player start 편집, AI 2 이름 편집)와 모든 페이지 표시다. 모든 필드/AI 8개를 일일이 편집하지는 않았다(모델 단위 시험 20건이 필드별 검증을 맡는다).
- S4: 실제 사용자 홈 경로와 읽기 전용 설치 위치에서의 저장·불러오기·Editor/Test Battle 진입.
