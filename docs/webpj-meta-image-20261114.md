# webpj 동적 Image·MetaImage 노드·MetaImage 목록 RawImage/MetaImageGump/MetaImageList (WB04-E440, 2026-11-14)

RawMenuControls 선행 **RawImage 120줄·MetaImageGump 114줄·MetaImageList 152줄(기준선 `1f00a25a`의 .cpp/.h 줄 수, todo→port, 합계 386줄)**을 새 모듈 [`www/ui/raw-image.js`](../webpj/www/ui/raw-image.js)·[`www/ui/meta-image-gump.js`](../webpj/www/ui/meta-image-gump.js)·[`www/ui/meta-image-list.js`](../webpj/www/ui/meta-image-list.js)로 이식했다. 기준선 전체 소스를 읽었다. **전용 PE fixture는 없다.** cpppj에도 직접 시험이 없고 아래 검사는 기준선 소스 계약·손 계산 바이트/값·실제 `GumpCreate`/웹 `ShapeDatabase`/`GumpPixelTarget` 위의 동작으로 한 것이며 원본 직접 관측 수치가 아니다.

## 기준선 뒤 변경 (3.6절)

세 단위 모두 기준선 뒤 변경이 없다.

## 구현

- **`RawImage`(원문 0046d790/0046d900/0046d8a0)**: `convert(bitmap, crop)`는 행 우선 8비트 bitmap(0=투명)을 받아 불투명 사각형만 단일 프레임 VFX 1.10 shape로 인코딩하고 bitmap은 소비한다. crop은 오른쪽/아래의 마지막 nonzero까지만 남기며(왼쪽/위쪽 여백은 자르지 않고 전부 0이면 1×1) 머리 40바이트는 `1.10` 표식·프레임 수 1·표 위치 16·(높이-1, 폭-1)을 두 번·불투명 상자의 `(왼쪽-(폭-1), 위-(높이-1), 오른쪽-(폭-1), 아래-(높이-1))`을 DWORD로 감아 쓴다(빈 상자는 `INT_MAX`/`-INT_MAX` sentinel). 행 본문은 skip(`1,개수`, 255 단위)·repeat(`개수×2, 색`, 127 단위, 길이 2부터)·literal(`개수×2+1, 색들`, 127 단위, 같은 색이 세 번 나오는 위치 직전에서 분리, 중간 pair는 literal에 남김)·행 끝 `0`이며 행 끝의 투명 run은 버린다. `copyFrom`은 **먼저 파괴한 뒤** 복사하므로 자기 복사는 빈 이미지이고, `destroy`는 두 버퍼/폭/높이를 지우되 `shapeSize`는 보존한다. 입력이 잘못되면(폭/높이가 양수가 아니거나 픽셀 수 불일치) TypeError다.
- **`MetaImageGump`(원문 0047f2a0/0047cbd0/0047cc00/0047cae0, VT `0050d290`, 종류 21)**: `construct`는 공통 `GumpCreate`로 크기 0 기본 생성 뒤 Image/배율 1/색 모드 0을 쓰고, `resize`는 float32(폭+1, 높이+1)와 `×scale`을 먼저 정수+1을 float로 좁히지 않고 쓰며, `attach`는 기본 부착 뒤 Image/기본 크기 비율(각각 float32 저장)이 1을 넘으면 큰 비율의 역수(배정도)로 축소한다. `draw`는 shape가 없으면 위치 조회 전에 돌아오고, 배율 비트가 정확히 `0x3f800000`일 때만 기준점=위치+Image 크기·16.16 배율 65536·`mapped`=색 모드로(색 모드 비영이면 먼저 고정 비활성 색표 `00556038` 설치) 요청하며, 그 밖의 배율은 `ftol(배율×65536)`·`ftol(크기×배율+위치)`의 plain 변환이다. `bindMetaImagePixels`는 그 요청을 웹 `ShapeDatabase`(frame 0)와 `GumpPixelTarget.image`(단위, 프레임 rect만큼 어긋난 기준점)/`scaledImage`(확대·축소)에 연결한다.
- **`MetaImageList`(원문 0047ccb0~0047d1f0·0047f360, VT `0050d0d4`, 종류 22, 21칸)**: `construct`는 개수/이미지 배열/플래그/선택/열 수만 쓰고 ID 배열과 셀 크기는 이전 비트를 보존한다. `add`는 ID → 자식 생성 → 배열/개수 → 자식 부착 → 목록 Push → 현재 x/y 재부착 순서(생성 중 count가 바뀌어도 각 읽기 시점 유지)이고 21칸이 차면 false다. `attach`는 열 수 0 이하면 가로 한 줄(셀 너비=기본 너비/개수, 셀 높이=`min(기본 높이, 셀 너비)`), 아니면 열×행 격자(행 수=개수/열+(나머지>0))로 셀 크기를 정하고 매 자식을 Push→크기 float32 쓰기→float32 누적 좌표로 부착한다(자식 통지 중 줄어든 현재 count를 종료 조건으로 읽음). `hit`은 경계 밖(오른쪽/아래 제외)이면 -1, 가로는 `count×dx`를 32비트 정수로 곱해 기본 너비로 나눈 ftol, 격자는 `행×열 수+열`이며 개수를 넘으면 -1(같으면 그 값)이다. `handle`은 뗌 비트 없는 좌누름(`0x10000`)만 처리해 선택을 토글·Redraw하고 부모에 `0x02000004`로 통지한다(화면 밖 클릭도 현재 비교 결과로 통지). `draw`는 선택 셀을 절삭 좌표(가로: `ftol(선택×기본 너비/개수+왼쪽)`·`ftol(왼쪽+기본 너비/개수)`, 격자: 열/행×셀 크기)로 녹색 체크무늬 채우기, `id`/`image`/`remove`는 0..20만 허용(RangeError)하고 삭제는 Kill 예약·접두부 압축·count만 줄이며 선택/끝 슬롯 잔여 값은 건드리지 않는다.
- 웹 적응 표기: `std::span<const uint8_t>`→`Uint8Array`, `RawImage&` 소유 규약→JS 객체 참조, 10.78 판본 검사는 웹이 10.78 전용이라 생략, `logic_error`→`Error`, `out_of_range`→`RangeError`, `invalid_argument`→`TypeError`.

## 검사와 발견

새 시험 파일 4개, **28건 통과**: `raw-image.test.mjs`(8, 손 계산 40바이트 머리+토큰, 웹 `ShapeDatabase`로 되풀이 해독한 픽셀 대조), `meta-image-gump.test.mjs`(6, 실제 `GumpCreate`·`RawImage`와 실제 `GumpPixelTarget` 표면에서 프레임 rect·색표·확대/축소 확인), `meta-image-list.test.mjs`(11)와 `meta-image-list-edge.test.mjs`(3, 열 1 격자·홀수 경계·셀 높이 25·0번 클릭·잔여 ID).

**최초 실패와 교정(제품 결함 아님, 시험 기대값 오류 6건):** 불투명 상자 밖의 앞쪽 투명은 인코딩하지 않으므로 skip 토큰 시험에는 상자 안 양끝 색이 필요하다는 점, 127 단위 literal의 두 번째 조각 값, 중간 pair가 세 번이 아니면 literal에 남아 토큰이 9라는 점, `resize` 시험의 배율은 float32 값이어야 한다는 점, 격자 열 경계 `66.67`의 정수 좌표와 `dy/25` 행 번호를 바로잡았다(모두 C++ 순서를 다시 읽어 확인).

**변형 확인:** 연산자/정수 상수 변형 277개(raw-image 112·meta-image-gump 45·meta-image-list 120) 중 258개를 잡았다. 처음 생존 54개 중 35개는 보강으로 잡았다: 16비트 필드 상위 바이트(폭/높이 300), 행 끝 세 번 같은 색 literal 분리, crop의 첫 행/첫 열, 1×N 크기, 소멸 뒤 bitmap, 구성 플래그, 한쪽 비율만 1을 넘는 축소·1×1 Image, 목록 기본값·구성 뒤 크기·개수 0 부착 시 셀 크기 보존, 열 수 1 격자, 오른쪽/아래 홀수 경계의 마지막 칸, 셀 높이 25의 행 경계, 0번 칸 클릭, 삭제 뒤 잔여 ID. 남은 19개는 동등 변형이다: 배열 끝을 한 칸 더 읽는 변형(`undefined`가 투명/종료로 같은 결과), 곧바로 덮어쓰는 기본값/구성 인자(색 모드·종류·`command`), 폭/높이가 함께만 0이 되는 `&&`↔`||`, 비율이 같거나 정확히 1일 때 같은 결과가 되는 `<`↔`<=`/`>`↔`>=`, 같은 값이 되는 셀 높이 선택, 이미 켜진 플래그 비트, 마지막 자식 뒤 쓰이지 않는 `stepY` 초기값.

## 남은 경계

- Image를 소유하는 실제 창/ImageGump(TYPE 그림, 별도 단위)·실제 Gump 트리/화면 연결·RawMenuControls의 Image 목록 사용은 별도 단위다. 앱은 아직 간소화판이며 실제 미션은 플레이할 수 없다. 원본 파일 변경과 게임/브라우저 실행은 없다.
