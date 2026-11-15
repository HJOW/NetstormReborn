# 장식 Regular 이식 (WB04-E527)

`o/RawDecorationRegular`(원문 462420: 장식·가장자리 팜의 단계별 HP 감소와 마지막 삭제)를
`webpj/www/core/raw-decoration-regular.js`의 `runDecorationRegular`로 옮겼다. 재기준선(E498~E502)에서
추가된 `[rebaseline-port]` 단위이며 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.

## 옮긴 범위

- `event`가 0일 때: signed `count`가 3 이상이면 서버 권한(`boss`)일 때만 현재 가상 +10 삭제(flags 0)를
  요청하고 −1.0을 반환한다. 아니면 최대 HP(4adcb0, 현재 TYPE/제단 역할 조회)를 먼저 얻은 뒤 현재 슬롯의
  signed HP(+26)를 읽고, 최대 HP × (3 − count)를 32비트로 감아 곱한 값의 0 방향 정수 /3을 한계로
  삼아 한계가 현재 HP보다 작으면 가상 +98(amount = 현재 − 한계, owner 0, attacker 0)을 요청한다.
- 그 뒤(사건이 0이 아니어도) 나머지는 실제 44b900인 `damageable`의 반환 payload를 그대로 전달한다.
- `boss`는 호출 시점의 현재 서버 권한이며 boolean 또는 현재 값을 돌려주는 함수이다(C++ `const bool&` 대응).
- 선택된 가상 하위는 도달한 위치에서만 요구하고 미연결이면 명시 거부한다(권한 없는 종료와 비0 사건은
  최대 HP/삭제/피격을 앞당기지 않는다). CD 10.72 풀은 거부한다.

## 시험

`webpj/tests/decoration-regular.test.mjs` 2건(`nan-exact.mjs` 선행 import):

1. 원본 관측 `decoration-regular-x86.tsv` **630행 전부**: 입력 7 DWORD(사건·count·payload 비트·서버 권한·
   최대 HP·현재 HP·최대 HP 효과의 가상 변경)에서 반환 비트·효과 순서(`M`·`H/h`·`D/d`·`B`)·최종 슬롯 50BYTE 전체.
   payload에 NaN 페이로드(`0x7fc01234`)와 −0이 있어도 **첫 실행에서 전부 일치**했다.
2. 권한 없는 종료·비0 사건·함수형 boss·signed count(`0xffffffff`)·미연결 삭제·CD 풀 거부.

시험 장면에서 처음 막힌 두 가지: SID 50이 할당되지 않아 `allocated(50)`이 거부됐고(5~50 할당으로 해결),
원본 입력이 상태 BYTE까지 `0xa5`로 채워 이후의 `allocated(50)` 확인이 실패했다(할당 직후의 살아 있는 뷰를
한 번 얻어 계속 쓰도록 바꿨다). 모두 시험 장면 문제이며 이식 본체의 결함이 아니다.

`webpj/tests/fixtures/manifest.json`에 `decoration-regular` 항목(원본 10.78, 630행)을 추가했다.

## 남은 경계

- 실제 호출자(장식/가장자리 팜의 `RawAssetRegular` 분배와 `RawCampaignAssetBindings`)와의 연결은 4-D C의
  `RawCampaignAssetBindings`에서 한다. 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
