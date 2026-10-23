# ColorSort raw primary 생성자439de0 독립 복원

단위388 준비·**공용 미등록/Client 미연결**이다. 현재/마지막 전체 정밀 디컴파일 PC는 HJOW-Athlon /2026-10-23이다. 새 [RawColorSortPrimary.h](../../cpppj/src/o/RawColorSortPrimary.h)/[cpp](../../cpppj/src/o/RawColorSortPrimary.cpp)는 실제0x3800 기록을 소유하고 같은 +8 저장 명도를 읽는다. 기존 제품/CMake/LEFT/git/공용 build/lib/GUI·게임·원본/CD는 변경하거나 실행하지 않았다.

## 기존 복원과 재사용 근거

기존 `BackgroundOracle.call/maps`는 실제439de0 전체와 정상 ABI·x87 균형을 실행하고 raw0x3800를 읽어 입력 불변을 검사한다. `decomp_colorsort_fixed_input.py`도 이 같은 원문 실행기를 사용하지만 기록은 전체9표와 raw SHA다. 따라서 [새 도구](../../tools/decomp_colorsort_primary.py)는 기존 실행기/원본 함수 범위/가상 메모리 감사를 바꾸지 않고 생성자만 좁혀 재사용했다. 기존6관측·캐시130관측·HSL15관측의 도구/fixture/report는 그대로다.

제품 `ParticleGradient`는 같은 원문 정규화 상수 `0x3f70101010101010`과 저장 RGB 의미를 이미 사용하지만 전체 HLS/56BYTE 물리/padding 소유자는 아니다. PaletteShade/Sprite/Preview/Owner의 RGB 재계산도 전체 생성자 재사용으로 세지 않았다. 이번에는 이 독립 원문에 맞춘 작은 생성자 몸체를 새로 분리했다.

## 제품보다 먼저 고정한 원문

[최초 fixture](../../cpppj/tests/fixtures/colorsort-primary-x86.tsv)/[Native 보고서](../../cpppj/colorsort-primary-native.json)는 **80관측/정상36·명시중단44/1,656,884명령**이다. 요청 x87 CW는 실제 `027f`와 `037f`이며 쓰기 직후 읽어 같은 제어어인지 확인했다. 생성자만 실제 실행했고 게임/할당 부모/Screen/검색 객체는 실행하지 않았다. [최초 고정 SHA](../../extracted/colorsort-primary-20261024-isolated/native-first-freeze.json)는 제품 파일 작성 전에 만들었다.

| 최초 자료 | SHA256 |
|---|---|
| tool | `bb9957d5e29ea5573efb6accdb42f8257f79175b8702d003d26f2a79e97e4b42` |
| fixture | `ddc522d49ab538e33c27661efcc4198f27a2e1e581bfa92de44020acec6ebc29` |
| report | `fcd08999171286a49ec3adbde480fb927a2ba8eae37aa6663b9c30e0399ab019` |

기존 실제/혼합/회색 입력과 추가 HLS 분기/흰색/혼합 입력을 사용했다. 입력 pointer는 외부 또는 this+0/this+24의 실제 alias다. 원래 비영 할당 전체를 채운 뒤 alias 입력을 올린 실제 초기 물리를 fixture에 보존했다. 명시중단은 N개 raw store가 완료된 **다음 명령 전**이며0/1/2/3/4/1023/1024/1025/1026/1791/1792를 관측했다. 중단 행의 GP/x87 스택을 정상 반환 계약으로 세지 않았다.

원문 두 정밀도의 정상 결과를 raw 필드별로 비교하면 hue409·saturation451항목이 다르고, 명도/R/G/B/index/padding 차이는0이다. **037f는 독립 원문 증거**다. 제품 지원은 기본53비트/nearest이며64비트 연산을 구현했다고 주장하지 않는다. 기존6관측의037f 아홉째 변환표 차이도 원래대로 보존한다. raw 명도가 같았다는 관측은 후속 검색/표 계산까지 두 정밀도가 같다는 뜻이 아니다.

## 실제 기록 배치와 API

| 항목 내 offset | 물리 |
|---|---|
| +00/+08/+10 | H/L/S double |
| +18/+20/+28 | R/G/B double |
| +30 | 원래 색 번호 DWORD |
| +34~+37 | **미작성4BYTE**, 최초 할당 값 유지 |

실제 순서는 첫 전체256순회에서 **R→G→blue BYTE 캡처→index→B**, 두번째 전체256순회에서 **H→L→S**다. 정상 raw store는1792개다. 같은 입력이 raw를 alias해도 선행 사본을 만들지 않고 읽기/쓰기 순서를 보존한다. 예약 X BYTE는 정규 RGB에 쓰지 않는다. 유효 BYTE 입력에서 원문 assert 도달은0이다. 부정 pointer/부족한 실제 메모리/입력 자체를 바꾸는 외부 observer의 동작은 지원 범위로 가장하지 않는다.

`RawColorSortPrimary(allocationBytes)`는 **실제 초기0x3800BYTE를 필수로** 받아 같은 payload에 복사한다. 기본0 채움/자동 COL fallback은 없다. `Construct(logicalRgbx, hooks)`가 첫1024BYTE를 현재 읽으며, `Bytes()`는 같은 payload 주소/부분효과를 빌린다. 복사/이동은 금지한다. `Lightness(index)`는 완료한 현재 payload+index*56+8을 memcpy로 읽을 뿐 RGB에서 명도를 다시 만들지 않는다. host `Constructed()` 표식은 payload 밖이며 원문 DWORD가 아니다. 잘못된 span 폭/번호와 미완성 getter는 host 물리 계약 오류로 명시 거부한다.

선택 `checkpoint`는 진입(0,0,0)과 각 store 뒤(완료수,offset,size)의 **외부 관측/중단용**이다. 원문에 callback이 있었다는 뜻이 아니다. 입력/기록 변경은 계약 밖이며 예외 때 이미 쓴 payload를 되돌리지 않는다. 정상 제품 경로는 observer가 필요 없다. 생성자 ABI·x87 상태를 C++가 직접 복제한다는 주장과 raw 물리/53비트 연산 결과 대조를 구별한다.

## 검증과 다음 접합

[ColorSortPrimaryTests.cpp](../../cpppj/tests/ColorSortPrimaryTests.cpp)의3검사는027f40관측(정상18/부분22)의 전체 payload·모든 store 순서·alias·미작성padding과 실제 +8 getter4,608비트를 대조한다. 037f40행은 별도 원문으로 보존해 필드 차이 수를 확인하며 제품 기대값에 섞지 않는다. 명시 초기 물리/같은 owner 재생성/getter 범위도 검사한다.

격리 MSVC14.51.36231/SDK10.0.26100 `/MD /fp:precise /W4 /WX` [첫 빌드](../../extracted/colorsort-primary-20261024-isolated/build1.log) 종료0·경고오류0, [첫 실행](../../extracted/colorsort-primary-20261024-isolated/test1.log)은 **3검사/실패0**이다. `--verify`도 최초80행을 통과했다. [최종 SHA/검증 목록](../../extracted/colorsort-primary-20261024-isolated/verification.json)을 따른다. 공용 테스트/실제 Client 실행 성공으로 세지 않는다.

다음 ClientColorSortInput은 최초 logical 캡처와 **같은 shared RawColorSortPrimary owner**를 보유해야 한다. 실제 allocation/439de0 완료 시점에 연결하고, Building5종은 그 owner의 저장 +8을 읽어야 한다. Scene palette 변경/Screen 교체/9표 갱신은 primary를 다시 만들지 않는다. 기존 RGB/9표 fixture를 고쳐 결과를 숨기지 않는다. `43b430`의 primary DWORD·검색 객체43a8e0/qsort·gradient·cache 부모/등록 수명 전체는 이 모듈로 완료되지 않았다. 기존 [Cursor 최소 사본](cpp-session-color-input-next-20261024.md)과 다른 후속 통합 범위다.
