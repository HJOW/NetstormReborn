# Native Login·연결·Editor 수신 등록 — 작업1261

**실제 Login/다른15행 중14F를 장기 기본표에 연결하고 기존67h·41h를 같은 값으로 대여했다. 알려진 gameplay50행은 유지한다. 전체 원문92행/활성표 재집계·모든 caller/owner·NoWorld·M6-l/목표는 미완료다.** Native 현재 등록을 짧은 Login/Session closure로 덮지 않으며 enqueue에 이전 Login/World/lease/pin을 넣지 않는다.

| type | F | observer | version/policy | 길이 |
| --- | --- | --- | --- | --- |
|27h|004a7800|0040a450|1/0|ffff|
|63h|004a7510|0040b380|1/0|8|
|29h|004b3b40|0040b380|1/0|34|
|67h|004b4900|0040ae80|1/0|ffff|
|2bh|004912d0|0040a590|1/0|ffff|
|2ah|004b3d30|0040a540|1/0|ffff|
|28h|004b8940|0040b380|1/0|18|
|81h|004b3e90|0040b460|1/0|ffff|
|84h|004a6f30|0040b380|1/0|8|
|6eh|004a7050|0040b380|1/0|8|
|72h|004b3bc0|0040b110|1/0|ffff|
|3dh|00491220|0040b380|1/0|9|
|78h|0048f880|0040b380|1/0|6|
|52h|004a71a0|0040b380|1/0|5|
|64h|004b6180|0040b380|1/0|5|
|41h|0042dc90|0040a8d0|1/0|ffff|

원문40b4b0의 해당 전체 등록 쓰기와 보호 PE0052ec40을 대조했다.63h의decimal99와64h의decimal100도 정규화했다.67h는 새 F를 만들지 않고 기존004b4900의 Client 전역 본문/observer를 보존한다.41h도 같은 장기0042dc90이면 현재 대여 여부와 관계없이 metadata를 확인해 같은 값을 붙인다. 실제 Login 생성자의27/63·Client8행·metadata72·3d/78 등록 위치와52/64 부모가 기존 Native F를 대여한다.32h·71h의 기존 장기 부모는 유지한다.

서버27/63과 Client 응답/metadata는 배송 때 현재 실제 Runtime과 Login을 지역 소유해 같은 기존 모듈을 호출한다.27h의 nullable 연결은 server 자연 반환/Player 부분 쓰기 뒤 실제 pendingReply가 선택된 지점에서만 요구한다. 기존 참조 API도 같은 ReceiveDirect 본문을 쓴다. Client8종은 불변 Native entry를 직접 선택하는 ReceiveFixed에 기존 switch 본문을 그대로 옮겼고 typed Register도 등록 시점의 entry를 캡처한다. compatibility Receive만 현재 packet종류를 해석하며 Native 배송이 packet[1]로 F→G를 재선택하지 않는다.8개 본문은 label/함수 인자를 바꾼 정확 대조를 통과했다.

동맹3d/78 본문은 같은 Impl에 각각 한 번만 두고 typed/Native가 공유한다. 가변3d의 같은 mutable 패킷과78의 양방향 AddAlly→방송→실제 종류7 안내 순서를 유지한다. Editor는 새 Login을 요구하지 않으며 실제 현재 UI 장치를 안내 위치에서만 선택한다. 호출 당시 Login이 있으면 반환까지 실제 owner만 보유한다.

52h는 현재 Player owner만 보유해 기존 비트 쓰기 함수를 직접 호출한다.64h는 같은22프레임/22벽시각·FrameTime·현재 server와 기존 RawConnectionActivity를 사용한다. 두 본문은 Session/연결/payload를 새로 요구하지 않는다. 실제 안내 삭제는 missionPost 자연 관문 뒤 기존 WarningMessages.Remove/재그리기 한 본문을 Client와 Campaign 서비스가 공유한다.27h의40a450은 두 원문 진단 뒤 현재 BYTE/WORD를 재조회하며,72h의40b110은 기존 순수 observer를 공유한다. 독립 typed의 기존 metadata/진단/효과를 보존했다.

Editor는 같은 비임대52h/64h만 허용하고 다른 소유자는 모든 쓰기 전에 거부한다. 전체 충돌 검사 뒤 두 기본 F를 기존 baseline 목록에 저장하고 정상/부분 실패 반납 때 같은64/50 gameplay/41/52 F로 복구한 뒤 사본을 비운다. 다른 observer/version/길이를 지우거나 성공 상태를 합성하지 않는다.

Release/x64 제품2·집중1대상 **353.615초/종료0·경고/오류0**, 고정18파일 불변·기존 **비MP300/300** 통과다. 최종 EXE Tutorial4 정상 생산/건설 요청→TYPE75/owner1 생성→600프레임 대기→정상 종료0과 PNG를 확인했다. 같은 제품의 격리된 일반 Edit는 TEST01 진입→Main Menu/No→메인 복귀를 같은 Client에서2회 통과했고 실제 OS 제목 EDITOR:TEST01/MAIN·정상 종료0·게임 자료/보호/EXE 불변·PNG를 확인했다. Test Battle·MP·포획/희생·특정 적 공격·승리/자연 완주·공중3종 전용 실행은0이다. 이 TYPE75는 유닛 TYPE 번호이며 모든 패킷 분기/MP 배송/완공/청취/전체 동일성 검증이 아니다.

보호 원본/CD/fixture·현행 AGENTS22a0877c3 **2,808파일 불변**이다. 사용자가 추가한 WB8/9차 제외를 공통 인계에 기록했고 CPP의7차 오류/예외·로그 우선순위와 구현 범위는 유지했다. AGENTS 자체는 수정하지 않았고 구/신 보호 기준을 보존했다. 마지막 Client2026-11-03/R2026-11-07 디컴파일은 같은 HJOW-Athlon이며 새 디컴파일/원문 수정/ZIP은 없다. 외부 fonts 준수는 M8/K/L에 남는다.

다음은 원문92행과 실제 Client 기본표/모든 등록 caller·metadata·Source/Poll/기록/해제의 전수 재집계다. 원문 C3 7행의 동적 기본 등록과 폐기7f/80 정책을 포함해야 하며 R 전용 literal이나 소스 태그 존재를 실제 활성표로 세지 않는다. `full-canonical-preparation.json`은 검색 후보일 뿐이다. M6-m JoinTestBattle·M6-n 큰 NoWorld/최초 후처리·Path/AI/배치/Fort Finish도 남는다.

[확정 보고서](../../cpppj/native-login-fixed-receivers-20261110.json), 원시 `extracted/cpp-resume-1261/`.
