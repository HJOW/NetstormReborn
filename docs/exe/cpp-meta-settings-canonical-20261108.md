# C++ Meta8 장기 수신과 현재 설정 접합

2026-11-08 / 단위1180 / HJOW-Athlon / 제품11.01. 1179 뒤 메타 수신8종과32h 설정 수신의 장기 기본 처리를 연결했다. M6-l/n 전체와 지연 큐는 미완료다.

메타20/21/22/23/24/25/67/88의 저장된 처리기는 같은 Client의 현재 데이터를 사용한다. Meta 객체의 this를 등록표에 남기지 않으며, 직접 호출자도 같은 본문을 공유한다. 원문의 모드 조기 반환, CS 진단, 관찰기, 버전1/정책0, 고정·가변 길이를 유지한다. 등록 시점과 원문 NULL 수신을 임의로 바꾸지 않는다.

87BYTE 옵션 배열은 Uber에서 ClientGameplayGlobals로 소유권을 옮겼다. 기존 초기값과 명시 Reset/부분 복사 writer는 보존하며 Session 교체로 새 배열을 만들지 않는다. 67h의 마지막 UI 관문은 같은 현재 bool 참조를 읽는다. **1184 정정:**1180에서 DWORD 변환 임시값 문제로 판단했지만, 실제 SquidOwnerMode::battle은 이미 bool이었다. 해당 진단/복원 주장을 정정하고 불필요한 StateBattleOptionFlag를1184에서 제거했다. 당시 빌드·실행·소스 해시는 그대로 보존한다. 원문4193f0의 실제 PE 인자c1400000에 맞춰 참가자 입장 높이를−12로 교정했다.

32h/44dc20은 같은 장기 GameAux의7DWORD에 순서대로 직접 쓴다. 실제 공용 기본 표가 비NULL 빈40b380 관찰기와 같은 F를 설치하고, Login/일반/Editor는 이 F를 다시 빌린다. 독립 typed 등록의 기존 계약은 유지한다. 이전 쓰기 뒤 다음 packet DWORD를 읽으며 전체 사전검사·정규화·성공 ACK를 더하지 않는다.

Meta 참가자 이미지에서 ExistingEditor를 요구하던 잘못된 조건을 분리해 독립14×14 지도/owner0/ignorePosition 경로를 제공했다. 현재 이 자식은 실제 Session의 Builder와 terrain 자원을 사용한다. 월드 없는 이미지/지면 효과/삭제와 전체 장기 owner 접합까지 완료한 것은 아니다.

원시 Meta8 누적 패치는 선행1179 Host/선택/자원 변경을 유지한 최소 hunk로 적용했다.32h의 CCT include 충돌은 실제 현행 문맥에 수동 병합했다. 변경은18개 소스 파일이며 `extracted/cpp-resume-1180/`에 적용 전·고정 해시, 두 빌드 로그, 허용 비MP 원시와 보호2800파일 해시를 보존했다. Meta의56함수 집합과 설정4함수 집합의 C/실제 inclusive PE 자료는 각 준비본 evidence에 있다. 겹친 함수를 고유 합계로 합산하지 않는다. 마지막 정밀 디컴파일은 Client2026-11-03/R2026-11-07의 같은 HJOW-Athlon이며 생성 원문은 변경하지 않았다.

NetstormCpp/NetstormRoot Release/x64는 최초와 관문 수정 후 모두 종료0, 컴파일 경고·오류0이다. 최종 EXE로 격리1920×1080/영어/무음 캠페인1-1에 정상 진입해90frame 대기와 게임 시각 증가, 정상 종료0을 확인했다. 설치 사본·EXE·고정 소스·보호2800파일이 불변이다. 새 MP·단위·패킷·통신·ZIP 검사 및 사용자 제외 포획/희생·특정 적 공격·승리 재현은 실행하지 않았다. 이 비MP 결과는 새 메타/MP 수신 동작의 실행 통과 근거가 아니다.

현재 직접 Native 태그는24개, 폐기 검증1개를 제외한88대상 중 무태그64개다. 태그 수와 전scope 완료 수를 구별하며 큐는 계속 비활성이다. 다음은41h의 현재 Chat0 조기 반환을 보완한 장기 수신, 전역 Source/listener/공통표/Poll, 자원 전용Regular/삭제/NoWorld, JoinTestBattle와 전수 마감이다. 이후 M7→M8→한국어K1~K5→시나리오S1~S4 순서를 유지한다.

[보고서](../../cpppj/meta-settings-canonical-20261108.json)·[현재 인계](../../LEFT_JOBS.md)·[M6 감사](cpp-lan-remaining-audit-20261108.md).
