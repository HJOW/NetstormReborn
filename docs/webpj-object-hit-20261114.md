# webpj 원본 픽셀 선택 (WB04-E408, 2026-11-14)

5-A RawCampaignFactoryDisplay가 요구하는 `client/RawObjectHit.cpp/.h` 69줄을 `core/raw-object-hit.js`로 이식했다. 기준선 `1f00a25a` 뒤 변경은 없다. 선택용 원색 버퍼를 사용하며 화면 사각형/표시 복사본/소유자 색 캡처로 대체하지 않는다.

## 순서와 저장소

- 첫 raw TYPE 번호를 저장한 뒤 실제 가상+78을 호출한다. 첫 결과가−1이면 현재 raw 프레임을 읽고, 그렇지 않으면+78을 다시 호출한다.
- 같은 현재188 TYPE/SHP에서 loaded·물리 프레임·flags1 0x440000의 두 배 범위·두 힙 채움 HP 패턴을 검사한다. 음수점/opaque보다 먼저 수행한다.
- 가상 효과 뒤에도 현재 raw를 읽기 위해 SidPool.physical의 살아 있는 보기를 사용한다. JS slot의 값 사본을 포인터처럼 쓰지 않는다. 선택한 signed SHP 헤더는 값으로 저장한다.
- 현재 float 위치와 같은 원본 카메라 투영·signed DWORD 감김으로 상대점을 구한다. 음수점이면 출력 없이 false, opaqueCollide이면 헤더 크기만 검사한다.
- 나머지는200×200 버퍼를255로 채우고 현재 extra bit2에 따라 raw 프레임 plain 또는 가상 main/추가Draw를 호출한다.200밖 점도 먼저 출력한다. 마지막 선택 픽셀의 **색255는 불투명 그림이어도 false**다.

## 검사

새 `tests/object-hit.test.mjs` **3/3 첫 통과**. 격리 초안3/3 뒤 실제 경로도3/3이며 제품/기대값 교정은 없다.

1. 원본 `object-hit-x86.tsv` **608행**의17개 입력을 같은 실제 풀/TYPE/SHP에 공급했다. 반환·getter/plain/+a4 사건 전체·40000BYTE 캡처 Adler와 raw 불변이 전부 같다. 픽셀 경계는 CPP 원문 관측 장치처럼 명시 입력 픽셀만 제공하며 RLE 출력 기대값을 재계산하지 않는다.
2. opaque의 그림 장치 미요구, 음수점/200밖·미적재 헤더·두 HP 패턴·프레임 두 배 허용·판본/표 길이/TYPE 범위를 구별했다.
3. getter 재진입으로 raw TYPE80→81/위치/extra를 바꿔도 처음 TYPE80을 사용하고, 선택 헤더/상대점은 이후 plain/추가 효과가 원본 헤더/TYPE/raw 위치를 바꿔도 유지한다. 가상 호출3회→plain→추가 순서와 효과 뒤 raw 변경 보존을 확인했다.

합성 TYPE80/81의 검사 저장소만 사용하며 실제 공중3종의 생산/생성/선택/행동 검사를 추가하지 않았다. 원본 자료 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3908/3908 통과**, 실패/취소/건너뜀0(310.5초/344.9초). 대응표 오류0, port658·partial58·todo180이다.

## 다음

83VT의 `ObjectHitVtables.inc`와 RawCampaignFactoryDisplay를 함께 이식한다. 표 데이터 자체를 이번 단위에서 완료로 세지 않는다. FactoryDisplay의 기준선 뒤 메타 추가Draw/선택용 실제 픽셀 장치 변경(`a0342fae`까지)은 해당 단위에서3.6절로 판정한다. 실제 입력/Session 연결과 자연 플레이 검증은 아직 별도다.
