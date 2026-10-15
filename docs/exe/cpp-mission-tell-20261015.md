# 실제 미션 진행 중 현재 Tell 섹션 조회

2026-10-15, **VM-W11-Codex**. 현재/마지막 정밀 디컴파일 PC도 같은 호스트다. 원본 `004c2a90`은 현재 단계 문자에 `.`를 붙인 뒤 `00482de0→004cf960→004cf730`으로 전달한다. `004cf730`은 명시 설정 객체의 섹션을 먼저 찾고, 찾지 못하면 현재 Mission+5c 설정에서 **전체 target**을 조회한다. Briefing/phase 제한은 없다. 이번에는 기존 정밀 C를 읽었으며 새 디컴파일이나 원본 게임 실행은 없다.

기존 `UberGump::Tell`은 현재 미션 섹션을 Briefing에서만 조회하여 실제 Mission에서 B./C. 안내를 받으면 일반 tell.english에서만 찾다가 오류가 났다. phase 제한을 제거했고 명시 섹션 조회가 실패한 경우에도 현재 미션의 전체 target을 찾는다. 현재 미션에 없는 공통 `PriestImmobile` 등의 안내는 기존 tell.english 조회로 이어진다.

원본의 선택적 사건 실행 반환을 연결할 `TryTell`도 분리했다. 섹션이 없으면 false를 반환하고 현재 페이지·정지를 바꾸지 않는다. 명시 명령의 `Tell`은 같은 해석을 호출하되 잘못된 이름의 기존 오류를 유지한다. 실제 승패/단계 자동 호출은 아직 연결하지 않았다.

Release `NetstormCpp` 빌드는 경고/오류0이다. `extracted/check_mission_tell.py`는 실제 Early1/2 최초 MORE/OK 입력으로 미션에 들어간 뒤 실제 Kernel에서 B./C.를 요청했다. **21상태**에서 B./C./B1./C1. 표시, MORE/BACK/OK의 현재 명령, 미션 복귀 및 공통 Warning의 비모달 fallback을 확인했다. `tools/check_dialog_controls.py`의 기존 **95상태**도 모두 통과했다. 자료는 `extracted/mission-tell/`과 `extracted/mission-tell-controls-regression.log`다.

Early1 B.의 실제 화면을 직접 확인했다. 현재 섹션과 버튼은 정상 표시되며 전체 HTML/픽셀 동일성을 새로 검증한 것은 아니다. 특히 기존 본문 renderer가 inline italic 구간을 별도 줄로 나누는 현상도 관찰하여 후속으로 기록한다. 두 클론 실행 묶음은 허용 설정을 보관·복구했으며 originals 전체 파일명/해시가 동일하다. 원본 게임·음향 장치는 실행하지 않았다. 원본 DirectDraw 전체화면 오류와 dotnetpj는 변경하지 않았다.
