# 실제 BackgroundGif 창과 공통 UI — 2026-10-24

현재/마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이다. 보호 PE SHA256은 `a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`, 정밀 소스 SHA256은 `af7cbf0b68e9d8e5aeb7068382c88d66201d3906c2926c54909ae14de4ad5646`다. 새 전체 디컴파일·원본 게임 직접 실행·GUI 실행은0이다.

## 분류와 원문 본체

Uber의 **0041a030/new0xb0는 BackgroundGif 창**이다. 기존 constructMessage의 HTML 주석을 바로잡았다. Tell의004cf120/new0x21e8 HTML 부모와 구별하며 HTML 전체 구현 완료로 쓰지 않는다. CD10.72 대응은 ctor00403630/SetEvent00403740/Refresh004037f0이며 이번 구현은10.78이다.

[BackgroundGifGump](../../cpppj/src/client/BackgroundGifGump.cpp)는 생성·Refresh·SetEvent·Reattach·Handle·Draw·소멸의 같은 현재 기본/파생 주소를 사용한다. 실제 vtable501b60과 signed kind54041e=3, 종료 사건+a0/a4/a8과 소유 bitmap+ac를 유지한다. Refresh는 이전 free/bitmap0 뒤 현재 설정/파일을 조회하고 실제 출력 폭·높이로 base/배율 치수를 쓴다. 소멸자는 free 뒤 bitmap을0으로 덮지 않는다.

419e40의 전체 입력 본체는 정밀 텍스트에 별도 진입이 없고419e9e 꼬리만 분리되어 있어 보호 PE/vtable을 직접 확인했다. release가 아닌 좌/우 클릭·ESC에서452f90에 세DWORD를 그대로 넣는다. 수정자는 원문 마스크로 처리하고452fe0의 Command 정규화를 추가하지 않는다. 나머지는 같은 기본 Help에 위임한다. Draw는 위치 조회 후 현재 bitmap과 **baseWidth/baseHeight(+3c/+40)**로 원시 복사하며 배율 치수44/48로 리샘플링하지 않는다. 실제 원문_ftol도 독립 관측에서 실행했다.

## 실제 파일·공통 UI

[RawBackgroundGifControls](../../cpppj/src/client/RawBackgroundGifControls.cpp)는 같은 Client VFS/ConfigInterface와 공통 Gump 등록표/부착/입력/수명/정렬/클립/픽셀 표면을 사용한다. 이미지 토큰72000000 영역은 실제 독립 소유 색 번호 버퍼의 주소 별칭이며 게임 결과 캐시가 아니다. 동시 창·교체·미부착/부분 생성·페이지 전환에서도 공통 Kill/Cleanup으로 실제 이미지를 반납한다.

[DecodeBackgroundGif](../../cpppj/src/client/GifImage.cpp)는407171/406ef7처럼 첫 descriptor의 폭·높이와 행을 사용하며 논리 화면 크기/좌상단 오프셋을 적용하지 않는다. 입력 사본에서 공통 LZW에 같은 첫 이미지 캔버스를 전달하고 보호 파일은 수정하지 않는다. 원시 복사는 GIF 투명 마스크/파일 RGB 변환을 추가하지 않는다. 배경 파일은 실제 titleMenu639×480, Defeat/victory640×480이다. 일반 Attach mode0의 화면 보정과 별도로 원시 이동 출력 검사에서는 명시 mode1을 사용했다.

## 독립 관측과 검증

- [상위 원문 도구](../../tools/decomp_background_gif_gump.py)는 실제7본문/_ftol과 현재14필드·분기·RET·장치 변경/선택 중단을 **154입력×두 x87 정밀도=308행**으로 관측했다. 두 정밀도의 결과 차이는0이다.14616명령 위치에는 장치/STOP도 포함된다. 기본 생성/파일/픽셀/큐/수명 하위는 명시 장치이며 전체 EH unwind 증거와 구별한다.
- [보호 자산 도구](../../tools/decomp_background_gif_assets.py)는 실제407171과406ef7/LZW를 실행해 **보호 GIF34개 전체**의 치수/색 번호 SHA256·FNV1a64를 관측했다.406939의 출력 행 복사만 장치 경계다. [헤더 변형 도구](../../tools/decomp_background_gif_variants.py)는 논리 화면/큰 오프셋/인터레이스/로컬 팔레트의 메모리 입력4개를 같은 원문으로 관측했다. C++ 기대 결과를 원문에 공급하지 않았다. 세 최초 자료 **346행**은 --verify 재생에서 모두 같고 변경0이다. 해독기 basicBlockEntries는 명령 수가 아니다.
- 실제 Client 결합 **6검사**에서 보호34개/변형4개 픽셀 지문·기본 경로·공통 실제 노드·이미지 교체·미부착/동시 반납·base 크기 출력·행 padding/클립·클릭/ESC/raw 큐·선택 실패/페이지 전환을 확인했다. 입력 경계의 명시 재진입에서 반납된 노드를 다시 읽지 않는지도 검사했다. 실제 출력 BMP를 직접 확인했다.
- 빌드1~5는 종료0이다. 중간 빌드4의 fill_n 정수0→BYTE C4244는 BYTE0으로 명시해 최종 빌드의 관측 경고오류0이다. 최초 검사의 flags1/폭640·Attach 보정 누락은 실제 원문/자산에 맞춰 수정했고 경과를 별도 기록했다. 최종 집중 **60검사/실패0**, 전체 CTest **2733검사/실패0·324.89초(wrapper)**다. 최종 집중/전체의 제품·검사 바이너리가 같고 originals1394개도 보호 기준과 일치한다. originals/CD/AGENTS/기존 Native 추적 변경0이다. AGENTS 네임스페이스 규칙은 별도 사용자 커밋eeec368d로 보존됐으며 이번 변경에 포함하지 않는다.

최종 제품 SHA256: `8cf5dc348c195e7dc23285650f6b1ab73640f714b6562971e6f0e9b7f99b19f7`

최종 검사 SHA256: `9bc1f20a93ca4b13d1a5f117d25a54bfe67413504ec2808eb07a5a7c1c7d54b4`

[제품 보고서](../../cpppj/background-gif-client-runtime-20261024.json)에 변경 소스14/참조10/보존 Native6/새 Native9와 종료 원시61파일 SHA를 기록했다. 원시는 `extracted/cpp-background-gif-402-*`다.

## 남은 연결

**실제 기존 게임 화면의 full Uber에는 아직 새 GIF/Chat/NSInfo 공장을 설치하지 않았다.** 전체 HTML 공장과 Production/Minimap의 기존/새 제공자 동시 소유, 새 Uber 부모/자식을 먼저 만든 뒤 이전 것을 반납하는 순서, Native State/Core/Pool·Win32 입력/clipboard/파일·전송 경계가 남는다. 모든 Early/캠페인/Demo 사용자 완주·Edit/언어별 시나리오 전체·완전 동일성 및393 Wide SaveOptions 원인은 미완료이며 /goal은 활성 상태다.
