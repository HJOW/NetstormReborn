# 미션 로딩 돌 창 실제 연결 재개 (단위373)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**으로 일치한다. 기존 10.78 정밀 소스와 보호 실행 파일을 읽었으며 새 전체 디컴파일이나 디컴파일 소스 변경은 없다. 전체 Early Missions·캠페인·데모·Edit·HUD 목표 중 미션 로딩 창의 실패를 해결한 단위다.

## 수정한 동작

- `RawMenuControls::LoadingMetadata`는 원본 `004c83b0 → 004c7c60`처럼 새 돌 버튼을 먼저 부모의 (0,0)에 부착한다. 이후 배치의 Push/재부착이 유효한 트리를 대상으로 실행된다.
- `LoadingBody`는 `004c7aa0`처럼 새 ScrollingText를 부모에 먼저 붙이고, 배치 시 분리한 뒤 최종 위치에 부착한다. 원본 본문 **+34는 secondaryDepth**다. 이를 alignFlags(+8)에 쓰던 오류를 수정해 부모 기준 깊이 -1을 유지한다. 기본 Gump의 void·정렬 검사는 약화하지 않았다.
- 미션 해제 시 실제 로딩 부모를 Kill하며, 비 Tutorial의 `LoadMissionWorld` 예약도 소유자와 함께 취소한다. 현재 보호 TEST01/TEST02는 Tutorial이므로 비 Tutorial 취소 분기의 GUI 증거로 세지 않는다.
- 실제 등록표의 부모 serial·본문/버튼 토큰·부모 직사각형을 `raw_mission_loading` 진단으로 읽는다. 일반 프레임·Cancel·다음 장면에서 부모가 남는지 같은 저장소로 확인한다.
- `RawMenuControls_MissionLoadingUsesSharedTreeAndCancel`에 실제 자식의 초기 부착·상대 깊이·정렬 플래그와 최종 부모 반납 검사를 추가했다.

## 원본 픽셀 관측

분석기를 Release로 빌드했다(경고/오류 0). 원본 전체 사본 세션 **20261023T122824095Z-d1a6d7b16649**에서 메인 메뉴 → Campaign → Early Missions → 1 Bridge the Gap을 실제 입력했다. 원본 로딩 화면의 증거는 다음 파일이다.

`extracted/analyzeManager/20261023T122824095Z-d1a6d7b16649/screens/6d55f55d2cdc1a8b8c8ad261505cc95946ce58a925a48dc872d3338fcf0f1893.png`

최종 빌드의 `extracted/cpp-mission-loading-20261023-final-pixels/summary.json`은 실제 로딩 부모 **[354,344,754,424), 400×80**의 RGB 차이 **0픽셀**을 기록한다. 원본 전체 화면과 같다는 판정은 아니다. 클론의 왼쪽 HUD는 아직 같은 생성·dock 배치를 연결하지 않았다.

원본은 로딩 후 정상적으로 미션 제목/장면에 도달했고 `end_session(force=false)`는 `closed=true`로 종료했다. 원본 전체 파일의 전후 SHA는 동일하다. 자동 녹화는 종료 경계에서 ‘게임의 표시 가능한 창을 찾지 못했습니다’로 failed를 기록했다(5,048프레임). 이 기록은 보존하며 녹화 전체 정상 마감으로 보고하지 않는다. 위 PNG는 별도의 정상 `game_input` 캡처 증거다.

## 실제 클론 검사

`tools/cpp_mission_loading_smoke.py`는 새 출력 경로에서만 입력·원시 보고서·픽셀·실행 당시 바이너리 SHA를 보존한다. 게임의 시계/성공 상태를 직접 쓰지 않는다. 로딩 부모 존재, 후속 반납, 실제 월드 진입 또는 Cancel 후 미션 없는 메인 메뉴 복귀를 검사한다. `--demo`는 실제 메인 Demo 메뉴와 원본 라벨을 누른다. `--reference`는 같은 크기의 원본 client PNG와 실제 부모 직사각형을 오차 없이 대조한다.

최종 제품 SHA-256은 `2a8eeaa40d586226bcd14a2fad1edce6f844b6710ef7bb9946d728d785c10dac`다. 최종 Release build3은 종료0/관측 경고·오류0, 관련 MissionLoading 4개와 RawMenuControls 10개는 모두 통과한다(한 검사가 양 필터에 포함되어 고유 13개).

최종 실제 관측은 `extracted/cpp-mission-loading-20261023-final-*`에 있다. Early 첫 미션/Cancel·돌 창 픽셀, Demo 메뉴의 세 항목 진입, Guard My Back 데모 로딩 중 Cancel을 통과했다. 앞선 build2로 캠페인 1-1/TEST01 진입·TEST01 Cancel도 관측했다. 모든 실행의 원본 전체 해시가 같았다.

새 도구의 최초 probe 두 건은 금지된 frame0 계획으로 실패했다. 제품을 실행하기 전 CLI가 거부했으며 양 원본 해시는 동일했다. 첫 Cancel probe는 아직 미션 명령이 실행되기 전인 frame1에 Cancel을 요청해 조작부 없음으로 실패했다. 실제 첫 로딩 화면 다음 입력(frame2)에서 성공한다. 실패 기록을 삭제하거나 제품에 최소 대기시간을 추가하지 않았다.

## 남은 경계와 다음 작업

- 전체 CTest는 종료0, **2,420검사·실패check0·483.87초**다(CTest 전체483.93초). LastTest 내부 결과도 확인했다. 이후 제품/검사 C++ 변경은 없다.
- 기존 `cpp_goal_stage_entry_smoke.py`는 현재 Guide 복원 전의 sequence=1, 캠페인 frame4/6 즉시 재개를 기대하여 11개를 실패로 보고했다. 실제 원문 계약 검사의 현재 sequence는 Init1 → 설치2 → Start3 → First4이며, 안내가 닫힌 뒤 일곱 번의 프레임 관문도 있다. 제품을 옛 기대에 맞추지 말고 이를 반영한 실제 진입 검사를 별도 단위로 고친다. `DemoStormRages`는 원본 Header 자체가 demo=1이다. 이름만 보고 MissionBegin의 demo0을 고정 기대해서는 안 된다.
- `decomp_mission_loading_message.py --verify`는 현재 PC에 이전 PC의 extracted JSON 보고서가 없어 실패했다. 커밋된 1,540행 fixture는 C++ `MissionLoadingMessage_Native1540`에서 통과했다. 이번 실행을 원문 생성기 재실행/해시 검증 성공으로 쓰지 않는다.
- 현재 로딩 경로는 기본 Starting Mission/Cancel과 짧은 본문만 연결한다. 일반 HTML 부모의 전체 메타·긴 본문 Fit와 현재 HUD dock 지도 사각형 연결은 남는다.
- HUD 6개 준비 모듈 및 미니맵 blit의 공식 등록·실제 연결, 일반 대화·Edit 세션, 전체 스테이지의 사용자 완주·승패·저장/불러오기·원본 전체 동일성은 계속 미완료다.
