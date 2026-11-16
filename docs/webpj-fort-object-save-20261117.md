# 객체/청크 snapshot writer와 실제 월드 검색 (WB04-E630)

기준선 `225553df`의 RawFortObjectSave.cpp/h와 Session2590..2623·
6005..6028을 이식했다. 캠페인 공용 snapshot의 Objects/Terr/Chaff도
같은 부모를 쓰므로 이 한 단위의 편집기 전용 제외를 교정했다.
해당 C++ 파일과 Session의 기준선 뒤 변경은0이다.

## 구현

- 단일 객체의 초기 float32 좌표만 고정한다. 실제 signed64 __ftol 절삭과
  불확정값·하위 DWORD 감김, 전투/미션/편집·scope의 독립 재조회와 가상
  robust/현재 섬 필터를 보존했다. 이후 TYPE·폭별 필드·version·owner·
  내용물은 원문 위치에서 현재 값을 읽고 캡처한 bytes만 외부 writer에 준다.
- 청크는 감긴 int<<4를 float32에 저장한 경계와 최초 origin을 보관한다.
  Begin→실제 MemStream marker99→0을 쓰지 않는 countWORD 예약→후보 순회→
  원래 data+offset의 WORD 확정 순서다. callback이 바꾼 현재 후보/물리
  상한·stream을 재조회하며 count 감김·부분 쓰기를 보존한다.
- 같은 실제 owner의 VT 허용/아홉 거부·벌룬 가상·현재 TYPE·레벨/가상80·
  섬 조회·기술/객체 공용 내용물 writer와 보관한 Template stream을 결선했다.
  청크별 실제 Pool/Hash/TYPE의 독립 RawSquidFinder를 만들고 기존
  TruncatePoint/Snap의 float 경계 변환을 재사용한다. Native 장기 가상
  대여는 후속 설치에 남기며 현재 typed 경로를 바꾸지 않았다.

## 검증

신규4건의 원본 재생은 객체3,294행·청크672행 전체다. 반환/정상·부분 중단,
stream·두 버퍼·raw50·전역/가상 응답·외부 trace를 대조했다. 청크 marker의
내부 MemStream 진입 trace 한 항목만 C++ 검사와 같은 사유로 제외했고,
marker BYTE/진단·pos/end/capacity·count 및 나머지 trace는 모두 비교했다.
원본 void 청크의 중단 EAX는 반환 계약으로 비교하지 않는다. 기존 관측은
수정하거나 새로 수집하지 않았다.

실제 Session 검사는 같은 VT/TYPE/섬/stream으로 종류85 객체의 좌표23h와
종류를 저장한다. 실제 해시의 빈/명시 후보 청크에서 marker·count·객체
접두와 Pool/해시·난수 불변, 설치 전 공개/옛 stream/반납 관문도 확인했다.
해시 후보는 독립 검사 입력이며 World Pop·실제 미션 적재 성공을 뜻하지 않는다.

최초4건은3통과/1실패였다. 결선 검사가 표준 `Terr00`을 `Terr0`으로 적어
파일 표준 섹션 조회가 빈 값이었다. 검사 이름만 고친 뒤4건 통과했고,
비어 있지 않은 실제 해시 결선도 더해 재검사했다. manifest에는 원본 두
fixture를 추가해811항목이며 실제 Session 결선은 직접 fixture 소비자로
등록하지 않는다. 앞선 writer들과 대응표/manifest를 포함한 집중24건 통과,
대응표1,160행/오류0, `git diff --check` 오류0이다.
전체 Node v22.23.3·v26.13.0은 각각5,986건 통과, 실패·취소·건너뜀0이다.
로그는 `extracted/webpj-e630-node22.log`·`webpj-e630-node26.log`에 보존했다.
v24는 사용자 지시대로 생략하고 호환을 가정한다.

## 다음

영역/Objects 전체 부모·Territory·Money/Mission·CoreData 등 snapshot의
나머지 자식과 실제 Template 결선을 이어 외곽 Session callback/공개·Load로
간다. 앱 session0·실제 플레이 불가와 기존 자연 완주/포획·희생·공중3종
새 전용 생성/생산/행동 검사 제외를 유지했다. 게임 창은 실행하지 않았다.
