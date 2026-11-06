# 1034 — 현재 미션 설정의 수명과 실제 소비자

기존32h/0044dbc0·0044dc20 수신/송신은 실제 Login에 등록되어 있었으나7DWORD가 Login 지역 배열에 머물렀다. 일부 친군 사격 bool만 갱신되고 피해 무효화/강제 배치 및 다른 총 판정에는 같은 현재 값이 전달되지 않았다. 이번에는7DWORD를 기존 Client 수명의 RawGameAuxState에 소유하고 Login/생산/실제 판정이 같은 주소를 읽고 쓰도록 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 원문 C와 보호 PE의 최초값/참조를 정적으로 대조했으며 원본 실행/수정과 신규 MP 검사는 하지 않았다.

| payload | 10.78 전역 | PE 최초값 | 실제 연결 |
| --- | --- | --- | --- |
| +4 GeneratorTimer | 005326cc | 1 | 생산 유지의 현재 getter. Login 없음에 따른 상수1을 제거 |
| +8 HackMover | 0055a488 | 0 | 두 실제 CanonPlacement 파이프라인의 즉시 허용 관문 |
| +12 NoDamageDebug | 0055a48c | 0 | 사제94380/일반add10의 동일 PriestDamageState 피해 무효화 관문 |
| +16 ShootMyFriends | 0055a490 | 0 | 표적/지상 사격·총/배리어·Wind/Rain 아비어리·애시드의 현재 비영 관문 |
| +20 FillMyTank | 0055a494 | 0 | 원문32h처럼 전체 DWORD 저장. 31h 명령의 값 해석/대입은 후속 |
| +24 CombatGumpColumns | 005326d0 | 1 | 원문32h처럼 전체 DWORD 저장. 별도 명령의 UI 사건은 후속 |
| +28 CombatGumpRows | 005326d4 | 8 | 원문32h처럼 전체 DWORD 저장. 수신에 clamp/재배치를 추가하지 않음 |

송신의 기존 load 순서1/2/0/4/5/3/6, 수신의현재payload0..6 store 순서, 같은32BYTE/등록 observer/프로토콜/정책은 보존했다. 수신은 callback 사이에 나머지 입력이 바뀌어도 다음 DWORD를 현재 bytes에서 다시 읽는다. 배열은 같은 원문7전역의 호스트 표현이며 원문의 비연속 주소를 연속 image라고 주장하지 않는다. 새 Session/World/Login 생성으로1/0/0/0/0/1/8을 다시 쓰지 않는다.

피해/친군 사격은 기존 RawCheckingFlag의 현재 DWORD 비영 뷰를 재사용했다. 이 뷰는 원래값을0/1로 저장하지 않으며 각 검사 시점에 같은 owner를 읽는다. 총 표적과 실제 GunProcessState를 모두55a490에 연결했고, Wind/Rain globals와 RawAcidFence도 bool 임시값의 주소를 붙잡는 대신 같은 뷰를 값으로 보관한다. 애시드와 아비어리의 기존 독립 bool 참조 입력도 뷰가 그대로 빌린다. checking 진단은 별도의 기존005e4794 주소를 계속 사용한다. 두 피해 몸체는 같은 PriestDamageState를 사용하므로 일반/사제 각각에 별도 설정 사본을 만들지 않는다.

강제 배치는 기존 PriestPlacementState의 독립 forcePlacement를 남기고 실제55a488 포인터/getter를 추가했다. 실제 두 파이프라인은 같은 현재 DWORD를 빌리고 기존 blockedRelation0→강제 허용/genus→모양/기하 순서에서 읽는다. 수신은 별도배치/공격/피해 작업을 즉시 실행하지 않는다. 추가 대조한44dc70의fillMyTank 분기도 정수 인자 확인/55a494 대입 뒤 반환하므로 도움말 설명만으로 에너지 변경 작업을 합성하지 않는다.

CD판32h는004a58f0/004a5960이며 현재7설정은00537e94..00537eac의 연속 DWORD다. 송수신payload 순서는 동일하지만 PATCH의 전역 주소/송신 load 재배치를 CD에 이식하지 않았다. 기존 RawMissionSettingsPacket은 계속 PATCH 전용이며 이번 Client 접합을 CD 전체 복원 완료로 세지 않는다.

Release 제품 빌드 **종료0·경고/오류0**. EXE **12,070,912바이트**, SHA-256 **`81e34f75740ae75ce097f10b8ecc997540643ebf5b0a184a581574ce3bc5eef5`**, 표시 버전11.01이다. AGENTS/원본/CD/기존fixture **2800파일 불변**, 신규fixture0이다. 신규 MP 단위/통신/게임 및 지정된 사용자 확인·공중3종 전용 검사는 실행하지 않았다. 실제 LAN/공격/피해 동작 검증 결과가 아니다.

다음은31h/0044dba0→40a6f0(packet,0)→44dc70의 실제 해석기/명령 효과·입력·전파와 나머지 수신/전송·재동기화/Activate·메타/Outpost/checking5다. 기존32h 등록은 Login 범위이므로 비Login/Editor의32h 등록 연결도31h 접합 후속으로 남긴다. 디버그 명령 이름/별칭66개는 작업 PC의 정적 메모로 추출했으며 제품 등록 완료가 아니다. 이후 한국어 후속→시나리오UI 순서를 유지한다.
