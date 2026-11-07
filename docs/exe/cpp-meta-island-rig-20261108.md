# 1124 — 실제 IslandRig와 CS Image 복사

제품1124 / 표시·파일·제품버전11.01이다. 이전 단위는 `4664c37b / 20261108-70 CP`로 커밋됐다.

`00418b10/418ca0/418de0/418eb0/4192a0`와 다섯 목표 helper·소멸자, 관련 Image/Kernel finder/실제 빈 notify의 전체 원문·각 다음 함수까지 기계어를 읽었다. `NativeIslandRig`는 기존 SquidProcessHost.Attach(flags10h)와 같은 Kernel을 사용하는 실제 ProcessForm 파생 클래스다. **Client 장치 바인더/생성 소비자·23h 등록은 아직 연결하지 않았다.**

CS Image는 기존32비트 할당 토큰을 재사용한다. 해제→현재 source 크기→bits의 DWORD 곱/할당·공개/복사→shape 크기 쓰기/양수 길이에서만 할당·복사 순서다. source shape가0이면 shapeSize를 새로 쓰지 않는다. 자기 복사도 해제가 먼저다. 현재52/53 Animator TYPE는 같은 Client 수명에 보관하며 CS Reset/Session 교체에서 재초기화하지 않는다.

Rig는 생성/가상 부착에서 현재 객체·두 TYPE를 진단하고 같은 CS Image를 깊게 복사한다. 실제 종속 Form의 next를 효과 전에 저장하며 현재 TYPE 조건에 맞는 Gump만 표시·숨김·Kill한다. kill 전환은 불가역이고, 완료 판정을 Spring 갱신 전에 읽어 도착한 프레임의 다음 프레임에 객체 삭제 또는 ProcessForm 반납을 수행한다. 별도 성공 상태나 가짜 객체는 만들지 않는다.

디컴파일의 표현을 그대로 믿지 않고 부호 확장과 FPU 스택을 대조했다. clip 비교의 이전 비트는0/ffffffff 값이며0/1로 정규화하지 않는다. `0041911d`의 배율 읽기와 `0041915c`의 원래 높이 읽기는 PUSH EBP로 이동한4BYTE를 반영한다. 객체 보조BYTE30은 원래 높이 z가 양수일 때만 쓴다. VT+1c의00480260은 실제 빈 함수이므로 명시 override했다.

첫 빌드 뒤 장기 Rig가 짧은 Uber/viewport 참조를 유지하지 않도록 현재 전역 getter로 보완하고 재빌드했다. Image destructor는 UI getter 없이 같은 CS 할당 소유자만 사용한다. 실제 바인더는 이 CS 소유자를 마지막 반납까지 유지해야 한다. 카메라 Rig·객체 이미지 표시/생성·나머지 CS·HTML/상태 접합은 후속이다. x87 확장정밀도·NaN·전체 입력 비트 동일성도 미검증이다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 실제 Client Rig 장치/생성 소비자·23h 등록·카메라 Rig·메타 객체 그림/나머지 CS 수신/참가자 생성/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,670,976바이트/SHA-256 `c7fa7a3101a9d8409d5d4e4ea83589c7b7835de883b99a194daaac3e99ff055e`, R.exe 669,696바이트/SHA-256 `361b2aa33da8a6b8781c35e517e85f0e8e6735649bbe0c5b0dd25e7e37117385`다. 보고서는 `cpppj/meta-island-rig-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1124/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
