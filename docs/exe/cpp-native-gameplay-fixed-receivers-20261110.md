# Native gameplay 고정 수신기 — 작업1260

**나머지26F를 연결하고 기존 장기32h를 보존해 알려진 Editor gameplay50행의 고정 F 연결을 마쳤다. 전체92행·다른 scope·NoWorld·M6-l/목표는 미완료다.** 건설계8/Carrier15/이번26과 장기32h의 집합이 현재50행 목록과 정확히 같다. 이50을 전체 등록표나 MP 동작 완료로 세지 않는다.

| type | F | observer | version/policy | 길이 |
| --- | --- | --- | --- | --- |
|2ch|00433c30|0040b380|1/0|35|
|2dh|004327b0|0040b380|1/0|18|
|2eh|00433170|0040b380|1/0|8|
|30h|00489ee0|0040a660|1/0|ffff|
|31h|0044dba0|0040a6b0|1/0|ffff|
|32h|0044dc20|0040b380|1/0|32|
|33h|004a71c0|0040b380|1/0|18|
|34h|004333f0|0040b380|1/0|12|
|35h|0046e940|0040b380|1/0|20|
|38h|0045e410|0|1/0|ffff|
|39h|0045e2b0|0|1/0|ffff|
|3ah|0046a0c0|0|1/0|ffff|
|3bh|00466d30|0|1/0|ffff|
|3ch|00466b60|0|1/0|ffff|
|4ah|00432710|0040b380|1/0|7|
|4bh|004326e0|0040b380|1/0|6|
|53h|004436b0|0040b380|1/0|6|
|5bh|004266c0|0040b380|1/0|6|
|62h|00433120|0040ad70|1/0|ffff|
|65h|00432850|0040b380|1/0|13|
|6ah|0044a280|0|1/0|ffff|
|70h|004cf980|0040b090|1/0|ffff|
|73h|0045e5d0|0|1/0|ffff|
|74h|0045e4f0|0|1/0|ffff|
|7ah|00434080|0040b2a0|1/0|ffff|
|7eh|00497030|0040b380|1/0|28|
|86h|00426490|0040b380|1/0|20|

원문40b4b0의 해당 전체 등록 쓰기와 보호 PE0052ec40을 대조했다.26F의 기본 등록은 장기 game/진단과 불변 코드 주소만 보유한다. 실제 배송 때 현재 Runtime을 pin해 기존 같은 본문을 직접 호출하며 old Session/World/lease/enqueue pin이나 packet종류/현재 표 G로 재선택하지 않는다.26F 모두 sender/nullable connection을 읽지 않는다.32h는 기존 장기7DWORD 직접 쓰기와 BorrowCurrentMissionSettingsReceiver를 그대로 사용해 Session/World를 새로 요구하지 않는다.

Session의 단일 공통 진입점은 기존 이동/좌표 정밀도·카메라·섬 정리·Path·Squid 수신/검증/변이/압축·공유 Regular·간헐천·건/비행/애니메이션/에이비어리·Daïs 본문을 재사용한다. 기존20본문 묶음이 각각 한 번만 남으며26F를 덮는다. 두 문자열 파싱은 RawDeveloperCommandPacket/RawRemoteTellPacket의 공통 Receive 함수로 옮겨 typed Register와 Native가 공유한다. 첫 NUL·물리 범위·선언 길이를 읽지 않는 수신·현재 Execute/Tell·표시 부호화 경계를 유지한다. 본문 끝의 공백 외 알고리즘을 바꾸지 않았다. RawPathData/RawSquidMutationReceive/RawSquidCompressedReceive/RawMissionSettingsPacket 알고리즘 파일은 변경하지 않았다.

Native는 각 원래 receiver 등록 위치에서 같은 F/metadata를 검증해 lease만 붙이고, typed/CD의 기존 metadata·등록 순서·NULL observer와6ah 판본 조건은 유지한다. 실제 Native observer는 NULL8/빈13/mutable5개다. 원래33/34/35의 typed metadata 선행 순서도 보존한다. 일반/Editor/반납의 나머지 scope와 모든 부분 실패·NoWorld의 전수 감사는 후속 작업이다.

변환 도구의 지역 end 변수 재사용은 단일 복사 검사에서 검출돼 첫 Session 쓰기 전에 중단했다. 별도 재시도에서 문자열 메타데이터 기록이 누락된 것은 변경 전 스냅샷에서 복원했다. 원시 후보/교정 기록을 보존했고 첫 빌드/실행 전 전체 단일 복사·균형 괄호·metadata 대조를 다시 통과했다. 제품에 맞춰 기대값을 변경하지 않았다. 최초 Tutorial4 진입은 Login include 파일의 별도31h 등록이 같은 고정 F를 짧은 Login closure로 덮어 실패했다. 최초 빌드/검사/실패 EXE 해시와 로그를 보존하고, Native Login의 원래 등록 위치도 같은 F/observer 대여로 교정한 뒤 새 빌드/비MP 회귀/단순 UI 검사를 수행했다. 일반/Session뿐 아니라 이 실제 Login 호출자도 같은 본문을 공유한다.

Release/x64 제품2·집중1대상 **127.939초/종료0·경고/오류0**, 고정11파일 불변·기존 **비MP300/300** 통과다. 최종 EXE Tutorial4 정상 UI 생산/건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0/PNG를 확인했다. 이 TYPE75는 유닛 TYPE 번호다. 이번26F의 MP 배송/전체 분기·포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용·MP 단위/패킷/통신/게임/기록 실행은0이다. 완공·청취·전체 동일성을 검증한 것은 아니다.

보호 원본/CD/fixture·현행 AGENTS8661e1060 **2,808파일 불변**이다. 사용자가 변경한 AGENTS의 이전/현행 기준은 따로 보존했다. 마지막 Client 양판본2026-11-03/R2026-11-07 디컴파일은 현재와 같은 HJOW-Athlon이며 새 디컴파일/원문 수정/ZIP은 없다.

사용자8661e1060과 새 요청에 따라 **오류/예외·로그는7차**로 확정했다. 현재 단위를 마친 뒤 LEFT_JOBS에 `K → E → S → B → L`로 반영하고 미확정 표기를 제거했다. AA/FSR8차는 기존 구현 마감 유지, 시나리오9차·알려진 버그10차·기타 언어11차다. E1~E3의 홈 logs/시간 파일명/원인 추적 정보 요건은 그대로이며 현재 M6를 먼저 계속한다. 외부 fonts 준수는 M8/K/L에 남는다.

다음은 gameplay50 밖 Login 구성의27/63·MissionClient8행·3d/78과52h 전투 이탈·64h 연결 활동·41h Chat의 등록/반납·호출자 수명 및 전체 등록표 재감사다. 이미 장기67h도 현재 Login의 short 재등록을 확인해야 하므로 소스 태그 수를 실제 현재 표로 세지 않는다. 원문64h는16진수 표기 대신 decimal100으로 등록하므로 전수 수집에서 양쪽 표기를 정규화해야 한다. 큰 NoWorld·최초 후처리·Path/AI/배치/Fort Finish·M6-m/n도 남는다.

[확정 보고서](../../cpppj/native-gameplay-fixed-receivers-20261110.json), 원시 `extracted/cpp-resume-1260/`.
