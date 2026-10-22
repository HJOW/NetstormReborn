# AI 다리 시작점과 실제 열린 연결점 복원

현재/마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-22**다. 앞선 검색 복원 커밋은 `de78dbaa / 20261022-40 CP`다. 이번에는 실제 `00410b10`과 `0040faa0`의 명령·현재 호스트 정밀 결과를 대조했다. 보호 원본/CD·AGENTS/dotnetpj·기존 원본 fixture를 변경하지 않았고 게임/창·새 전체 디컴파일을 실행하지 않았다.

`RawAiBridgeCandidates::Start`는 같은 Brain+298 graph/+260 owner·현재 surface/spot/TYPE/프레임·기존 현재 맵 경계를 참조한다. 목표XY와 왼쪽/위 경계를 원본 signed64 CRT의 낮은DWORD로 절삭한다. 맵을 X 바깥/안쪽Y 순서로 훑으며 정수좌표와 오른쪽/아래float 경계를 strict 비교한다. 경계를 임의로 정수 inclusive 범위나 지도 안으로 바꾸지 않는다. 후보가 물리 지도0..255 밖이면 surface0처럼 건너뛴다.

현재 surface의 graph/owner가 전체 본체DWORD와 일치하고 네 방향 중 빈 spot이 있는지 확인한다. 목표와의 signed Manhattan거리가 현재 최저점보다 작을 때만 `Open`을 실행한다. `Open`은 현재 프레임의 A~P 원본 연결 비트와 전체DWORD bridge TYPE 구분을 사용해 북/동/남/서 순서의 빈 연결점을 찾는다. 다리는 켜진 연결, 육지는 꺼진 연결이다. 첫 합법 지면SID를 반환하고 없으면0이다. 거리 동률에서는 앞서 찾은 X/Y를 유지한다. 시작점 전체 부재는 두float `-10000`이며 본체/타이머를 수정하지 않는다.

새 도구 `tools/decomp_ai_bridge_start.py`는 원본 시작점·열린 연결·graph·현재코드·TYPE·CRT 명령만 격리 실행했다. 지도4종×다리/육지×A~P 코드×목표3종384장면, graph/owner/bridge 전체DWORD 비교5장면, 분수·빈 사각형·NaN·지도 밖 경계7장면, 목표절삭 경계3장면으로 **399개**다. 각 장면을 x87 `0x27f`/`0x37f`에서 실행한 **798정상 반환·38,446칸**이 동일했다. ret12·결과포인터·비휘발 레지스터·제어워드 보존과 전체0xb08 본체/지도/원본 슬롯 불변도 확인했다. 계산 스텁은 없다. [원본 출처](../../cpppj/ai-bridge-start-20261022.json)의 보호 PE/도구/fixture SHA4개를 다시 확인했다.

C++의399장면 출력 전체8BYTE와 본체 불변이 통과했다. 실제 Start를 `RawAiBridgePath::Advance`에 연결한 조립 검사는 가장 가까운52,64를 선택하고 시작점 부재/기존timeout9 보존도 확인했다. Release는 **첫 빌드 종료0·경고0·오류0**, 다리 관련 **24개·실패check0**다. `extracted/cpp-ai-bridge-start-20261022-{build,tests}.log`를 보존한다. 전체 CTest는 이 단위에서 반복하지 않았으므로 최신 전체는 직전 단위315의 **2,226개·실패0**이며 이번 새2개를 포함한 전체 통과로 합산하지 않는다.

다음은 실제 Session에 Start/Initialize/Prepare/Search/Step/Extend를 연결하는 일이다. 배치에는 기존 `aiCollectors->DeployWithNotice`, 현재 기술부모에는 실제 Player `Technology`, 소비에는 실제 world 삭제, 보충에는 `aiStrategySetup->RefreshBridges`를 사용해야 한다. 같은 월드·공용 난수·로그·현재 시각과 TYPE를 공유하며 별도 성공값이나 재고/HP/SP 보정을 넣지 않는다. 검색의 미초기화 caller 임시XY 입력도 별도 계약으로 명시한다. 네 전략·전체 자동AI프레임/종료,15개 미션·Edit/custom Fort·언어별 시나리오·자연 플레이·표시/음향 동일성은 여전히 미완료다.
