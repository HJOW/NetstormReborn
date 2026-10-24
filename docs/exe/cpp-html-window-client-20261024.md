# 실제 HTML 부모의 본문·제어 수명 — 2026-10-24

현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 소스 SHA256은 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`다. 새 전체 디컴파일·원본 게임 직접 실행·게임 GUI 실행은0이다.

## 실제 부모와 공통 UI

[HtmlWindowGump](../../cpppj/src/client/HtmlWindowGump.cpp)는 원문 **4cf120/new0x21e8**의 실제 장식 기본 주소와8192BYTE HTML/20개 세DWORD 제어 행을 유지한다. kind540436은27이며 초기 timeout double500460은0이다. 생성자는 HTML 첫BYTE와 지정 필드만 초기화하고 timeout/onExit 인자 DWORD·HTML 뒤쪽은 덮지 않는다. CD 대응은 ctor4c2210/body4c2500/clear4c2680/add4c26d0이며 이번 구현은10.78이다.

[ClientHtmlWindowGlobals](../../cpppj/src/client/ClientHtmlWindowGlobals.h)는 인스턴스 수5cab2c·created/transient serial·최근 종료 시각5cad30/34를 Client 수명에 유지한다. 두 부모를 동시에 만들고 하나씩 반납해 count와 장기 주소 보존을 확인했다. 소멸은 vtable설정→count감소→OnExit raw큐→현재aux serial조회/Kill→벽 시각→기본 소멸 순서다. **원문은 OnExit 필드를0으로 덮지 않는다.** 기존 페이지 helper의 중복 방지 정책을 새 부모에 적용하지 않았다.

[RawMenuHtml.inc](../../cpppj/src/client/RawMenuHtml.inc)는 같은 공통 등록표/장식/실제 돌 버튼·각 배경/ScrollingText·Slider·StyleText/입력/수명에 연결한다. 본문 교체는 LF보충→기존Kill/0→새 생성→secondaryDepth=-1→scroll0→Markup→부착→현재 보조창Kill→비void Push/가상 재부착 순서다. LF 없는1007BYTE 이상은 원문 지역 버퍼 밖 덮어쓰기를 모사하지 않고 명시 거부한다. 원문의 일반 factory는 최종 LF를 보충하는 별도 상위 경로이며 아직 후속이다.

AddButton은20행 한도/생성 후 현재 행/가상 부착/인자→명령/Instant/count 재조회를 유지한다. Clear는Kill 뒤 현재 count를 다시 읽는다. Handle은 count와 무관한20행 serial·첫 행·태그/메뉴 종료를 처리하며 release를 추가 거부하지 않는다. 선택 명령을 raw40칸 큐에 넣고 부모를Kill한 뒤 실제 후반 소멸에서OnExit를 넣는다. 실제 부모/본문/줄·문자 조각은 raw_html_globals/root/body 진단에서도 같은 저장소를 읽는다.

## Idle와 독립 원문

원문 vtable515e7c+30의 **4c8350 Idle**는 정밀 텍스트에 별도 진입이 없어 PE 전체 본체를 확인했다. 기한0은clock 없이 반환하며, 그 외에는clock 뒤 **현재 기한/명령/인자**를 다시 읽는다. ordered 현재시각>=기한일 때 raw큐→Kill이며 명령0도 별도 거부하지 않는다. 음수/±0/NaN/무한 기한과clock 돌연변이를 대조했다. timeout 메타/실제 공통 idle 표 등록까지 완료했다는 뜻은 아니다.

[상위 원문 도구](../../tools/decomp_html_window_gump.py)의 실제6본문 **250행**과 [Idle 도구](../../tools/decomp_html_window_idle.py)의 별도 실제 **v2 88행**, 합계338행을 최초 작성·읽기 전용 재생했다. 두 x87 정밀도 결과 차이0이며 C++ 본체도 같은338입력을 통과했다. 상위24현재DWORD/20행/8192BYTE와 실제 분기/RET를 관측했고 기본 장식/할당/자식/가상 부착/큐/링크/시각/수명 하위는 명시 장치다.130782명령 위치에는 장치/STOP도 포함된다. 선택 중단은 전체 Native EH unwind 검증과 구별한다.

첫 Idle v1은 일부 불변 필드를 실제 메모리에 설치/재조회하지 않아 제품 검사에서 제외했다. 도구/자료/보고서를 ignored raw에 보관하고 v2에서 자식/전역/20행을 모두 실제 메모리에서 관측했다. v1/v2 출력 바이트가 같지만 v1을 검증 증거로 세지 않는다. 기존 tracked 원문 자료는 변경하지 않았다.

## 검증과 경과

- 빌드1은 NamedPaletteColor→DWORD C2664/C2679, 빌드2는 검사 파생 포인터 삼항 C2446으로 종료1이었다. 실제 index와 기본 주소 if 반환으로 고쳤고 멤버 가림/부호 경고도 수정했다. 빌드3~6 종료0·관측 경고오류0이다.
- 최초 집중1은 원문338 C++ 본체를 통과했으나 실제20버튼 검사에서 공통 버튼 소유 배경을 메뉴 전용 Node로 읽은 빈 포인터 접근으로 종료했다. 실제 button.child/배경 토큰 관계로 검사하며 callback이 참조하는 관측/clock보다 UI를 먼저 반납하도록 검사 수명도 명시했다. 임시 진단은 최종 코드에서 제거했다. 제품 본체의 원문 계산을 기대값에 맞춰 바꾸지 않았다.
- 최종 실제 결합 **6검사**, 집중 **105검사/실패0**(기존 Tell/Dialog/로딩/GIF/Chat/편집/수명 포함), 전체 CTest **2740검사/실패0·311.01초(wrapper)**다. 같은 최종 제품/검사 바이너리와 originals1394동일을 확인했다. 보호/CD/AGENTS/기존 Native 추적 변경0이다. 이번 단위는 새 HTML 부모의 실제 전체 배치/게임 화면 픽셀 검증은 하지 않았다.

최종 제품 SHA256: `6419ae40ad3c1ea27779ea22aeaadecde4480ce65051df7a3ef413d135faa76e`

최종 검사 SHA256: `33ca6b7e26e8c7e68eb75d023e4f29d63b2c137df20a8b6897e93ab37b57a2c0`

[제품 보고서](../../cpppj/html-window-client-runtime-20261024.json)에 소스14/참조14/보존 Native9/새 Native6와 종료 원시91파일 SHA를 기록했다. 원시는 `extracted/cpp-html-window-403-*`다.

## 다음 접합

전체 Fit/Attach4c7d40·메타4c83b0·factory4cf270과 정상 Idle 등록·실제 링크 처리를 이어야 한다. **기존 LoadingFields/0xc4 부분 미션 안내와 새 부모를 아직 교체하지 않았다.** 미연결 전체 Fit은 명시 거부한다. full Uber의 새 부모/자식 생성 뒤 이전 것 반납, Production/Minimap의 동시 제공자, Native State/Core/Pool/Win32 경계가 남는다. 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성 및393 Wide SaveOptions 원인은 미완료이며 /goal은 활성 상태다.
