# 768 — DrawWater 일반 명령과 실제 후단 표시

2026-11-03 HJOW-Athlon. 마지막 전체 정밀 디컴파일 HJOW-Athlon / 2026-10-23 유지. 단위768 완료이며 전체 목표는 미완료다.

## 현재 상태와 실제 프레임

DrawWater(542760/0200001a)는 물 배경 옵션이 아니라 원문의 후단 진단 표시다. 현재 이름/HTML 허용표/FIFO와 Guide/Gump/공통 release 뒤의 일반 입력에 연결했다. HelpOpen/Console/정지/Editor/World 조건을 실행 관문으로 추가하지 않는다. 원문처럼 worldRedraw=1을 먼저 쓰고 현재 비영 전체 DWORD를0으로 반전한다. flag는 Client의 같은 장기 소유자에 BSS0으로 만들고 Session/Renderer 교체로 다시 지우지 않는다. 기본 debug UI나 설정 reader를 합성하지 않았다.

실제 일반 프레임은 Renderer Draw/잠금 반납/기존 Finish → 연속 캡처 → DrawWater → Present → 기존16DWORD snapshot 순서를 유지한다. nativeFrame && captureFrame일 때만 새 후단을 실행하며 loading의 즉시 PresentScene(false)는 별도다. sprite/overlay/dirty 사각형으로 대신하지 않아 dirty0이나 후보0개라도 flag가 비영이면 전체 viewport를 한 번 출력한다. 다른 네 후단 진단은776에서 이어간다.

## 원문 계산과 장치

같은 RawSurfaceStorage Pool/Hash/TYPE와 기존 RawSquidFinder의 네 단계 Begin/Next를 사용한다. 가시영역은 현재 Renderer world의 ftol과 전체 화면 크기의 signed 셀 나눗셈으로 구하고, lower/upper snap의 float32 저장도 보존한다. 후보 +0e/+12는 별도의 이전 snapshot으로 투영한다. halfX/halfY도 snapshot의 signed 셀 크기/2다.

첫 사각형의 left=right=X−halfX, top=Y−halfY, bottom=Y−halfX를 일반 bounds로 고치지 않았다. 처음 clip에 그 특이 사각형을 그대로 쓰고 세 번 현재 파랑 DWORD를 읽으며 한 픽셀씩 확장한다. 중간 flag0은 이미 시작한 호출을 중단하지 않는다. rectangle wrapper는 현재 clip 저장 → 전체 viewport clip → Lock → 상/하/좌/우 VFX line → Unlock → clip 복구다. 마지막 기존 Screen::PresentPaletteClear는 실제4a46d0의 전체 viewport 출력/clip 복구이며 팔레트를 바꾸거나 Unlock을 대신하지 않는다.

분모0/INT_MIN÷−1은 host domain_error로 거부한다. DrawWaterRectangle 내부 예외는 잠금/clip을 반납하고 앞선 픽셀이나 flag를 되돌리지 않는다. Screen.Lock 자체의 depth 증가 뒤 장치 부재 예외는 이 try 밖이며 새 검증 범위가 아니다. 정상 프레임은 앞선 동일 화면 잠금이 성공한 뒤 후단에 진입한다. 기존 Windows10/11 DIB 호환 출력을 재사용하며 원문 DirectDraw 표면 ABI를 가장하지 않는다.

## 증거와 한계

원문 전체 표시76입력/152관측(실제 rect786회), 일반 입력384/768, 이름/FIFO9행, 전체 프레임 부모5/10을 고정했다. root의 추가 관측이 세 fixture 및 프레임 효과/현재 snapshot과 일치했다. Native finder/투영/rect/VFX는 실제 몸체이며 Lock/Unlock/Present는 메모리 장치다. 프레임 부모는 하위 효과를 기록하는 명시 경계여서 실제 OS/완전한 전체 프레임 관측으로 합산하지 않는다. generic TYPE/SID만 사용했고 새 MP/공중3종 전용 관측은 없다.

첫 집중5개는 통과했지만 검사 std::fill의 int0→BYTE에서 C4244 경고2종이 나왔다. 입력값을 바꾸지 않고 uint8_t{0}으로 명시했다. 최종 Release 경고/오류0, 집중 **5개(새5개)/실패0**. 전체 픽셀/clip/색과 현재 값 변이·원문 입력/이름·장기 flag 주소/값·flag0 미도달·dirty0 전체 viewport 출력/clip 복구를 확인했다. 새 실행대상0/기존66개·전체71대상을 유지한다.

기존 보호 2689개 모두 불변, 새 fixture3 포함 2692개/fixture872. 다음 기준은 extracted/cpp-resume-768/protected-after.json이며 제품 SHA-256은 `6c9dac754d21f8da5a72152b63c6d4167beba1f895fbbcbf6e9f3e58aab1b7c2`다. [보고서](../../cpppj/draw-water-frame-20261103.json)와 같은 extracted의 빌드/집중/최초·추가 Native 자료를 따른다.

새 게임/GUI/CLI/MP·물리/PCM·공중3종 전용검사·포획/희생·특정적공격·승리·자연완주·전체C++/34/ZIP 검사는0이다. 기존 sceneVisible/fullscreen 정책과 전체 외관/물리 출력은 별도다. 다음은769와 각 reader/생성/일반 AI 명령·호환성이다.
