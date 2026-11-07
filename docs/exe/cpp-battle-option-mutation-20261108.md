# 1114 — BattleOptions 변경과 제외 목록 전체 몸체

제품1114 / 표시·파일·제품버전11.01이다.1113은`1c565aec / 20261108-49 CP`로 커밋됐다.0041c3c0/460/4b0/4d0/6d0/730/7a0의 일곱 전체 몸체와 각 다음 함수까지 기계어를 읽고 RawBattleOptionMutation을 제품 라이브러리에 등록·컴파일했다.별도 옵션/제외 목록 사본을 만들지 않으며 실제 입력의87BYTE를 빌린다.

첫 BYTE getter와1105에서 보호PE로 확인한 동일22레코드/76선택의 포함 최댓값을 사용한다. 순환은 BYTE 증가/최댓값 순환을 계산한 뒤 guy.wav가 복귀해야 실제BYTE를 저장한다. 직접 선택은 음수에서 표 조회 없이 반환하며0..포함 최댓값에서만BYTE를 쓴다.

제외 목록은 전체DWORD 인자를 실제BYTE와 비교한다. 추가는 중복1·최초count40관문·목록에 잘린BYTE 저장→현재countBYTE 증가→음향 순서다.입력을 비교 전에BYTE로 줄이지 않는다.제거는 최초 일치의 뒤를 최초count 기준 길이만큼memmove→음향→그때의현재countBYTE 감소다.끝 잔존BYTE를 지우지 않는다.추가/제거 반복 조건은 현재count를 재조회하며 독립getter는 최초count를 캡처한다.음향 실패 전 이미 이동/추가한 부분효과는 보존한다.원문 배열 밖 접근/메모리 오염은 호스트 물리 범위에서 명시 오류다.

두 Release/x64 빌드 종료0·경고/오류0·보호2800파일 불변이다.새fixture/C++/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 새 MP 검사는 사용자 지시로 제외한다.**아직 BattleInfo0047b8f0 전체 입력/실제 메뉴에서 이 writer를 호출하지 않는다.** 현재 컴파일을 옵션 UI 동작/전체 메타 장면 성공으로 쓰지 않는다.다음은 전체 BattleInfo 입력/설정·제외 메뉴와1114호출→CS 수신/생성·카메라Rig/HTML→1dh/28~33등록/1095송신 상태호출이다.세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어후속→Edit시나리오UI도 남아 전체 목표는 미완료다.

Netstorm.exe 12,620,800바이트/SHA-256`6f74aee437766a8172c89b16964ea7282154c56f0aa38aa0ab56238cea727756`, R.exe 669,696바이트/SHA-256`09ae99ba0dc695e7194e329b6ed4b40d3ead809c1e1fb927b5c2a7deb74757ad`다.보고서`cpppj/battle-option-mutation-20261108.json`, 정밀 원문/기계어·빌드 기록`extracted/cpp-resume-1114/`는Git제외다.마지막 전체 정밀 디컴파일**2026-11-03/HJOW-Athlon**, R전용**2026-11-07/HJOW-Athlon**이며 재생성/생성소스 변경0이다.AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
