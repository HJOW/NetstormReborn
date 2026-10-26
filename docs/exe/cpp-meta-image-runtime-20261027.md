# 전용 MetaImage의 실제 생성·기하·공통 UI 출력 — 단위476

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon / 2026-10-23**이다. 정밀 C와 원본/CD 파일은 수정하지 않는다. 앞 단위475의 동적 `RawImage`를 빌리는 전용 MetaImage를 복원하고 같은 실제 공통 트리에 연결한다.

## 원본 파생과 공통 소유권

`MetaImageGump`는 `0047f2a0`의 실제 기본 Gump 생성/등록 뒤 가상 표0050d290, 타입BYTE21, Image 주소, 배율1, 색 모드0을 쓴다. 생성 시에는 Image 크기를 측정하지 않으므로 기본/배율 크기는0이다. `0047cbd0`의 별도 측정은 현재 Image 폭/높이+1을 사용한다.

`0047cc00`은 기본 Attach를 먼저 호출하고 그 뒤의 현재 Image와 기본 폭/높이를 읽는다. 두 비율을 각각float로 저장하고 필요한 경우 큰 비율의 역수로 축소한다. 배율 필드의float 저장과 마지막 크기 곱셈까지 남아 있는 reciprocal 정밀도를 구별한다. 기본 부착의 통지가 Image 크기를 바꾸는 입력도 독립 관측과 비교한다.

`0047cae0`은 shape가 없으면 위치 getter 전에 반환한다. 정확한 배율1 비트일 때 Image 폭/높이를 현재 정수 위치에 더해 plain 또는 고정00556038 mapped 출력을 호출한다. 다른 배율은 실제 CRT 절삭의16.16 값과 기준점을 만들고 색 모드와 관계없이 plain 변환을 쓴다. UI 크기+1, Image 크기, VFX 헤더의 폭−1/높이−1 기준점은 각 원문 값으로 유지한다.

`RawMenuControls`의 실제 기본 저장소 조회·가상 Attach·수집/정렬·Draw·공유 클립·지연 삭제에 같은 MetaImage를 등록했다. 현재 Image의 RLE는 매 Draw에서 읽으며 캐시한 옛 shape로 대체하지 않는다. 비활성 색표는 기존 `BuildProductionColorMaps`와 보호 파일 팔레트를 사용하고 같은 VFX 색표 문맥에 설치한다. 파생 소멸은 빌린 Image를 해제하지 않는다. 목록/창이 Image의 소유자여야 한다.

## 독립 관측과 범위

`tools/decomp_meta_image.py`는10.78의 실제 생성0047f2a0/기본 생성004642c0, 측정0047cbd0, 기본 부착 뒤0047cc00, Draw0047cae0, CRT ftol을 실행한다. 총163입력×두 x87 모드=326행이며 실제 기하float 비트, 가상 표/타입/flags, 등록 count/serial, 위치→색표→Draw 인자 순서를 기록한다. 빈/후보/지도 크기, 기본 크기0, 가로/세로 제한, 음수·0·단위 인접 배율, shape 유무와 부착 중 Image 변경을 포함한다.

Native의 기본 Attach 효과·위치 getter와 색표/픽셀 소비자는 명시 경계다. 따라서326행은 Native raster 픽셀 관측을 뜻하지 않는다. 실제 C++ 공통 트리의 수집/클립/지연 소멸과 기존 RLE 처리기의 plain/mapped 픽셀을 별도로 비교한다. 원문 CODE는 변경하지 않는다. 10.72 대응 생성은00471880이며 이번 신규 확대 Native 관측과 전용 C++ 파생은10.78에 한정한다.

첫 빌드는 신규 검사 include 문자열 오류로 실패했다. 이 연결 구문과 한국어 주석2곳을 수정하고 원시 로그를 보존했다. 기존 검사 본문·입력·기대값은 유지했다.

## 최종 결과

- Release build2는 종료0·경고/오류0이다. 최종 제품/검사 소스2,820파일을 동결한 뒤 마지막 manifest와 SHA가 일치했다.
- 신규2개를 포함한 집중98고유/99관측·실패0이다. MetaImage2 + RawMenuControls14 + ImageGump3 + EditorImage2 + MenuCheck3 + EditorIsland4 + ExistingEditor58 + NewFort13이며, Native MetaImage 사례1개가 두 필터에서 실행됐다.
- 같은 제품의 전체 C++3,250개·실패0이다. 기존3,248등록 이름을 유지했다. CTest wrapper 대신 실제 검사 실행 파일 전체를 직접 실행했다.
- 원본326행을 생성한 뒤 CPU로 다시 실행해 fixture 전체 바이트가 일치했다. Native 범위와 실제 C++ 픽셀/트리 검증의 경계는 위와 같이 구별한다.
- 원본/CD/AGENTS·기존 검사/fixture3,175파일은 바이트동일이다. 기존 공통 메뉴 검사46,116BYTE 전체 접두부를 보존하고 마지막 include만 추가했다.
- 게임 SHA256은 `1856b2a8629998974c117c674aabc01bf33ce1870cba3383711390fcd999bb58`, 검사 실행 파일은 `6c3eeab0d4274f05af80f96f990448bd83dba1a88915116d6d6d96166b15cbb4`다.

명령·실제 종료값·로그 SHA·소스 manifest는 [실행 보고서](../../cpppj/meta-image-runtime-20261027.json), 원문 함수 범위와 관측 한계는 [Native 보고서](../../cpppj/meta-image-native-20261027.json)에 있다. 원시 로그는 `extracted/cpp-meta-image-476/`에 유지한다.

실제 게임/GUI/음향 장치는0회다. 다음은 목록의0047ccb0 생성/0047f360 추가/0047cf00 부착·행열 배치/0047cd90 적중/0047ce60 선택/0047d070 출력/0047d170·0047d1c0·0047d1f0 getter/0047cd80 소멸이다.4열21개 후보와 실제 원문 ID/선택을 연결한 뒤 Add Island 창의 생성/Refresh/Idle/소멸·단일 창 수명을 공통 트리에 조립한다. 전체 Edit/시나리오와 미션 자연 완주·화면/음향 동일성은 계속 후속 작업이다.
