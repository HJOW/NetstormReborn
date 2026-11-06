# 1028 — 비행 전체 수신과 테마별 재해석

10.78의39h/0045e2b0과0045afb0을 같은 Session/Kernel에 연결했다. 마지막 전체 정밀 디컴파일은 **2026-11-03 / HJOW-Athlon**이다. 이번에는 원문과 보호 PE를 정적으로 읽었으며 원본을 실행하거나 수정하지 않았다. PATCH 전용 수신 ABI는 CD와 혼합하지 않는다.

신규 free Form은 dirty ff 진단 뒤 임시 Sun 기본 생성자와 첫 decoder를 실행한다. 그 부모의 현재 TYPE.theme가1이면 Rain,0이면 Wind,그 외이면 Sun 기본 생성자를 고른다. 두 번째 객체 배정/생성 뒤 임시 inner의 실제 VT/두 DWORD−12345 소멸 쓰기와 해제를 수행하고, 새 객체에 다시 decoder를 실행한 뒤 기존 TYPE31/명시 Form/flags10으로 부착한다. 일반 생성자의 홈·위치/속도 계산·서버 진단·현재 시각 쓰기/부착이나 첫Frame 초기화를 추가하지 않는다.

기존 live Form은 dirty&1 진단 뒤 같은 PID의 outer 필드만 해석한다. 두 가상0 observer는 현재 네 실제 VT에서 모두480260/ret12임을 PE로 대조했다. 미연결 PID/VT는 가짜 객체나 빈 observer로 대신하지 않는다. decoder는 부모DWORD를 별도 반환하고 두 WORD/세 DWORD/두8BYTE를 실제 비트/폭/순서로 복사한다. dirty/inner 소유권과 미선택 바이트는 보존하며 선택 필드의 짧은 입력도 앞선 부분 쓰기를 되돌리지 않는다. writer도 같은 필드 폭/위치 정의를 사용한다.

기본 생성자45c280의00500460은 PE에서0.0임을 확인했다. Sun/Rain/Wind의 전용 타이머/VT는 기존 정확한 leaf 함수를 재사용하며 원문 미작성 영역을 새로 지우지 않는다.39h의 observer NULL/버전1/정책0/가변 길이 등록과 Editor 충돌/반납 목록도 같은 수명으로 연결했다.

Release 빌드는 종료0·경고/오류0이다. 제품1028/11.01은12,019,200바이트, SHA-256은 `d0f85f2102d708e2d0da5527d5b82721aa9053865b112f5266d9fd3c61873e88`다. 신규MP/단위/게임/통신·공중3종 전용 검사와 fixture 생성은0이며 보호2800파일 불변이다. 이 결과는 정적 복원/컴파일이며 비행 행동이나 LAN 플레이 검증이 아니다.

다음은 애니메이션3bh/3ch 등 나머지 decoder와 재동기화/Activate 부모다. 내용물 통지의 PlayerKnown binding, 메타/Outpost/checking5, 한국어 후속과 Edit 시나리오 UI도 남아 있다.
