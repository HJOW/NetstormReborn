# 일반 LAN Fort 모드와 두 참가자 시작 — 단위1367

2026-11-26, HJOW-Athlon. 마지막 디컴파일 PC도 HJOW-Athlon이다. 기존 C와 보호 PE의 정적 읽기만 사용했고 재디컴파일·원본 실행/변경은 없다.

일반 전투 생성자가 미션 기본 objectState.chaffMode와 이전 Fort의 Client 장기 FortFinishOwnerState(chaff/special/fort)를 남겼다. 원문004be610은 fort/missionPost일 때만Chaff를 읽으며 일반 로그인 이미지는 Subscriber/Territory/TypeNames/CoreData/선택Terr의5섹션만 보낸다. 또한 RawIslandRuntime::Matches는special 또는chaff일 때owner 검사를 생략해 다른 참가자의 같은Terr까지 읽는다. 실제 실패는stream1,pos21,size21이었다. 두 Chaff 별칭만 고친 중간 실행도 실패했고 공유special/fort까지 일반 전투 값false로 맞춘 뒤 범위 오류가 없어졌다. 스트림 읽기·버퍼 한도·빈 데이터 처리는 완화하지 않았다. BYTE 예외에는 실제stream/pos/size를 남긴다.

같은 PC의 새 두 프로필이 같은 subscriber9375232를 가져 대기실에서 올바르게 중복 거부된 검사1회도 확인했다. 봇은 표준registeredSubId 옵션으로 서로 다른 오프라인ID6356993/6356994를 선택하도록 보강했다. World/ACK/Player 생성·전투 결과를 주입하지 않는다. 실패 때 양쪽 보고/화면도 보존한다.

최종9차 실행은 실제 양쪽 로그인14/PlayerKnown·Ready43비트·시작 안내 소멸·짧은 자연 대기·두 종료0을 통과했다. 두 관측의 Player명단/ID/Ready·로컬owner1/2·SP6500·초기월드7660객체가 일치한다. 화면에도 `Game started!`가 표시됐다. [호스트](../screens/cpp-lan-fort-modes-1367-20261126/host.png) · [상대 봇](../screens/cpp-lan-fort-modes-1367-20261126/bot.png). 기존 원시 관측을 새 봇의 강화된 성공 조건으로 독립 재대조했다. 단순 코드 재빌드만으로 통과 처리하지 않았다.

**후속 잔여는 남는다.** 별도8차 실행의 상대는 로그인14 뒤 `Archer 현재 clock/경과 오류`로 종료1이었다. 간헐적 시계 동기화/프레임delta 경계를1368에서 조사한다.16 이후17/18도 아직 미구현이다. 보호 PE의4b7cd5/4b7cf3와 원문4b6dd0을 대조해 로그 `Reconnection accepted/denied`,17의즉시14,18의sourceType0/next3을 확인했다. 정적 근거는 원시 `state-17-static-1367.json`에 있다. 생산·건설/개별 객체·SP 변경 동기화·긴 대기·재접속·실제 서버 인수/실물LAN은 이번 성공으로 세지 않는다. 사용자 직접 확인·MP 포획/희생·특정 공격·승리는 실행하지 않았다.

Release 빌드 성공. 새 C++ 단위 검사나 기존 집중37검사 반복은 없고 실제 두 참가자 검사/관측을 사용했다. EXE SHA-256 `4716a5f0b3da37ba789ae7a517c5ef0cd960588a2df3e4a001e2ff2fc8eabdb1`. 보호3212파일/기존fixture 불변. [검증 JSON](../../cpppj/lan-fort-modes-1367-20261126.json). 원시는 `extracted/cpp-lan-1364/bot-battle-5`~`bot-battle-9`·`build-*1367*.log`·최종`identity-summary.json`이다. 최신ZIP1362는1363~1367 수정 전이다.

[DN 후속](../dotnet-cpp-lan-followup-20261126.md)의DN-MP-04에공유모드와17/18, DN-MP-06에같은C# Archer 경과 검사와 시계 경계를 기록했다. DN 코드/실행/진행률은 바꾸지 않았다.
