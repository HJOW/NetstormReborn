# 실제 플레이 검사 설정 복구

2026-10-22 작업 단위302. 시작 HEAD는 `da3a5c8a`다. 현재 PC 및 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**다. 단위301의 전체 CTest에서 발견한 실패 중 검사 설정 세 곳을 수정했다. 제품 코드와 기존 독립 원본 fixture는 변경하지 않았다.

미션 First/F8의 큐 용량 검사는 사건100~138을 넣어 반환 코드가 모두 같아야 한다고 요구했다. 이 범위에는 DEL127이 있다. 원본 `00452f00..00452f81` 명령은 마스크 후127을 `Ctrl+Backspace`로 바꾼다(`00452f32/00452f3e/00452f44`). 따라서 순서·39개 용량을 검사하는 입력을 일반 문자1000~1038로 옮겼다. 별도 기존 `InputQueue_ReleaseFilterDeleteAndCommand_MatchOriginal`은 DEL 변환을 계속 검사한다. 큐 구현과 변환 기대값은 바꾸지 않았다.

Player Regular의 native fixture는 빈 State 생성 함수 진입에서 관측을 멈춘다. 후속 구현으로 미연결 진단 문구가 바뀌었지만 재생기의 catch는 예전 문구만 인식해 전체 재생을 예외 종료했다. 현재 문구도 같은 기존 원본 경계 주소로 기록하도록 했다. 반환·State28바이트·헤더·기한·진단·부수효과의 기대값은 그대로이며 10.78/CD10.72 각329행을 다시 비교했다.

Session AI 검사는 로컬 로그인 연결을 선택하지 않고 독립 외부 서비스를 공급한다. 그런데 공용 fixture에 현재 필요한 `participation.sendAlliance` 경계가 없어 로컬 AI 초기화부터 예외가 났다. 외부 송신 요청의 서버/owner/mask 세 인자를 저장하는 관찰자를 공급했다. 동맹 표·응답·플레이 상태를 이 관찰자가 대신 변경하지 않는다.

Release 빌드는 종료0·경고0·오류0이다. 큐 관련4개, Player Regular3개가 통과했다. Session AI23개 중19개가 통과하고 다음4개는 실제 후속 조사 대상으로 남았다.

- `CampaignSession_AiCollectorSelectionUsesActualDeployment`: 수집자 추가 목록 기대값.
- `CampaignSession_AiFactoryProductionUsesOriginalWorkshops`: 원격 워크샾 개수 기대값.
- `CampaignSession_AiDropTargetsUseActualContentsGunAndLifetime`: 장애물 후보 수집 경계/용량 오류.
- `CampaignSession_AiDropScanUsesActualMapsStateAndLifetime`: 스캔 진행 기대값.

이번에는 전체 CTest를 반복하지 않았다. 단위301에서 전체2,212개·실패check72개를 관측했고, 위 focused 결과로 전체 통과 수를 추정하지 않는다. 다음에는 이 네 검사와 미션 Guide/이동 등 나머지 전체 실패를 원본 동작과 대조한다. 다리 후보/경로 탐색·자동 AI 프레임/종료·15개 미션·Edit/custom Fort 및 자연 플레이/동일성 목표도 계속 남아 있다. 인수인계 후 `20261022-27 CP`로 로컬 커밋하며 원격 push하지 않는다.
