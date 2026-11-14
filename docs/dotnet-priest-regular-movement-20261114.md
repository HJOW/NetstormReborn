# dotnetpj 0.522 — 사제 정기 분배와 현재 이동 프로세스 선택

2026-11-14, HJOW-Athlon. cpppj 기준 커밋은 `30523aaf2`다. 기존 디컴파일 산출물은 읽거나 변경하지 않았다.

## 구현

`RawPriestRegular.h/.cpp` 전체를 Core에 이식했다. 601은count0일때3초를 반환하고 비영일때만 실제 삭제를flags0x100000으로 호출한뒤-1을 반환한다. count는부호 없는 원문 비트로 읽는다. 602~605는고유후단, 나머지는직접Carrier 부모를 선택한다. 선택하지 않은 경계는 요구하거나 읽지 않는다. 고유 회복/추락/의식/기도 몸체는 이 분배기에서 재구현하지 않는다.

`RawPriestWorldBindings.h/.cpp`에서는 이동 선택 자료와 선택 함수만 Client에 먼저 이식했다. 사제 월드 전체는 아직 잔여라 두 행은`port`다. 실제 장기 owner에 신전 제품133·고갈된187·RainWalker156·벌프135·Dude프로세스24 상자를 추가했다. 기존 사제158상자와 함께 같은 현재 주소를 빌리며 구성·일반 월드 준비에서 값을 덮어쓰지 않는다. 명시 복사 생성자는 독립 주소를 만들고 값만 복사한다. `Assign`은목적지 주소를 유지하며 원문tuple의17항목 순서로 읽고 쓴다. 교차 별칭에서 중간 쓰기가 뒤의 읽기에 보이는 동작도 보존했다.

이동 선택은CD의sunWalker 특수 반환을 타입 표 조회 전에 수행한다. PATCH는타입을 먼저 확인하고sunWalker/고갈된을 처리한다. Walker→balloon→텔레포트→벌프→Dude→사제→마나볼트 순서를 유지한다. PATCH의나머지는웜, CD의미지원 타입은원문assert 대응 오류다. 현재TYPE와프로세스 번호는전체DWORD를 유지한다.

## 원본 대조

세fixture를 cpppj 원문에 읽기 전용으로 연결했다. 기대값을 C#으로 만들지 않는다. SHA-256은LF 정규화 기준이다.

| fixture | 입력 | SHA-256 |
| --- | ---: | --- |
| priest-regular-dispatch-x86.tsv | 384 | 354a610312baf7ca3b6179ac98527d5625c48585629ad305eb0ae3e179a8eb7d |
| pathprocesstype-x86.tsv | 718 | 4a5da6130b1c840c4a6387efc64671f535c46251ccc529510f5386d0e9bcbb6f |
| priestworldevents-x86.tsv | 2872 | 4556993d777756bfe17a4c0c0eae30311d1f6ffbdcafc9101be0b2cf5ed6df54 |

합계3974입력이다. 정기는반환float 비트·삭제 flags·전체 부모 인자·고유 후단 선택을 비교한다. 이동은각판본 전체TYPE×balloon과 사제/Walker별칭·프로세스 번호 이동을 대조한다. 초기 정기원본384/이동3590입력은전부 일치했다. 이동현재상자/복사/대입/조회 순서·전체DWORD 계약7검사는처음부터 통과했다.

정기 실제Kernel 검사의 최초 입력은Form0에host.Kill을 요청해실패했다. 입력을실제 공장 Form부착/반납으로 보완했다. 이 합성의 자산 삭제 경계는현재Regular의 실제Host.Kill로 연결하며, 부모 사제 전체 삭제로 확대해 주장하지 않는다. 실제 정기Form·Kernel 예약/제거·부모체인 반납 범위를 검사한다. Form반납 확인의 다음 입력에서는void비트4를free로 잘못 기대한1실패가 있었다. 실제 RawSidPool.Release가상태를free1|dead2로 쓰는 근거에 맞춰public FreeBit를검사하도록 교정했다. 제품/golden은변경하지 않았다. 교정 뒤정기3/3·이동7/7통과했다. signed count변이는정기2/3, balloon선행변이는이동6/7검사가검출했다. 원복 SHA-256은정기`53ba4a35422c67376f216d04bdc35b92961077d0f73a5ba8c7b8142b5aab331c`·이동`d7a3a2348aa09a7dccbd16ee3480c9c932ff79bdbda75972dee0702c5efb68f1`이다. 로컬근거는 `extracted/dotnet-priest-regular-movement-0522/`의TRX·변이스크립트/결과다(Git제외). 원복뒤전체비증분Release빌드0경고/0오류·최종관련Core499/499·Client32/32통과(실패0·건너뜀0·각종료0)를확인했다. 전체Core/Client회귀완주로합산하지않는다.

PORT_MAP은792행(L1 378+port414), 공유fixture274개다. 기존RawGameAuxState.h의부분행은현재5역할을추가한근거만 갱신했고 전체헤더완료로 올리지 않았다. 이번 모듈에해당하는 새10차NB 항목은없다.

## 보호와 잔여

실제 게임/MP 실행 없이 부품과 합성만 검사한다. 사제 포획/희생·특정 적 공격·시나리오 승리 재현은생략했다. originals/originalCD·AGENTS.md·README개요·cpppj·webpj는변경하지 않았고 외부 접속 코드를 추가하지 않았다. 실제 사제 월드 묶음/HostSession·일반 지역 정책은잔여다. M1전체일치0/34와 새경로 플레이불가 상태는그대로다.
