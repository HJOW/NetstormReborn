# dotnetpj 0.521 — 일반/사제 후보 충돌과 실제 finder 조립

2026-11-14, HJOW-Athlon. cpppj 기준 커밋은 `a127fb5cc`다. `RawCanonPlacementCollision.h/.cpp`, `RawPriestPlacementCollision.h/.cpp` 전체와 원본 `PlacementIgnoreValue`를 이식했다. 기존 디컴파일 산출물은 읽거나 변경하지 않았다.

## 변경

일반 충돌은 현재 배치/후보TYPE→후보 좌표 저장→현재 ClientSID 관문→현재 무시 마스크/helper→로컬 발자국 표시→현재extra/mode 순서를 유지한다. 무시helper는 조기 분기와 전체DWORD 반환을 보존한다. PATCH의`0x218000`, CD의`0x210000` 마스크와 bridge 추가 비트를 구별하며 bool로 축소하지 않는다. 역할/마스크는 독립 전역 또는 실제 장기 owner의 현재 값을 후보마다 읽는다.

후보 발자국 표시는 y/x를 늘리며 float 중간 저장 없이 넓은 차를 절삭한다. 표시 뒤 거부도 배열 효과를 남긴다. 실제 finder는buried를 제외하고 기본flag0/필터로 탐색한다. 첫 거부 뒤 지형/Next를 생략하며, 거부를 무시하고 재scan/성공을 요청하는 상위 정책도 첫 거부를 덮지 못한다. 무시한 후보는 지형 효과를 받은 뒤 저장한finder.next로 진행한다. 다음 후보는 현재 전역·TYPE를 다시 읽는다.

사제 wrapper는 사각형/각 후보 진입에서 현재genus와 유한 요청을 검사한다. 일반/사제는 같은 `CanonPlacementCollisionState`, `CanonPlacementPreviewState`를 사용한다(C++ alias의 현재 상태 공유). 함수 객체는 복사하고 자료/실제 모듈은 대여한다. 미리보기/실제 기하 어댑터는 같은 실제 풀만 허용하고 다른 함수 사본은 유지한다. 일반 기하의 지역 정책은 원래 요청마다 새로 만들어 실제 패턴/방향과finder 범위를 연결한다.

## 검증

원본 세판본34200입력을 읽기 전용으로 공유했다. SHA-256은LF 정규화 값이다.

| fixture | 입력 | SHA-256 |
| --- | ---: | --- |
| canoncollision-x86.tsv | 22680 | c83e9d19f380b110a18c04a42029fe4b8877a936d71b51d46d3996bc8ece582f |
| priestcollision-x86.tsv | 11520 | a3876d2cc444a5a0a258c598aca8c3376bd808357eca78680afdc3b4ed8bd403 |

기대값을 C#으로 생성하지 않고 원본 전체 무시 반환·한 후보 반환·144BYTE 배열·raw 불변성을 대조했다. CD raw36BYTE와 C# 나머지14BYTE 입력 보존을 각각 확인했다. 새19검사는 다음을 포함하며, 원본 입력 대조는 처음부터 전부 일치했다.

- 실제 finder의buried 제외·첫 거부 뒤Next/지형/재scan 중단, 일반/사제×두판본.
- 무시한 첫 후보의 지형 효과가raw.next를 지워도 실제finder의 저장한next로 진행하고 다음 후보에 현재 공유 마스크를 적용하는 두 경로/판본.
- 실제 받침 패턴→기하→미리보기→접두→finder/충돌의 두 번째 모양 거부. 사건은begin/shape/terrain/end/shape이며최종 관계는 생략, 차단36칸이다.
- 실제 사제 픽셀 getter→접두→기하→미리보기→기존 사제 지형→충돌/finder→나선→서버 공장→실제 사제/공통Owner→현재Carrier를 같은 풀로 합성. 첫 후보(21,22)를 거부하고 둘째(21,21)에SID15001을 생성했다. WORD0x8181·owner1·Pop/사제 알림 각1·차단16칸·지면 미완성 상태가 일치한다. Pop은 이번 합성의 기록 경계다.
- 공유 역할/전체DWORD 마스크의 현재 값과 선택 조회, 현재 후보TYPE가 ClientSID 관문보다 먼저 검사됨, 비로컬NaN 후보의 표시 생략, 누락 자료/정책·다른풀/요청 진단, 함수 사본.

추가 계약 검사2개에서 Client 풀에 기본Server 할당을 요청해 실패했다. 입력을명시적`SidAllocation.Client`로 교정했고 제품/golden은 변경하지 않았다. 교정 뒤19/19통과했다. PATCH의무시 마스크를CD값으로 바꾼 변이는19검사 중3개가 검출했다. 제품 소스는 SHA-256 `322648bc7e5716473bbd0be0c91d2e33269b5776d1735117b67169b6a7c85665`까지 바이트 원복했고 전체 비증분 Release빌드0경고/0오류를 확인했다. 원복 뒤 최종관련Core429/429·Client21/21도 통과했다(실패0·건너뜀0·각종료0). 전체Core/Client회귀 완주로 합산하지 않는다. 로컬 근거는 `extracted/dotnet-placement-collision-0521/`의 TRX·변이 스크립트/결과다(Git 제외).

PORT_MAP은788행(L1 376+port412), 공유fixture271개다. 이전 사제충돌 헤더의 함수 형식 부분 행을 본체로 갱신했다. 기존 기하 파일의 함수 형식 선언은 재사용하고 설명만 현행 공유 관계로 바꿨다. 이번 모듈에 해당하는 새10차 NB 항목은 없다.

## 보호와 잔여

실제 게임/MP 실행 없이 부품/합성 검사만 했다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현은 생략했다. originals/originalCD·AGENTS.md·README 개요·cpppj·webpj를 변경하지 않았고 외부 접속 코드를 추가하지 않았다. 일반 지역 정책/전체 사제 월드·HostSession과 다른 파생별 후처리는 잔여다. 사제 배치의 조립 부품은 갖췄으나 캠페인 호스트에 연결한 것으로 세지 않는다. M1 전체일치0/34와 새 경로 플레이 불가 상태는 그대로다.
