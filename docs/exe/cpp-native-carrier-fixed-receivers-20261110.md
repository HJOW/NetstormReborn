# Native Carrier·동맹·의식 고정 수신기 — 작업1259

**15행을 같은 전역 고정 F/Native lease에 연결했다. 기존 건설계8행도 유지하며 전체 M6-l·NoWorld·목표는 미완료다.** Source 기본표의 F는 장기 game과 불변 코드 주소만 보유한다. 배송 시 현재 실제 Runtime을 pin한 뒤 같은 본문을 직접 호출하며 old Session/World/lease/enqueue pin과 packet[1]/현재 등록표 G 재선택은 없다. 초기 등록 자체는 현재 Session/Player/모드를 읽지 않는다. 모든 startup 이전 설치나 전체 NoWorld 동작의 완료 증거는 아니다.

| type | F | observer | version/policy | 길이 |
| --- | --- | --- | --- | --- |
|77h|0048f7f0|0040b380|1/0|6|
|79h|0048f8f0|0040b380|1/0|6|
|6fh|004900e0|0040b380|1/0|6|
|40h|00428570|0040b380|1/0|18|
|4fh|00426520|0040b380|1/0|9|
|50h|00444bc0|0040b380|1/0|9|
|60h|004267c0|0040b380|1/0|10|
|61h|0048bfe0|0040b380|1/0|11|
|5ch|0041e860|0040b380|1/0|6|
|5fh|00420bf0|0040aca0|1/0|ffff|
|7dh|0041e320|0040b390|1/0|ffff|
|6bh|00448e90|0040b380|1/0|10|
|6dh|00493f80|0040b380|1/0|8|
|75h|004205f0|0040b150|1/0|ffff|
|76h|004207a0|0040b380|1/0|6|

원문40b4b0의 해당 전체 등록 쓰기와 보호 PE0052ec40을 대조했다. Native의 기존 각 등록 위치에서 같은 F/metadata를 검증한 뒤 lease만 붙이며, 다른 등록이나 부분 실패 순서를 앞당기지 않는다. 독립 typed/CD의 기존 metadata/NULL observer는 유지한다. 기존12개 실제 lambda 본문과 동맹3개의 공통 본문은 새 `ReceiveFixedCarrierPacket`에 각각 한 번만 남는다. owner 인자명만 sender로 바꾼 다섯 본문 외에는 기존 내용이 그대로다. Drop의 PrepareCarrierDrop과 sender 사용, 선택Form 삭제, 기도·시전 요청/효과의 순서를 유지한다. 새 별도 게임 알고리즘을 복제하지 않았다.

Native75h가 NULL로 덮이던 문제를 교정했다. 원문40b150처럼 송신 때만 현재 count DWORD를 진단하고, 복귀 가능한 실제 Client 진단 뒤 현재 low WORD를 다시 읽어 같은 선언WORD를 쓴다. `RawMeltdown::FinalizePacket`을 재사용하며 독립 typed의 기본 진단 예외는 유지한다. Native Meltdown 방송은220BYTE의 전체 mutable 예약을 기존 `BroadcastAssetPacket`에 넘겨 현재 Player/Source 관문 뒤 실제 대상별 observer만 길이를 정하게 했다. typed의 기존 수동 finalizer/peer 관문은 그대로다. 새 MP 실행으로 확인한 결과는 아니다.

초기 정적 검토에서 변환 도구가6bh 내부 lambda의 닫는 괄호를 외부 receiver 끝으로 잡은 것을 발견했다. 첫 빌드/검사 전에 변경 전 등록 한 줄의 전체 본문으로 교정했다. 최초 원시와 교정 이유를 보존했고 기대값은 변경하지 않았다. 첫 빌드 중 확인한 Native 송신 경로는 해당 빌드 종료를 확인한 뒤 수정해 다시 빌드했으며 빌드 도중 제품 소스 변경은 없다.

최종 Release/x64 제품2·집중1대상 **140.509초/종료0·경고/오류0**, 고정9파일 불변·기존 **비MP300/300** 통과다. 최종 EXE의 Tutorial4 정상 UI 생산/건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0과 PNG를 확인했다. 이 TYPE75는 유닛 TYPE 번호이며 패킷75h의 실행 검증이 아니다. 추가15F·포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용·MP 단위/패킷/통신/게임/기록 실행은0이다. 전체 동일성·모든 분기·완공/청취를 검증한 것은 아니다.

보호 원본/CD/fixture·현행 AGENTSb5117f524 **2,808파일 불변**이다. 마지막 Client 양판본2026-11-03/R2026-11-07 디컴파일은 현재와 같은 HJOW-Athlon이며 새 디컴파일/원문 수정/ZIP은 없다. AGENTS의 외부 fonts 계약은 M8/K1/K4/K5에 남는다. 작업 중 사용자8136df49e의10차 상세 로그/11차 일본어 등 요구를 LEFT_JOBS의 E1~E3/L1~L3에 추가해 잔여22묶음으로 갱신했다. AGENTS 자체는 수정하지 않았고 구/신 보호 기준을 따로 보존했다.

다음은50행 gameplay 목록의 건설계8/이번15를 뺀 **준비27행**이다. 이미 장기화된32h 등의 실제 등록도 포함하므로27개 모두가 미구현이라는 재집계는 아니다. 이동·Squid 변이/압축·관측·개발자 명령·남은 caller와 Activity/Chat/BattleLeave를 대조하고 같은 본문/F/metadata/lease를 연결한다. 큰 NoWorld·최초 후처리·Path/AI/배치/Fort Finish·M6-m/n과 전체92행 감사도 남는다. 사용자는10차 오류/예외 공통 처리의 선행을 검토 중이며 우선순위 변경은 아직 미확정이다. LEFT_JOBS에 이를 명시하고 결정 전까지 현재 M6를 계속한다.

[확정 보고서](../../cpppj/native-carrier-fixed-receivers-20261110.json), 원시 `extracted/cpp-resume-1259/`.
