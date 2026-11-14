# webpj GIF 창 조립 RawBackgroundGifControls (WB04-E462, 2026-11-14)

기준선 `1f00a25a`의 `RawBackgroundGifControls.cpp/.h`를 `www/ui/raw-background-gif-controls.js`로, 허브 헤더(`RawMenuControls.h`)의 접합 구조체 `RawMenuTree`를 `www/ui/raw-menu-tree.js`로 옮겼다. 앞서 E444가 옮긴 `BackgroundGifGump`(생성·교체·종료 입력·원시 복사·소멸의 훅 본체)에 실제 Client 파일/설정과 공통 Gump 등록표·픽셀·수명을 연결하는 계층이다.

## 이식한 것

- `createRawMenuTree(parts)`: 돌 버튼이 쓰는 단일 공통 트리의 처리기 묶음이다. 필수 객체 9개(`create`·`life`·`factory`·`collection`·`tree`·`lifetime`·`events`·`input`·`stones`)와 필수 함수 4개(`token`·`node`·`attach`·`changed`)가 없으면 `TypeError`, 선택 3개(`background`·`baseRedraw`·`invalidate`)가 없으면 같은 Client의 변경 통지(`changed`)로 대신하는 `requestRedrawBase`/`requestInvalidate`를 둔다. 원본 상태/등록표 사본을 만들지 않는다. 허브(`RawMenuControls`) 본체는 별도 단위다.
- `RawBackgroundGifControls(tree, { files, config, dispatch, outside }, baseHandle)`: 원문 `new 0xb0`에 대응하는 고정 파생 저장소(`allocate`)와 전체 생성자(`create`), 노드/이미지/본체 조회, `draw`(현재 표면을 Draw 안에서만 빌리고 재진입 거부·예외에도 반납), `handle`(원문 입력의 전체 DWORD), `deleteScalar`(이미지 free → 기본 소멸 → 객체 free), `release`, `close`(등록표에 올라간 창은 dead가 아니면 Kill, 공통 Cleanup, 기본 생성 전에 실패한 미등록 노드만 직접 반납, 등록 노드가 남으면 오류).
- 파일은 현재 VFS의 존재 조회(`files.has`) 뒤에 `gifFileDimensions`로 두 치수를 **해독 앞에** 출력하고 `decodeBackgroundGif`(첫 descriptor 규칙)로 해독한다. 이미지 주소 별칭은 입력/Help/Chat 문자열과 구별되는 `0x72000000` 대역에서 단조 증가한다. 원문 raw blt는 GIF 투명 마스크/팔레트 변환/리샘플링 없이 색 번호를 그대로 `rawBitmap`으로 복사하며, 소스가 실제 할당 밖이면 `outside` 공급이 있어야 한다.

## 기준선 뒤 판정

두 파일 모두 기준선 뒤 cpppj 변경이 없다(base `1f00a25a`). CD 10.72 판본 검사(`edition`)는 웹 범위 밖이라 옮기지 않았다.

## 검증

- 원본 GIF **34개** 전체의 해독 색 번호가 cpppj가 원본 LZW 본체로 미리 관측한 독립 지문(FNV-1a 64·SHA-256·치수)과 첫 실행에서 모두 일치했다(`background-gif-assets-x86.tsv`). 입력 변형 **4개**(논리 화면/오프셋 변경·작은 캔버스·지역 팔레트·인터레이스)도 같은 방식으로 일치했다(`background-gif-variants-x86.tsv`, 원문은 논리 화면/오프셋을 무시하고 첫 descriptor·지역 표·네 인터레이스 패스를 쓴다). manifest `background-gif-assets`·`background-gif-variants`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- cpppj `BackgroundGifClientTests` 6건 중 5건을 옮겼다: 기본 `BackgroundGifSpec` 경로와 동시 창의 독립 이미지·교체·반납, 34 자산 독립 픽셀, 서술자/인터레이스 변형, 공통 위치/원시 base 치수 복사/클립/행 끝 패딩(`pixels[y*647+x]` 전체 대조), 원시 큐 세 DWORD와 수정자/해제/명령/일반 키 구별·콜백 뒤 재조회 없음, 없는 파일/잘못된 파일/페이지 전환의 반납. 여섯 번째 `ProbeMetadataAndCurrentSecondaryKeepDecodeFailureLifetime`은 Client의 파일 이름 고리(`FileNameOwner`)·보조 VFS 출처 전환을 쓰는 네이티브 전용 경계라 웹에는 없고, 그 안의 "이전 그림을 반납한 뒤 해독 예외가 나도 없던 파일로 바꾸지 않음"만 짧은 파일 시험으로 옮겼다.
- 독립 시험 3건: 조립 경계(생성 인자 거부·`outside` 공급·Draw 재진입/예외 뒤 표면 반납·미연결 원시 큐·이미 반납한 이미지 재반납), 호스트 이미지 주소 별칭 대역, `RawMenuTree` 필수/선택 멤버와 변경 통지 대체.
- 첫 실행에서 일치하지 않은 곳은 시험 장치 두 곳뿐이었다(허브가 없는 시험 장면에서 두 번째 컨트롤을 만들면 공통 노드 조회가 첫 컨트롤만 봄, 이중 반납 시험은 한 번 더 부르는 것이어야 함). 모듈은 고치지 않았다.
- 변형 확인: 자동 변형 26+10개와 추가 확인으로 생존은 모두 동등 변형이다: 주소 별칭 소진 상한(`0x72ffffff`, 1,600만 개가 필요해 시험 불가), 원시 복사 클립의 `±0x80000000` 경계(어느 값이든 제한 없음), 기본 생성 인자 `command/width/height`(이후 `construct`/`refresh`가 덮어씀), 등록표 순회 끝 한 칸(`undefined` 노드는 건너뜀).

## 남은 것

앱 경로(`main-menu`·Credits 배경)는 아직 이 컨트롤을 쓰지 않는다. 허브(`RawMenuControls`)의 `EnableBackgroundGifs`가 이 클래스를 설치하고 공통 `RawMenuTree`를 만드는 결선은 허브 단위에서 한다. 실제 화면 비교(구름 배경 4절)는 ClientRendererFrame 이후다.
