# Native 건설 등록8행 연결 마감 — 작업1258

**54h/51h/2fh/6ch를 추가해1257과 합친 건설계8행의 고정 F/Native 대여를 연결했다. 전체 M6-l/NoWorld/M6/목표는 미완료다.** 실제 Source 기본표 등록에서 장기 game/코드값만 보유하며 배송 때 현재 실제 Runtime과 같은 본문을 선택한다. old Session/World/lease/enqueue pin을 저장하거나 packet[1]/현재 등록표 G로 재선택하지 않는다.

| type | F | observer | version/policy | 길이 |
| --- | --- | --- | --- | --- |
|54h|004436f0|0040b380|1/0|8|
|51h|004dc050|0040b380|1/0|8|
|2fh|004332c0|0040b380|1/0|15|
|6ch|004d56f0|0040b380|1/0|11|

원문40b4b0의 전체 해당 등록 쓰기와 보호 PE 길이를 대조했다.54/51은 같은 salvageCommand/vortexDeparture의 실제 본문을 호출하며51의 sender signed 변환을 유지한다.2f의 PrepareConstruction/모든 칸·후보 정리와6c의 현재 SP/플레이어/표시/송신 순서는 기존 lambda 본문을 공통 함수로 옮겨 Native와 typed가 직접 공유한다. 거부318BYTE·송금2,108BYTE UTF-8 본문이 각각 정확히 한 번 남는 것을 정적으로 확인했다. RawConstructionFailurePackets/RawSalvageCommand/RawVortexDeparture 알고리즘3파일은 변경하지 않았다.

Native의 기존 RegisterConstructionCommands는 각 원래 등록 위치에서 같은 불변 F와 metadata를 확인해 lease만 붙인다. 앞선37/3e/3f/58도 유지한다. 독립 typed/CD의 기존 등록과 metadata는 유지했다. Native observer는 이번 네 행 모두 실제 비NULL40b380이며 typed의 기존NULL 계약과 구별한다. 미지 타입은 다른 F로 보내지 않고 명시 실패한다.

최종 Release/x64 제품2·집중1대상 **339.144초/종료0·경고/오류0**, 고정5파일 불변·기존 **비MP300/300** 통과다. 최종 EXE Tutorial4 정상 UI 생산·건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0/PNG를 확인했다. 모든 추가 F의 분기·MP/지연 배송·완공·청취·전체 동일성의 검증은 아니다. 새 MP 단위/패킷/통신/게임/기록 및 실제 포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용 실행은0이다.

변환 스크립트의 초기 CRLF 가정은 실제 LF와 달라 본문 찾기 관문에서 멈췄다. Session 파일 쓰기/검사 실행 전이었고 실제 LF를 확인해 보존한 국소 변환으로 교정했다. 보호 원본/CD/fixture·AGENTS75e4eed9e **2,808파일 불변**이다. 마지막 Client 양판본2026-11-03/R2026-11-07 디컴파일은 현재와 같은 HJOW-Athlon이며 새 디컴파일/원문 수정/ZIP은 없다. 글꼴 외부 fonts 계약은 M8/K1/K4/K5에 남는다.

다음은 RegisterCarrierCommands와 다른 실제 Native scope다. 기존50행 gameplay 목록에서 건설계8을 뺀42행의 원문 등록 자료를 `next-native-gameplay-rows.json`에 준비했다. 이는 전체88행이나 각 실제 caller/본문의 완료 판정이 아니다. 현재8F의 본문은 현재 Runtime을 요구하며 전체 NoWorld 장기화도 남는다. 큰 하위·최초 후처리·Path/AI/배치/Fort Finish·M6-m/n을 계속한다.

[확정 단계 보고서](../../cpppj/native-construction-completion-20261110.json), 원시 `extracted/cpp-resume-1258/`.
