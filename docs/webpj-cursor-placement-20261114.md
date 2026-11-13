# webpj 커서 위치·표시 범위·배치 판정 (WB04-E401, 2026-11-14)

`client/RawCursorPlacement.cpp/.h`(기준선 `1f00a25a`, 228줄)를 `core/raw-cursor-placement.js`로 옮겼다. 기준선 뒤 소스와 `CursorPlacementTests.cpp` 변경은 없다. 6단계 선행 목록의 내려놓기·반환·취득/취소·위치 판정 **4/4 부품**을 마쳤다. 6단계 전체나 앱 연결 완료는 아니다.

## 계약

- `CursorPlacementState`: 배치 경고·허용 DWORD 네 개·현재 장애·Gump 수신 이벤트, 설명 여백16, signed 기준점X/Y·폭/높이 네 칸, 첫 그림의 signed WORD, 설명·독립 화면 원점 또는 현재 카메라 두 칸을 소유한다.
- `query`: 현재 설명/허용 → Canon 각 그림의 범위 → 다리/문자/받침 여백 → Gump 또는 월드 배치 → 연관 지역 → 현재 Process 전이 순서다. x87 중간값과 float32 저장, unordered NaN 비교, DWORD 감김, owner BYTE·signed 나눗셈을 원문대로 둔다. `describe`는 설명의 살아 있는 `{value}` 참조를 받아 C++ 문자열 참조를 대신한다.
- `sourceAllowed`: flags0x20일 때만 원래 연관 객체 지역을 조회하고 현재 Canon 패턴과 표면 지도의 지역 일치를 확인한다. 실제 유효 칸에 도달했을 때만65536칸 지도를 요구하며 일치 시 Advance 없이 끝낸다. SP 부족 판정과 구별한다.
- `prepare`: 본체→선택된 경고→선택된 비용 사각형 순서다. 첫 출력이 상태/현재 카메라 원점을 바꾸면 뒤 사각형은 변경된 값을 읽는다. 가격 좌표는 실제 첫 미리보기 raw의 signed 화면 WORD와 현재 TYPE 발자국을 쓴다.
- `bindCursorPlacement`: 수명·내려놓기의 준비/판정을 같은 몸체로 연결한다. 자산 바인딩은 실제 Canon 디코더와 SHP 헤더, 월드 바인딩은 같은 풀의 배치 파이프라인 경계와 실제 Renderer 변경 표를 연결한다. GUI·설명·지역은 호출자 경계를 보존한다. 현재188TYPE는 세션 소유 표를 명시 대여한다.

## 검사

`tests/cursor-placement.test.mjs` **4건, 첫 실행 전부 통과**:

1. 원본 `cursor-placement-x86.tsv` **573행**의 Query/Prepare/SourceAllowed 반환·17DWORD 상태·설명·모든 외부 사건이 일치했다. 빈/여러 Canon 칸, NaN·지도 경계·음수 그림 치수·32비트 감김·세 여백 분기·Gump 전이·효과 뒤 현재 상태를 포함한다.
2. cpppj 두 번째 검사(같은 준비 바인딩/선택된 의존 누락)와 웹 보강(판본·TYPE·Gump 부모·유효 칸에서의 누락 지도 거부).
3. 첫 출력이 바꾼 실제 카메라 원점을 뒤 비용 사각형에서 읽는 독립 입력. 전체 출력 좌표를 고정 기대값으로 대조했다.
4. 실제 원본 자산의 Canon 한 칸·SHP 추가 헤더, 잘못된 자산/프레임, 실제 Renderer 변경 표와 배치 경계·같은 풀·기존 GUI 보존을 확인했다. 이 검사의 배치 경계는 명시 입력이며 실제 전체 배치 파이프라인/Session 통합을 재검증한 것으로 세지 않는다.

원본/클론 게임·브라우저·실제 입력/음향은 실행하지 않았다. fixture를 수정하지 않았으며 정규화 SHA-256은 `92948f3001f745d088ec58fb8212f3a42e6c02514e1c69fef7e77bc3a12e742f`(파일574행)이며 manifest `cursor-placement`에도 기록했다. 대응표 검사는 오류0(port648·partial60·todo188)이다. Node v26.13.0·v22.23.3 전체 검사도 각각 **3867/3867 통과·실패/건너뜀0**(352.8초·386.9초, 종료 코드0)이다. Node v24는 미설치다.

## 다음

4-D A의 **ClientCursorGlobals**가 요구하는 수명/위치 상태가 갖춰졌다. 그 장기 소유자와 준비/취소 연결부터 진행하고, 사용자 선택 상태·미리보기 생성/표시·Session을 같은 저장소에 연결한다. ClientGameplayGlobals의 실제 대여/반납 검사는 아직 이월 상태다. Tutorial1 Load/Start와 실제 미션 플레이는 미완료다.
