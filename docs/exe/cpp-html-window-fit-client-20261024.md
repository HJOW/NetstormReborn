# HTML 전체 Fit/Attach와 실제 배치 — 2026-10-24

현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 소스 SHA256은 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`다. 새 전체 디컴파일·원본 게임 직접 실행·게임 GUI 실행은0이다.

## 원문 전체 가상 부착

[HtmlWindowFit](../../cpppj/src/client/HtmlWindowFit.cpp)는 실제 **4c7d40/ret16**의 공통 버튼 최대 폭·15간격/본문 자연 폭/20픽셀 확대·높이 정체/뷰·출력 화면 제한/메뉴 열/버튼·본문 부착/지도 또는 전체 화면 중앙/depth-80을 복원한다. 정밀 C가 incoming 스택 인자/x87 입력을 잃은 부분은 보호 PE 전체 명령과 실제_ftol로 확인했다. x/y는 현재 중앙으로 덮고 **원래 parent/mode는 보존**한다. CD 대응은4c27c0이며 이번 구현은10.78이다.

원문 FST32 중간 저장과 unrounded FILD/곱셈을 구별한다. 버튼 span/target20증가/현재 본문 높이·폭 기록/50초과 복구/뷰폭-80/화면폭-40·본문 높이 절반/slider·scroll·요청0을 그대로 따른다. 화면 제한은 원문 긴 본문 분기에서만 수행하며 버튼/메뉴 확장 뒤 별도 제한을 추가하지 않는다. 모든 자식 Push/Attach와 현재 count/body/메뉴 상태를 원문 시점에 다시 읽는다.

메뉴는 current 조회와 실제 AttachCurrent/pop을 사용한다. 두 열의 이전 높이 비교·조건부 두 번째 current 조회·원래 첫 메뉴 포인터의 현재 높이·현재 menuColumns로 부모 폭 확장·마지막columns0을 유지한다. 본문 X는 차이를_ftol한 뒤 signed /2, 버튼 좌표는 각 정수/float32 전환 위치를 보존한다. 마지막 기본 Attach 뒤 현재 depth를 읽어-80으로 맞춘다.

## 실제 공통 UI와 출력

[RawMenuHtml.inc](../../cpppj/src/client/RawMenuHtml.inc)는 같은 실제 Stone FitToText/ScrollingText FitContent·FitHeight/Slider·scroll/controlled 메뉴/공통 Push·가상 자식 Attach·기본 부모 Attach·AddDepth에 연결한다. 임의 fitAttach 호스트를 현재 Client의 **주소별 뷰/출력 크기/전체 크기/지도 직사각형 read**로 바꿨다. 제공자가 없으면 실제 Fit 진입에서 명시 거부하며 PE 미초기화0이나 기본 배치로 채우지 않는다. 기존 미제공 경계 검사도 통과했다.

[실제 결합 검사](../../cpppj/tests/HtmlWindowFitClientTests.cpp)는 보호 Arial/팔레트/TYPE·SHP/StyleText·실제 돌 버튼/두 controlled 메뉴/같은 공통 트리·등록표·클립/픽셀을 사용한다.400폭 짧은 안내의 균일 버튼/중앙·깊이, 긴 문장의320×240 화면에서280×140 스크롤 본문과320×205 부모, 두 메뉴 pop/부모 확대, 부착 후 본문 교체·현재 뷰 재배치, 보조 serial 생존 여부의 전체/지도 중앙 선택, 실제 출력의 클립/7BYTE 행 padding을 확인했다. 실제 BMP를 직접 확인했다. 이는 기존 게임 전체 화면/메타/미션 완주의 픽셀 대조는 아니다.

## 독립 관측과 검사

[독립 원문 도구](../../tools/decomp_html_window_fit.py)는 실제4c7d40/실제_ftol의 x87 계산·비교·분기·RET를 **80입력×두 정밀도=160행**으로 최초 관측했다.26객체/현재6필드/20child/9전역/메뉴 스택은 실제 설치한 메모리에서 다시 읽는다. 측정값/가상Push·Attach/메뉴pop/기본Attach/깊이는 명시 하위 장치이며 기대 배치를 원문에 공급하지 않는다. C++ 본체도160행 모두 일치했다. 두 x87 정밀도 차이0·90344명령 위치에는 장치/STOP 포함이다. 선택 중단은 일반 EH unwind 검증과 구별한다.

새160행과 보존된 HTML 상위250/Idle88행, 합계 **498행**을 읽기 전용 재생했고 모두 같고 변경0이다. 빌드1/2 종료0·관측 경고오류0, 실제 결합6검사·최종 집중 **112검사/실패0**, 전체 CTest **2747검사/실패0·305.48초(wrapper)**다. 집중/전체의 제품·검사 바이너리가 같고 originals1394개도 보호 기준과 일치한다. 보호/CD/AGENTS/기존 Native 추적 변경0이다.

최종 제품 SHA256: `db8e0fe62cb6347480532193cb3e85a7819b5f342cf22609b1c41c7d6540f4d7`

최종 검사 SHA256: `901eb6775b8b18ea997abe9c911a961fffb2edbc74175bed6a22037e404f8bea`

[제품 보고서](../../cpppj/html-window-fit-client-runtime-20261024.json)에 소스9/참조14/보존 Native6/새 Native3와 종료 원시78파일 SHA를 기록했다. 원시는 `extracted/cpp-html-fit-404-*`다.

## 다음 접합

메타4c83b0·factory4cf270·정상 Idle 등록/실제 링크와 기존 LoadingFields/0xc4 부분 미션 안내 교체가 남는다. **실제 기존 게임 화면의 full Uber에는 새 전체 HTML 부모를 아직 설치하지 않았다.** Production/Minimap 동시 제공자·새 부모/자식 먼저 생성→이전 반납·State/Core/Pool/Win32 경계와 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성 및393 Wide SaveOptions 원인은 미완료이며 /goal은 활성 상태다.
