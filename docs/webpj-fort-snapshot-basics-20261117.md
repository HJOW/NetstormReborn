# 현재 TypeNames/Deck snapshot과 실제 stream (WB04-E628)

기준선 `225553df`의 RawFortSnapshotBasics.cpp/h와 Session5825..5845를
이식했다. E627의 캠페인 공용 snapshot도 두 writer를 호출하므로 이 한
단위의 편집기 전용 제외를 교정했다. 해당 C++ 파일과 Session의 기준선
뒤 변경은0이다. 외곽 Session 공개/Load와 전체 snapshot 완료는 아니다.

## 구현

- TypeNames는 현재 manager/owner 진단 뒤 Create 반환 stream을 고정하고,
  State 조회가 바꾼 current로 쓰기 대상을 바꾸지 않는다. version0은 쓰기
  장치를 요구하지 않고 반환한다. count BYTE와 signed BYTE 이름 해시/NUL,
  각 이름/진단 뒤 현재 signed 타입 수 재조회를 보존한다. JS live getter는
  C++ const 참조를 대신하며 별도 동기화 사본을 만들지 않는다.
- Deck은 고정 슬롯의 owner0..8로 실제 Refresh/Get을 먼저 실행한 뒤
  현재 manager에서 섹션을 만든다. 최초 count만 BYTE로 쓰며 반복 조건은
  signed count를 매번 읽는다. 5BYTE 런타임 항목 중 앞4BYTE만 저장한다.
- count의 일반 writer와 인라인 BYTE/DWORD 쓰기를 분리했다. 용량 진단
  뒤 현재 data/pos, 출력 뒤 현재 pos/end 재조회·DWORD 감김과 부분 효과를
  유지한다. 임의 확장·버퍼 교체·실패 복구를 추가하지 않았다.
- 같은 실제 owner의 Template9·State 부모·현재 TYPE·생산 덱을 연결했다.
  반환 당시의 manager/노드/생성 수명을 현재 섹션과 구별하고 명시 token
  범위·반납 관문을 유지한다. 공개 loaded 검사는 후속 외곽 caller의 몫이다.

## 검증

신규4건은 최초 실행부터 모두 통과했다. 기존 원본2,060행의 signed hash,
현재 count와 callback 변경, State/stream/current·출력·trace와 정상/부분
중단을 전수 대조했다. 원본 관측을 새로 만들거나 기존 자료를 수정하지
않았다. 실제 Session owner 검사에서는 보호188TYPE의753BYTE 이름 저장,
State로 current 변경 뒤 원래 TypeNames 쓰기·Pool/난수 불변·수명 종료,
같은 실제 생산 덱의5번째 BYTE 보존을 확인했다. 실제 미션 시작/플레이나
전체 저장 성공으로 세지 않는다.

manifest는 원본 fixture 한 항목을 추가해808항목이며 실제 Session 결선은
직접 fixture 소비자로 등록하지 않았다. E627 부모와 대응표/manifest를 포함한
집중17건이 통과했다. 대응표1,160행/오류0, `git diff --check` 오류0이다.
전체 Node v22.23.3·v26.13.0은 각각5,979건 통과, 실패·취소·건너뜀0이다.
로그는 `extracted/webpj-e628-node22.log`·`webpj-e628-node26.log`에 보존했다.
v24는 사용자 지시대로 생략하고 호환을 가정한다.

## 다음

외곽 callback의 공용 snapshot에 필요한 내용물·객체/영역·Money/Mission·
CoreData 등의 실제 자식 직렬화를 도달 지점에서 연결한다. 그 뒤 같은
Session callback/공개 수명·Load를 계속한다. 부모 이외 writer의 제외
분류는 공유 호출 근거를 확인할 때마다 별도 교정한다.
앱 session0·실제 플레이 불가 경계와 기존 자연 완주/포획·희생·공중3종
새 전용 생성/생산/행동 검사 제외를 유지했다. 게임 창은 실행하지 않았다.
