# 사제 정기 이식 (WB04-E528)

`o/RawPriestRegular`(원문 494580: 사제의 사건 선택과 Carrier 부모 순서)를 `webpj/www/core/raw-priest-regular.js`의
`runPriestRegular`로 옮겼다. 재기준선(E498~E502)에서 추가된 `[rebaseline-port]` 단위이며 기준선 `225553df`와
현재 cpppj 소스가 같음을 확인했다.

## 옮긴 범위

- 사건 601: `count`가 0이면 3.0을 반환하고(후단 불필요), 양수이면 실제 가상 +10 삭제(`flags 0x100000`)를
  요청한 뒤 −1.0을 반환한다. `count`는 부호 없는 비트로 전달하며 601은 0인지 여부만 읽는다.
- 사건 602(회복)·603(추락)·604(의식 프레임)·605(기도): 기존 실제 고유 후단의 반환을 그대로 전달한다.
- 그 밖의 사건: 직접 42a750 Carrier 부모의 반환을 그대로 전달한다.
- 선택한 후단만 도달한 위치에서 요구하며, 미연결이면 명시 오류이다(선택되지 않은 경계는 읽지 않는다).

## 시험

`webpj/tests/priest-regular.test.mjs` 2건(`nan-exact.mjs` 선행 import):

1. 원본 관측 `priest-regular-dispatch-x86.tsv` **384행 전부**: 사건·count·payload 비트·외부 후단 반환 비트에서 반환 비트와
   호출한 후단(`destroy:<flags>`·`fall:`/`ritual:`·`carrier:<사건/count/payload 12BYTE>`)을 대조했다.
   **첫 실행에서 전부 일치**했다(cpppj 시험 `ClientPriestOwner_OriginalDispatch384`의 대응).
2. 601 count 0의 후단 불필요, 선택하지 않은 후단 미읽기, 경계 사건(600/601/602/605/606), 미연결 거부.

`webpj/tests/fixtures/manifest.json`에 `priest-regular-dispatch` 항목(원본 10.78, 384행)을 추가했다.

## 남은 경계

- 실제 고유 후단(회복·추락·의식 프레임·기도)과 Carrier 부모의 연결은 6·7단계의 사제/운반/의식 client 바인딩
  (`RawPriestWorldBindings`·`RawCampaignPrayer` 등)의 몫이다. 앱 수명에는 연결하지 않았다(PORT_MAP `port`).
