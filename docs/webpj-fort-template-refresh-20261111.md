# webpj Template 객체 해석 갱신(RefreshObjectRecords) — WB04-E280 (2026-11-11, 4-B)

3.1절 4-B 의 **Template**(partial 389줄)에서 남은 부분인 `FortTemplate::RefreshObjectRecords` 를 `assets/fort.js` 의 `refreshFortObjectRecords(fort, types)` 로 옮겨 항목을 닫았다. 나머지 `Template.cpp` 의 `Parse`·`Section`·`ConvertType`·`NormalizedOwner`·`SectionNames` 는 WB03-U 의 `parseFort`·`fortSection`·`convertType`·`normalizedOwner`·`SECTION_NAMES` 가 이미 같다. 기준선(`1f00a25a`) 뒤 `Template.cpp/.h` 에는 변경이 없다(3.6절 ①). 게임·브라우저·cpppj 실행과 디컴파일은 하지 않았다.

## 1. 무엇이 비어 있었나

3.1절 항목 설명은 "cpppj `Template` 부모(구역 이름표·슬롯 수명)를 올린다" 였지만, 실제 `Template.cpp` 는 파일을 구조로 읽는 클래스일 뿐이고 구역 이름표(`RawTemplateSectionNames`, E19)와 manager 수명(`RawDataManager*`, `RawTemplateLifetime`)은 이미 따로 옮겼거나 뒤 항목의 몫이다. 웹 `fort.js` 에 없던 것은 **현재 원시 섹션에서 객체 해석 자료만 다시 만드는 `RefreshObjectRecords`** 하나였다. 소비자는 실제 Editor(웹 제외)와 cpppj `CampaignSessionTests` 의 준비 입력 도우미(`ReadyConstructionInput.h`: 원시 공장 섹션에 splice 한 뒤 재파싱)다. 후자는 4-D Tutorial1 준비 입력 단위에서 쓴다.

## 2. 구현

- `readTypeConversion`: 시작할 때 새 배열을 만드는 대신 같은 `typeHashes`·`typeConversion` 을 제자리에서 비운다(`length = 0`, `fill(0)`; cpppj `clear()`/`fill(0)`). 새로 읽은 fort 에서는 이미 빈 배열이라 `parseFort` 결과는 같다. 이전 파일의 해시/변환이 빈 새 `TypeNames` 로 넘어가지 않게 한다.
- `readObjectRecords`(새 내부 함수): `Chaff` 를 읽고 영역 섹션 20개를 같은 인덱스에 차례로 다시 쓴다. `parseFort` 도 이 함수를 쓰도록 바꿨다(`push` 대신 인덱스 대입, 결과 같음).
- `refreshFortObjectRecords(fort, types)`(내보냄): 변환표를 먼저 갱신한 뒤 객체를 읽는다. 파일 읽기와 manager cursor 는 바꾸지 않고, `technology`·`deck`·`subscriber`·`money`·`territory`·`sections` 는 다시 읽지 않는다. 오류는 던지며 **부분 갱신**이다(chaff·앞선 영역은 이미 바뀐 채, 뒤 영역은 이전 값). 원자적 갱신이 필요하면 호출자가 사본에 적용한다(cpppj 준비 입력 도우미도 사본 `candidate` 에 적용하고 성공 시에만 교체).

## 3. 검사 (`tests/fort-refresh.test.mjs` 6건)

cpppj 에는 이 함수의 전용 시험이 없어(소비자 시험뿐) 호출 계약을 직접 검사했다.

- 보호 원본 Fort **465개**(느슨한 433 + 아카이브 32): 읽은 그대로 다시 해석하면 처음 읽은 결과와 `deepStrictEqual` 로 같다(멱등).
- 원시 `TypeNames`·`Chaff`·`Terr00` 을 다른 Fort 의 것으로 바꾼 사본: 변환표·chaff·영역이 **재직렬화해 처음부터 읽은 결과와 같고**, 다시 읽지 않는 필드는 같은 객체 그대로다.
- 두 저장 번호의 해시를 서로 바꾸면(저장 구성이 같은 현재 타입끼리) 갱신된 변환표가 객체 해석에 쓰인다(변환표 갱신이 객체 해석보다 앞섬).
- 빈 `TypeNames`: 해시·변환표·플래그가 비워지고 같은 배열이 제자리에서 갱신된다.
- 손상된 영역 섹션(`Terr05` 에 청크 표지가 아닌 바이트): 오류가 나며 새 chaff 와 영역 0..4 만 갱신되고 5 이후는 이전 객체 그대로다.
- 섹션이 없는 Fort: 빈 chaff/영역·변환 없음.
- 변형 확인 12개 중 처음에 1개가 살아남았다: 갱신 순서를 뒤집은 변형(객체 먼저). 대부분의 Fort 가 같은 `TypeNames` 를 공유해 두 변환표가 같으면 순서가 결과를 바꾸지 않기 때문이며, 해시를 교환하는 시험으로 잡았다(두 번째 시도에서 서로 다른 저장 구성의 타입을 바꾸자 뒤따르는 바이트 해석이 어긋나 `num == 0` 오류가 났고, 저장 구성이 같은 짝으로 고쳤다). 나머지 11개(제거·플래그·배열 새로 만들기·영역 수/이름/순서)는 모두 죽었다.

## 4. 남은 경계

- Editor 호출(웹 제외)과 4-D 준비 입력의 원시 splice 도우미는 해당 단위에서 이 함수를 쓴다.
- 3.1절 4-B 는 10/30 이다. 다음은 **RawTemplateLoad**(81줄)와 client `ClientTemplateSectionNames` 이다.
