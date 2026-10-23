# 단위354: 메인 메뉴 돌 버튼의 원본 질감 프레임 수정

사용자 요청으로 Early Missions 전체·캠페인 전체·데모 3개·Edit/커스텀 시나리오 및 Multiplayer 제외 메뉴 복원 목표를 재개했다. 시작 HEAD는 `39cd2549`이며 작업 트리는 깨끗했다. AGENTS.md 및 공통/닷넷 인수인계를 다시 읽었다. 현재 PC와 마지막 전체 정밀 디컴파일 PC는 **VM-W11-Codex / 2026-10-23**으로 일치한다. 기존 디컴파일 소스는 변경하지 않았다.

## 원본 근거와 수정

기존 `extracted/refined/originals/Netstorm.c`의 `004254b0`은 배경 생성자 `004655d0`에 `DAT_005ca920`, `DAT_00542664`를 전달한다. 보호 PE의 SHA-256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`이다. Python pefile로 실제 파일을 읽어 가상 주소 `00542664`의 DWORD **1**, 타입 초기값 `005411c8`의 DWORD **92**를 확인했다. webpj 작업에서 남긴 A01/Sandy marble 및 원본 스크린샷 대조 근거와 일치한다.

UberGump가 버튼 생성 정보와 실제 픽셀 합성에 fortGump의 A01 프레임을 사용하도록 두 경로를 함께 수정했다. 기존 A00을 사용하는 별도 패널의 선택은 유지한다. 호출자가 임의 프레임을 공급하는 공용 배경 검사와 과거 x86 fixture는 바꾸지 않았다.

REA MCP는 같은 PE의 식별에는 성공했으나 Ghidra의 `read_bytes`가 `H:\` 경로 속성 검사에서 Win32 87로 실패했다. 이 실패를 바이트 관측 성공으로 계산하지 않는다. 읽기 전용 `rea doctor`는 종료1로 설치 CLI 6.7.0과 등록된 6.5.0의 차이도 보고했다. 설정은 변경하지 않았고 세션은 닫았다. 실제 DWORD 확인은 위의 일반 shell/pefile 관측이며 REA Evidence가 아니다. 이 단위는 기존 Ghidra 산출물과 보호 파일 직접 읽기로 진행했다.

## 검증

- Release 빌드2 종료0, 관측된 컴파일 경고·오류0. 로그: `extracted/cpp-resume-menu-20261023-build2.log`.
- 기존 GumpBackground 12개, RawButtonControls 5개, RawMenuControls 8개: 총 **25개·실패check0**. 변경은 질감 선택 두 곳과 관련 설명이며 전체 회귀는 이 단위에서 다시 실행하지 않았다. 이전 전체 회귀 기록은 단위352의 **2,335개·실패0**이다.
- 호스트 예외 규칙을 확인하고 클론만 창 모드/음소거/1024×768/6프레임으로 실행했다. 종료0, stderr 비어 있음. 원본 실행 파일은 실행하지 않았다.
- 실제 클론 캡처 `extracted/cpp-resume-menu-20261023.bmp`를 눈으로 확인하고, `screenShots/mainMenu.png`의 클라이언트 원점 `(2,33)`부터 1024×768을 잘라 RGB 채널 절대차 평균을 계산했다. **전체 2.6858512031, 버튼 영역 `[356,311,668,354)` 6.3366378454**다. 손실 잡음이 포함된 참조 스크린샷과의 근접성이며 완전한 픽셀 일치 판정은 아니다.
- 클론 실행이 바꾼 허용 파일 `originals/d/options.cfg`는 시작 시 깨끗했던 Git 내용으로 복원한다. 나머지 원본/CD 및 AGENTS.md는 수정하지 않았다. 원격 push는 하지 않는다.

## 다음 단위

Edit 버튼은 여전히 `Tell(NotImplemented)`이며 실제 Client/Editor Session에는 `EditorFiles` 및 `BindFileSave`가 연결되지 않았다. 기존 원본 저장 부모·Fort 모듈을 재사용하여 generic Fort 설치의 money/badges/technology/populate, 편집 상태 전환과 사용자 작업본 저장 수명을 연결한다. 전체 미션 자연 완주/음향·GUI 동일성·배포 압축을 완료했다고 주장하지 않는다. 목표는 활성 상태로 계속 진행한다.
