# 1133 — 메타 선택 캡처의 실제 Image·글꼴 문맥 연결

제품1133 / 표시·파일·제품버전11.01이다. 이전 단위는 `3abed63a / 20261108-97 CP`로 커밋됐다.

`498000/4a2b10/4a2c70` 전체 원문·기계어를 대조했다. 기존 선택 몸체는200×200/색255 버퍼와 현재 실제+a4를 사용한다. 메타+a4에만 같은 Client의 동적 Image/TYPE/SHP·글꼴 출력 장치를 제공하고, 기존 음수/opaque/범위·plain/추가Draw 순서를 변경하지 않는다.

선택용 글자 출력은 기존 RawMenuControls의 같은 FontStore/GumpGlyphColorState를 임시 표면/clip에 빌린다. 원점0/끝점199의 같은200픽셀 문맥을 사용하며 이전const activeText 포인터는 정상/예외 모두 복원한다. 메뉴의 현재clip/팔레트/표면을 영구 변경하거나 별도 글꼴·색·게임 상태 사본을 만들지 않는다.

실제 메타 display 생성 직후 현재 Client/controls를 동기 장치로 설치하고 정상 화면과 같은 DrawClientMetaSprite를 호출한다. 필요한 실제 장치가 없을 때 새 그림이나 선택 성공을 합성하지 않는다.

**MetaSpriteDisplay.Hit의 픽셀 장치 접합까지**이며, 메타 마우스 입력/선택 caller·4789b0 전체 메뉴/항해 입력은 후속이다. 사용자 지시대로 신규 단위/MP/게임/통신 검사는 실행하지 않았다.

이번 접합/주소 근거는10.78이다. 10.72의 메타 가상표/선택 pane 동일성은 이번 단위에서 추가 판정하지 않았으며 같다고 가정하지 않는다.

NetstormCpp/NetstormRoot Release/x64 종료0·경고/오류0·보호2800파일 불변이다. 새fixture/C++ 단위/MP/게임/통신/전체검사/34미션/ZIP 실행0이며 마지막 C++ 실행검사는1052 기존7개다. 컴파일을 전체 메타 장면·MP 실행 성공으로 쓰지 않는다.

다음은 메타 객체 입력/메타 선택 caller/나머지 CS 수신/참가자 생성과 카메라 caller/HTML→1dh/28~33 등록/1095 송신 상태 호출 → 세.server·LAN hostname/Config registryValue·LAN/Outpost/checking5→한국어 후속→Edit 시나리오 UI이다. 전체 목표는 미완료다.

Netstorm.exe 12,795,392바이트/SHA-256 `6bc2e4c54ae3f717fab4b72cb47fd7b04b6e41c3b47da1568e6f0b636171c141`, R.exe 669,696바이트/SHA-256 `697518bbd0d8e966e9228de590085097bc891eaa712421ace2b075017df119eb`다. 보고서는 `cpppj/meta-selection-pixels-20261108.json`, 원문/기계어·빌드·정적 감사는 `extracted/cpp-resume-1133/`(Git 제외)다. 마지막 전체 정밀 디컴파일 **2026-11-03/HJOW-Athlon**, R전용 **2026-11-07/HJOW-Athlon**이며 생성 소스 변경0이다. AGENTS.md·원본/CD·루트README.md·DN/WB는 수정하지 않았다.
