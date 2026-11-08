# 1170 — M6-c2/c3 실제 건설 송수신과 비멀티 건설 확인

2026-11-08 / HJOW-Athlon. 이전 CPP1169/976f2b7b, 제품11.01이다. Client 정밀2026-11-03·R2026-11-07/같은 호스트의 C와 inclusive PE 범위를 사용했으며 생성 디컴파일 소스는 바꾸지 않았다.

실제 생산 Confirm/Request/LocalGeyser가 ActualConfirm을 사용한다. builder 선택 뒤 현재 패턴8칸을 읽고 호출별 지역512BYTE를 초기화한다. 각 Create→SID WORD 기록→개수 증가→20개 이상 후행 진단→decoder Advance 순서를 보존한다. 전체 조각 사전 순회/19개 선제 거부를 실제 경로에 넣지 않는다. 원문의 Construction.cpp:0x32d 진단 뒤 정상 복귀하면 다음 조각을 계속 처리한다. 기존 typed Confirm의 독립 입력 계약은 유지한다.

같은512BYTE에 원문 순서로 본문을 작성하고 대상별 현재 Player flag→NULL peer→local 제외 뒤 같은 Registry.SendMutable로 방송한다. observer가 바꾼 같은 버퍼를 ActualHandle에 직접 넘긴다. loaded/로그인 생성 관문, const 복사, 송신 전 별도 observer 호출을 추가하지 않았다. LastPacket은 방송 뒤 진단 사본이며 실제 처리의 입력 owner가 아니다. NULL 원격 peer뿐이면 초기 선언 WORD4를 그대로 유지한다.

Client 기본 표에3fh 순수 observer/version1/policy0/가변 길이를 설치했다. Session은 prepare→ActualHandle의 direct receiver를 대여하고 반납하며 Editor 목록은42→43종이다. 대여 전/반납 뒤 observer는 같은 장기 Storage만 보유하고 Session 수신기를 남기지 않는다. 원문 decoder의 저장 TYPE는 Advance로 변하지 않으므로 값형 request.type을 그대로 사용한다.

호스트는 원문 지역 배열 침범 UB를 복제하지 않는다. 실제512BYTE 쓰기를 넘으면 해당 Create 뒤 접근 위치에서 예외를 내며 앞선 효과는 유지한다. 빈 decoder의 미초기화 첫 SID는 기존 명시0 경계다. 단독 비멀티 실행은 다중 peer의 observer 누적 변경을 입증하지 않으며 해당 부분은 원문/실제 송신 경계 정적 대조로 기록한다.

허용된 실제 실행에서 두 기존 결함을 발견해 함께 수정했다. 첫 실행은 메뉴 자원 프레임의 실제 조회 hook 누락으로 실패했다. 원문의 boss 기본1을0으로 바꾸지 않고 현재 Config/벽시각/실제 budget/간헐천 목록/TYPE cost의5개 조회를 연결했다. 비전투 budget0과 실제 count 비교가 자연 반환한다. 두 번째 실행은 State8 준비 중 loaded 선행 검사로 실패했다. 해당 선행 오류만 제거하여 원문의 LoginEnd/모드/기한 관문을 먼저 실행하게 했다. World 없는 replenish/populate의 실제 전역 생성 부모 분리는 여전히 M6-n이다.

세 번의 제품 두종 Release/x64 빌드는 모두 종료0·경고/오류0이다. 실패 실행 둘의 로그와 전후 해시도 보존했다. 최종 같은 EXE로 별도 격리 설치/사용자 루트에서 다음 두 확인을 수행했다.

| 확인 | 실제 결과 |
| --- | --- |
| Tutorial1 다리 | 정상 생산 항목 취득→화면 포인터의 legal 확인→좌클릭. 3eh 수신0→1, 새 서버 SID15003/TYPE82/owner1/좌표63,49. 추가90프레임 대기 후 정상 종료0. |
| Tutorial4 윈드 제네레이터 | 워크샾 우클릭→Put Knowledge into Production→Wind Generator→TYPE75 생산 취득→legal 확인→좌클릭. 3eh 수신0→1, 새 서버 SID15074/owner1/좌표125,85, HP700, SP2000→1600. 추가600프레임 대기와 실제 PNG를 확인한 뒤 정상 종료0. |

Tutorial4에서 잘못된 이름 기반 도움말 닫기와 자산 이름 입력2건은 MCP 오류로 보존하고 실제 PNG 좌표 입력으로 계속했다. 성공 입력으로 바꾸어 기록하지 않았다. 두 성공 실행은 EXE·각 설치 자산·AGENTS/원본/CD/fixture2800파일 전후 해시가 모두 같다. 전체 건설 가상 Process 수명이나 모든 픽셀 동일성까지 확대하지 않는다. MP·새 C++단위/패킷/통신·포획/희생/특정 적 공격/승리·공중3종·ZIP 실행은0이다.

M6-c1~c3의 실제 접합을 마쳤다. 다음은 M6-d 삭제 통지/공통 삭제 부모다. M6-d~n/M7/M8와 한국어/시나리오 UI를 포함한 전체 목표는 미완료다. 원문11함수/사본은 extracted/cpp-m6-construction-prep/, 자원 권한 추가 원문은 그 아래 nonmp-smoke/authority-audit/, 빌드와 네 실행의 원시는 extracted/cpp-resume-1170/다. [기계 판독 보고서](../../cpppj/construction-live-integration-20261108.json)를 따른다.
