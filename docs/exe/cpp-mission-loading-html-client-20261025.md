# 미션 시작 안내를 실제 전체 HTML에 연결 — 2026-10-25

단위407을 마친 후 **사용자 요청으로 로컬 커밋 뒤 목표를 일시 중지**한다. 전체 목표는 미완료이며 단위408은 시작하지 않는다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일과 원본 게임 직접 실행은 없었다. 보호 PE/정밀 소스 SHA는 보고서에 고정했고 originals1394개와 AGENTS/기존 Native 자료가 같다.

## 변경

[MissionLoadingMessage](../../cpppj/src/client/MissionLoadingMessage.cpp)는 4cf570의 기존 본체에 실제 Client의 transientSerial/createdSerial 필드를 참조로 빌린다. 두 전역을 복제하거나 공장 반환 serial과 현재 createdSerial을 합치지 않는다. 기존 독립 상태 호출도 같은 참조 본체를 사용한다. Cancel 번역은 첫 `~.`/대소문자 구별 `$Button` 조건에서 필요한 경우만 조회한다.

[RawMenuLoading](../../cpppj/src/client/RawMenuLoading.inc)은 LoadingFields/0xc4 장식 부모·Cancel 전용 메타·부분 Fit·별도 입력 처리를 제거하고 실제 전체 HtmlWindowFactory/Metadata/Gump/Fit을 사용한다. 신규 공장 뒤 현재 createdSerial을 다시 조회해 원래 메타/본문을 두 번째 갱신하는 순서와 현재 updateDepth/serial 재조회를 보존한다. 교체한 자식은 Kill 후 공통 Cleanup에서 반납한다. 안내의 Bounds/Point/Report도 같은 실제 전체 부모와 현재 버튼 슬롯을 읽는다.

[UberGump](../../cpppj/src/client/UberGump.cpp)는 같은 Client HTML 전역/벽 시각/원시 입력 큐/현재 화면과 지도 뷰/Config/기존 HTML 단락 전역·명령/80칸 문자열 고리/맵 캐시를 공급한다. 맵 이름은 각 조회에서 다시 읽되 반환 문자열의 수명을 유지한다. 한 번 설치한 콜백도 변경된 현재 뷰를 읽는다. 링크는 부모의 현재8192BYTE에서 읽어 기존 Help/cmd/외부 분배기에 전달하며 Kill 뒤 Cleanup 전의 수명을 보존한다. 선택된 미복원 장치의 명시 실패는 유지한다.

## 검증

- 기존 원문 HTML **732행**과 미션 안내 **1540입력×두 정밀도=3080반환**을 현재 PC의 실제 PE에서 읽기 전용으로 재생했다. cf570의 명령 위치 1638460개/정밀도 차이0이며 fixture 변경0이다. 하위 Config/파서/할당/메타·본문 등은 각 원문 도구의 명시 경계이며, 실제 C++ 접합과 구별한다.
- [새 실제 Client 결합4검사](../../cpppj/tests/HtmlWindowFactoryClientTests.cpp)는 같은 소유 전역/전체 생성·재사용/지연 삭제와 Enter Cancel 원시 큐, 사용자 메타/`~.`와 긴 본문, 현재 뷰 변경, 파서 효과가 current createdSerial을 다른 실제 부모로 바꾸는 경우를 확인한다. 기존 실제 메뉴의 마우스 Cancel/픽셀/반납 검사도 새 경로를 사용한다.
- Release 빌드1/2 종료0, 집중 **175/실패0**, 전체 CTest **2767/실패0·348.41초**다. 같은 최종 제품/검사 바이너리로 실행했다. 첫 결합 검사의 한 실패는 화면 전체 교체 뒤에도 serial이 같다고 본 기대값이었다. 기존 SetControls의 Clear가 전체 안내를 반납함을 확인해, 같은 뷰 안의 변경은 재사용/전체 화면 교체는 새 부모 생성을 각각 검사하도록 고쳤다. 제품의 수명 동작은 바꾸지 않았다.
- 실제 클론 GUI1회는 Tutorial2의 첫45프레임에서 정상 미션 시작을 실행했다. 첫3프레임은 전체 부모/serial18·본문31·버튼29와 직사각형(354,344,754,424)을 유지했다. 4프레임에 전체 부모 수/모달 표가0으로 반납됐고40프레임에 첫A. 안내가 열려 있었다. 시작/첫 안내 BMP를 직접 확인했다. 게임 상태/SP/기술/진행값 주입과 원본 게임 실행은 없다. 허용된 두 설정은 실행 전 바이트/존재 상태로 복구했고 originals1394개가 같다.

제품 SHA256 `449a2f3904ff63296323df7a61f61add5a633ec8dcc7e1464f5435f4f39daa96`

검사 SHA256 `5c1833cef35bb7d5e1f5b0a860576d6f221feb7576b311752d628913086549d9`

[제품 보고서](../../cpppj/mission-loading-html-client-runtime-20261025.json)는 소스10/참조/기존 Native/종료 로그·GUI 원시 SHA와 경계를 보존한다. 원시는 `extracted/cpp-html-loading-407-*`다.

## 재개 지점

기존 Tell/full Uber 전체 교체, 기술/@/#/%/Input·전송·배경/Checked 전역, 기존 페이지와 전체 HTML 개수·Guide의 통합 및 dock 초기85의 실제 연동이 남는다. Production/Minimap 동시 제공자·State/Core/Pool/Win32 접합, 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성과393 Wide SaveOptions 원인도 미완료다. 이번 GUI는 시작 전환만 검증했으며 이 전체 범위의 완료를 뜻하지 않는다. 10.72는 별도 비교·복원이 필요하다.
