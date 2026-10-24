# HTML 배경 전역의 실제 수명 연결 — 2026-10-25

단위412는 HTML 배경 태그의 쓰기를 Tell·Help·전체 HTML 공장이 공유하는 장기 저장소에 연결한다. 현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 새 전체 디컴파일·원본 직접 실행·클론 GUI 실행은 없다. 전체 목표는 활성/미완료다.

## 원문과 변경

10.78의 `46c321`은 `=` 뒤의 첫 따옴표를 건너뛰고 다음 따옴표·꺾쇠·NUL까지 원문 바이트를 `565e70`에 쓴 뒤 NUL을 붙인다. 다음 독립 전역은 `565ec0`의 단락 억제 DWORD다. 따라서 유효한 이름은 최대79바이트다. 짧은 이름으로 덮어쓸 때 그 뒤의 기존 바이트를 지우지 않는다. 태그가 없는 다음 페이지와 숨긴 배경 태그는 이 전역을 바꾸지 않는다.

[HtmlTextParser](../../cpppj/src/client/HtmlTextParser.h)에 이80BYTE 저장소와 연결 함수를 추가했다. [UberGump](../../cpppj/src/client/UberGump.cpp)의 기존 Tell 지역 문자열과 Help/전체 공장의 미연결 콜백을 같은 장기 HTML 전역에 연결했다. 구형 Tell 표시 어댑터도 두 번의 HTML 처리 뒤 현재 전역 이름을 읽는다. 저장소 반납 시점은 개별 HTML 부모의 수명과 별개다. 범위 밖 쓰기는 기존 복원 방침에 따라 명시 거부한다.

전체 Uber가 이 이름으로 BackgroundGif 창을 새로 생성하고 기존 루트를 교체하는 연결은 아직 남았다. 이번 단위는 배경 이미지의 모든 화면 전환·팔레트·음악·표시를 완성했다고 주장하지 않는다. 10.72의 별도 전역 배치는 이번 관측 대상이 아니다.

## 독립 관측과 검사

[새 관측 도구](../../tools/decomp_html_background.py)는 보호 PE의 실제 HTML 생성·설정·전체 순회·태그 본체를 **648행·471900명령** 실행한다. 초기80BYTE 세 종류, 대소문자/인용/구분자/빈 이름/첫NUL·여러 태그·79BYTE 이름, 표시·숨김 상태와 단락 전역을 조합했다. 기대 출력은 C++를 읽거나 실행하지 않고 실제 출력/상태/80BYTE 전체에서 얻었다. 읽기 재생은 자료를 변경하지 않는다.

CRT `tolower`는 ASCII 계약의 명시 장치다. 게임·UI·OS를 실행하지 않으며, 정수/문자 경로이므로 두 x87 정밀도 비교를 별도로 수행하지 않았다. 원문80BYTE를 넘는 오버런 입력의 결과도 주장하지 않는다.

[파서 검사](../../cpppj/tests/HtmlTextParserTests.cpp)는648행 전체 출력·현재 필드·단락 전역·배경80BYTE를 대조하고, 다른 파서가 같은 저장소를 빌리는 수명과 유효 범위 거부를 검사한다. [실제 전체 공장 결합](../../cpppj/tests/HtmlWindowFactoryClientTests.cpp)은 일반 창/다음 페이지/빈 본문 임시 부모/창 생략의 효과와 실제 공통 UI 반납 뒤의 전역 보존을 검사한다.

Release 빌드 종료0·관측 경고오류0,집중 파서8+실제 전체 공장8=**16개/실패0**,전체 CTest **2787개/실패0·319.57초**다. 보호 **originals1394/CD425/AGENTS/정밀 소스=1821개**가 시작 시점과 같다. 제품 SHA256은 `7d13019fdc91f627f84c10ce399930c71ec29c8fb34073939ef0ff11ce7c2330`, 검사 SHA256은 `08d282a9ff46f98bb0499cb37abd0b71c57cd17be5c143649848042abe0dc505`다. [제품 보고서](../../cpppj/html-background-client-runtime-20261025.json)에 소스/참조/원시 SHA와 범위를 기록했다. 원시는 `extracted/cpp-html-background-412-*`다.

## 다음 범위

Checked의 실제 `userCheat`/개발 모드 전역, 기존 Tell 전체 부모·구형 `%` 표시·Guide의 실제 HTML 개수 통합과 full Uber가 남는다. `originals/d` 디스크 우선·TAFF 폴백으로 합친 영어 리소스549개에서 배경83곳·Checked491곳·TechList1곳·ThemeTechList0곳·Input9곳·Zsend2곳을 확인했다. `*TECHLIST*`는 SacrificeMenu에서 사용되고 Input/Zsend는 서버·길드·관리 서비스이므로 현재 범위의 캠페인/일반 UI를 우선한다. 디스크/TAFF의 원문 행과 섹션 기록은 `extracted/cpp-html-background-412-resource-uses.json`에 있다. 전체 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오·완전 동일성과393 Wide SaveOptions는 계속 미완료다.
