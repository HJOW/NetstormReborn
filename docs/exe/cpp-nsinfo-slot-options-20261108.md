# 1107 — 메타 슬롯 변경과 공통 랭크 getter

제품1107 / 표시·파일·제품버전11.01이다.1106(`41b2b735 / 20261108-30 CP`) 뒤 NSInfo 입력의 선행 몸체0041c4f0/520 색 변경·0041c550/580 랭크 변경·0041c6a0 특수 슬롯 전환을 전체 복원했다.0041c5b0의24DWORD 랭크 getter도 같은 모듈로 옮겨 현재 실제 PlayerInfo 표시가 공유한다.

모든 함수는 signed slot0..7 관문을 span 접근 전에 검사한다. 무효 슬롯은 읽기/쓰기 없이0이고 유효 writer는1을 반환한다. 색은 BYTE 연산 후 첫 저장과 비교를 수행한다. 증가는7초과일 때0, 감소는 정확한ff일 때만7로 두 번째 쓰기를 한다. 랭크는 원문 BYTE를 zero확장한 DWORD에서 먼저 연산/비교하고 최종BYTE만 쓴다. 감소는 음수에서만23으로 바꾸므로 비정상 큰 입력을 임의 정상 범위로 clamp하지 않는다. 특수 슬롯은 정확히1인 경우만0, 그외1로 쓴다. XOR나 단순 bool반전으로 바꾸지 않았다.

원문 전체 기계어의 DL/EAX 폭·상한/부호 분기·두 저장 위치·setne를 정적으로 대조했다. 기존 PlayerInfo 표시의 getter를 같은 구현으로 교체해 표/입력 간 중복을 제거했다. **다섯 writer의 실제 UI 입력 호출, 세 NSInfo Handle·메타 참가자 메뉴·상태/옵션 송신은 아직 후속이다.** 이번 단위를 입력 복원 완료로 보고하지 않는다. 원문 물리 필드/24DWORD 표 밖의 접근은 호스트 예외로 드러내며 원문 메모리 오염을 재현하지 않는다.

NetstormCpp·NetstormRoot Release/x64 빌드 종료0·경고/오류0이다. 보호2800파일 불변·새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 사용자 제외에 따라 새 MP 검사는 실행하지 않았다. 전체 메타 화면/State19 실행 성공이 아니다. 다음은 실제 입력/메뉴/송신→CS 수신/생성·카메라 Rig/HTML→1dh/28~33 등록/1095 송신 상태 호출이다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다.

Netstorm.exe 12,545,536바이트/SHA-256 `7b31e65d80b957b10ba6bd15fc69a41e59390d77316f01e973255dbc6b421d93`, R.exe 669,696바이트/SHA-256 `7032048f74791bbe48d15a3fcfd16a973071739c9b33c810bf6baf8c6dddf8ba`다. 보고서는`cpppj/nsinfo-slot-options-20261108.json`, 원문/기계어·빌드·정적 감사는`extracted/cpp-resume-1107/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
