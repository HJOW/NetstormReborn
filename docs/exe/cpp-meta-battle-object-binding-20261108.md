# 1122 — 메타 전투 실제 물리 바인더와 배율 정정

제품1122 / 표시·파일·제품버전11.01이다. 1121은 `f40e1625 / 20261108-66 CP`로 커밋됐다. `004adca0` 및 `004ac220` 전체 원문·다음 함수까지의 기계어를 읽고, 현재 Client/Session의 실제 장치를 생성 부모에 공급하는 `BindClientMetaBattleObjects`를 추가·컴파일했다. **바인더의 소비자 호출, Rig와 23h 등록은 후속**이다.

`004adca0`은 객체 raw+1a의 DWORD 배율을 쓰는 함수다. 1121의 미등록 생성 초안에 있던 owner 표현·훅을 제거하고, QA→raw+8 WORD0→raw+1a DWORD0→현재 좌표 Pop 순서를 보존했다. 이전1121 문서·보고서도 정정했다. 실제 렌더링의 별도 소유자 지정은 이 함수와 혼동하지 않는다.

바인더는 현재 ChallengeGlobals의 전투 TYPE과 같은 Session의 World.Create/Pool.Slot/World.Pop을 사용한다. QA는 raw+0xc WORD를 쓴 뒤 현재 다리 TYPE일 때만 NotifySurface한다. 바인딩 시 Session/CS를 선취득하지 않으며, 각 효과를 선택한 위치에서 조회한다. 실제 DWORD SID의 물리 범위를 확인하며 free/active 필터는 추가하지 않는다. 기존 animate 콜백은 보존한다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0, 보호2800파일 불변이다. 새 fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이다. 컴파일을 실제 메타 장면 실행 성공으로 쓰지 않는다. 제품 기준은 **1122 / 11.01**이다. 메타 전투 생성의 배율 쓰기를 정정하고 현재 Client/Session의 실제 팩터리·Pool·QA·Pop 바인더를 컴파일했다. **바인더 소비자·Rig와 23h 등록은 후속**이다. 두 Release 종료0·경고/오류0·보호2800 불변·새 C++/MP/게임/통신 실행0이다. 다음은 Rig·나머지 CS 수신/생성·HTML→1dh/28~33 등록/1095 송신 상태 호출이다. 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI도 남아 전체 목표는 미완료다.

Netstorm.exe 12,670,976바이트/SHA-256 `1af9d6e4f0c8aaa88fc9c97de73b5c094726c3764af34c6762355014f89bf41d`, R.exe 669,696바이트/SHA-256 `1073feb51bbbb67fbe9f2bb10ea2108dd2063a8e6a2c53bf9521c8640d895e3b`다. 보고서는 `cpppj/meta-battle-object-binding-20261108.json`, 원문·기계어·빌드·정적 감사는 `extracted/cpp-resume-1122/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
