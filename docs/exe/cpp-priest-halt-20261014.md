# 사제 실제 Halt와 공통 운반 Halt

현재/마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-14**다. 새 `carrierarrival` 읽기 전용 내보내기 PATCH54/CD59(세션93649 종료0)를 읽고 사제 실제 가상+38과 공통 운반 Halt를 먼저 분리했다. 단위13의 DaisBridge640/기존 생산·검사·보고서는 변경하지 않는다.

## 실제 몸체와 재사용

| 몸체 | 패치10.78 | CD10.72 |
| --- | --- | --- |
| 사제 Halt |00491bb0|0040c950|
| 공통 Carrier Halt |0042a5f0|004e45c0|
| 공통 위임 thunk |0042ab70|004e7910|
| 실제 가상+cc |00426fc0|004e50d0|
| 마지막 기본10 |004ac990|004ac460|

새 `RawPriestHalt`는 같은 `RawCarrierCheck`·`RawPriestState.CurrentHitPoints`·`SquidReward.MaxHitPoints`·`RawSquidFinder`를 재사용한다. 사제 지면 모드에서는 최초 좌표를 보관하고 거의올림 spot6이 없으면 권한에 따라 가상 Unpop0→거의올림한 최초 좌표의 Pop0을 호출해11을 반환한다. 지면이 있더라도 HP<MaxHP/2이며 권한이 있으면 같은 공간 순서 뒤11이다. 공간 콜백이 raw 위치를 바꿔도 원본은 처음 보관한 좌표를 사용한다.

나머지는 실제 +cc→단순 절삭 점 finder→특정 운반 타입 helper를 실행한다. +cc가 0 아니면12다. 패치는 타입156/187 각각 `00429ed0`/`0042a0d0`의 반환12를 검사하고, CD는 타입156의 `004e7980`의 0 아닌 반환을 검사한다. 다른 후보 또는 helper의 다른 반환에서는 검색을 계속하며 실제 기본 몸체만10을 반환한다.

## 원본과 C++ 대조 자료

`decomp_priesthalt_oracle.py`는 **864 정상/assert0**을 두x87에서 실행했으며 결과·사건·부모 전체 raw가 동일하다. 판본당432입력/864정상반환이며 부모 HP0/99/100/200·지면0/6/8·전투/권한·소수/정수경계·Unpop 후 변이와 공통검색 genus·가상낙하 반환·특정후보·하위 반환0/1/10/12를 교차한다. 모든 분기 결과는 실제 PE 몸체에서 얻으며 Python/C++ 기대함수로 계산하지 않는다.

첫20smoke 전체5파일과 미등록 초기864 전체5파일은 각각 `recovery-history/priesthalt-initial-smoke`, `priesthalt-before-cd-input`에 보존한다. CD의 비특정 후보187은 실제 타입 범위 안170으로 준비하도록48입력만 교정했다. `analyze_priesthalt_cpp_binding.py`는 이전 oracle 전체 원문의 정확 역변환과 부모 전체 raw·사건·반환864 전량 동일를 감사한다. 초기20/미등록864/이전 CarrierCheck·HP·finder 관찰은 최종 새864에 합산하지 않는다.

다음 입력 감사에서 기존 Finder의 지면 주소0x11320000이 hash1과 겹쳐 지면 초기화가 특정 운반 후보를 지우는 구성 오류를 확인했다. 당시 Native/fixture/oracle/C++ test/binding/생산/본문/문서 **전체35파일**을 `recovery-history/priesthalt-before-spots-isolation`에 보존하고, 지면만 **0x11850000**에 분리했다. 같은864입력/부모 전체 raw는 전량 동일이며, 복구된 특정후보 요청120행과 반환50행만 바뀐다. 현재 감사는 `analyze_priesthalt_spots_successor.py`/`recovery-priesthalt-spots-successor-evidence.json`으로 이전 oracle의 네 조각만 정확 역변환하고 초기 binding 원문도 역사 경로에서 재현한다. 최종 Native864만 한 번 합산하며 새 추가입력0이다.

`PriestHaltTests.cpp`의 **새1검사**는 같은 실제 풀/해시/타입/spot/HP 모드/지면조회/일반 finder를 연결하고 반환·명시효과·전체 부모 raw를 그대로 대조한다. fixture macro는 **NETSTORM_PRIESTHALT_FIXTURE**, prefix는 **rawPriestHalt_**다. C++ test/생산 몸체는 입력 지면 교정 전후 불변이다. 현재 Native/successor 전체SHA 감사와 단위14 공유 C++ 검사 **1검사/864전수/실패0**이 통과했다. 실제 같은 Host 사제 Arrival 소비자는 별도 입력 교정 후 재실행을 기다린다.

## 남은 경계

원본 가상 Unpop/Pop/낙하와 큰 포획·너겟 전달 helper의 반환은 이 전수의 명시 외부 입력이다. 빈 완료 callback을 공급하지 않으며 실제 걷기 소비자에서 선택되면 완전한 공간/포획/RPC 몸체를 연결해야 한다. 실제 CarrierArrival·사제Arrival/귀환·포획·운반·같은 Host 삭제/의식 다음단계·전체 캠페인 metadata/UI/음향/1-1 완주는 아직 미완료다. 보호 원본/AGENTS/dotnetpj·공유 기존 파일 편집0, 게임/GUI0, 커밋/푸시0이다.
