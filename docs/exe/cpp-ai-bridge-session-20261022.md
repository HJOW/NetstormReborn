# 실제 Session의 AI 다리 탐색·배치·소비 연결

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**다. 시작 HEAD는 `55ef3103 / 20261022-41 CP`였고 작업 트리는 깨끗했다. AGENTS와 공통/dotnet 인수인계를 다시 읽었다. 직전 턴은 검색/시작점 구현·원본 대조·전체 회귀·커밋 두 건을 완료한 진행 턴이다.

`RawCampaignSession::AdvanceAiBridge(owner,target,endpointScratch)`는 같은 살아 있는 Brain의 `004174c0`을 명시 호출한다. 설치 전/owner범위 밖을 거부하고, 본체 부재는false이며 본체가 있으면 실제 시작점/연장/timeout 반환값을 그대로 반환한다. 검색 임시XY는 원본 caller 미초기화 입력에 대응하는 명시적 입력/출력이다. 이를 AI 힙 할당 입력으로 대체하거나 별도 성공점/재고/HP/SP를 만들지 않는다.

Session의 실제 `RawAiBridgeCandidates`/`RawAiBridgeSearch`는 현재 풀·188TYPE/프레임·hash0·spot·bridge/fencemark/윈드 아처 TYPE·공용 난수를 공유한다. 한 호출의 경로 객체는 같은 메모리를 참조하고 하위 효과를 다음과 같이 연결한다.

- 시작점/초기화/준비/검색은 앞서 원본 대조한 실제 모듈이다. 시작점은 현재 맵 경계10,10,246,246를 참조한다.
- 다리 배치는 `aiCollectors->DeployWithNotice`로 실제 일반 배치→생각 기한/로그→로컬 드롭 음향을 처리한다.
- 디버그 표식은 실제 `aiDeployment->Place`의 argument0/방향0이다. 현재 다리 로그 초기값99와 시작/끝/목표 TYPE113/119/117은 보호 PE의0052f3cc/0054121c/234/22c에서 확인했다.
- 기술부모는 현재 Player `Technology`를 매번 다시 조회하고, 조각 소비는 실제 `world.Destroy`, 보충은 `aiStrategySetup->RefreshBridges`다. 배치→현재 첫 일치 조각 소비→현재 재고 재조회/보충의 원본 순서를 유지한다.
- timeout 로그는 기존 현재 AI 장치로 전달한다. 전체 자동 전략 프레임은 이 명시 호출 관문과 구별한다.

새 실제 Session 검사는 `thewarbegins`의 **실제 로컬 로그인→AI 생성→초기 전략→준비**로 재고4개를 만들고, 실제 상대 사제 좌표를 목표로 삼았다. 목표나 결과에 맞춰 본체의 진행/승패/지갑/HP를 수정하지 않았다. 실제 호출 **14회** 동안 검색의 예산 중단/재개를 거쳐 패턴 **22**, 회전 **6**, 배치/새 끝점 **(124,112)**를 선택했다. 기술부모의 실제 조각 체인에서 선택 패턴이 제거되고 재고가 **4→3**이 됐다. 같은 지갑/SP를 유지했고 원본 배치/건설을 거쳐 owner2의 실제 hash0 다리 칸이 **15→21**로 늘었다. 로컬 호스트 권한의 Request→Confirm은 서버 월드에 즉시 적용된다. 설치 전/owner0 거부·AI 없음/삭제 뒤false도 확인했다.

Release는 **첫 빌드 종료0·경고0·오류0**이며 새 실제 Session 검사 **1개·실패check0**다. 원본 출력은 `extracted/cpp-ai-bridge-session-20261022-tests.log`, UTF-8 보존본은 `extracted/cpp-ai-bridge-session-20261022-tests-utf8.log`다. 큰 공용 Session 헤더 변경으로 관련 제품/검사 파일을 다시 컴파일했다. 이후 전체 CTest는 **내부2,229개·실패check0·종료0·303.69초**로 통과했다. 새 시작점2개와 실제 Session1개도 포함됐다. 같은 실행 핸들을 끝까지 관찰했고 관측 대기로 새 검사를 중복 시작하지 않았다. `extracted/cpp-ai-bridge-session-20261022-{build,tests,tests-utf8,ctest,all-tests}.log`와 [회귀 보고서](../../cpppj/ai-bridge-session-regression-20261022.json)에 실제 결과/보호 PE·제품 소스/로그 SHA를 보존했다.

다음은 `00417bd0`의 네 다리 전략과 `00417c70` 전체 행동 순서다. 정밀 함수00417bd0의 실제 본체는4175b0부터의 case 영역과417bd0 입구를 모두 포함한다. 선행 미복원 하위는 다음과 같다.

- `00410d40`:같은 graph의 간헐천 가장자리12칸에서 자기 다리의 J/K 방향 연결을 찾는다.
- `00411310`:yetToConnect count0이면 현재 시각+40초,그외에는+120초로5d0 기한을 쓴다. 두 QWORD 상수005013b8/b0을 보호 PE에서 확인했다.
- `00411470`:caller 사각형에서20번 y→x 공용 난수를 뽑아 같은 graph/owner의 열린 지면을 선택한다.
- `004119b0`:inclusive 사각형의 모든 spot BYTE가0인지 X 바깥/안쪽Y 순서로 확인한다.

이후 기존 실제 준비→목표선택→수집자/시설→부족좌표→사제→건물배치→다리전략→운반자→경제 순서와 `00417d30/00417de0` 자동 프레임·삭제 수명을 연결해야 한다. 15개 미션·Edit/custom Fort·언어별 시나리오·자연 플레이·표시/음향 동일성은 계속 미완료다. 보호 원본/CD·AGENTS/dotnetpj·기존 원본 fixture 변경이나 새 전체 디컴파일·게임/창 실행은 없다.
