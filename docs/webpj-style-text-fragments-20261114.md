# webpj StyleText 조각 저장소·소유권 (WB04-E418, 2026-11-14)

RawMenuControls 선행 **StyleText 519줄(.cpp/.h, partial→port)**을 완료했다. 기준선 `1f00a25a` 전체 소스·CPP HtmlTextParser_InlineStyleTextIntegration·기존 StyleText/메뉴 사용처와 [CPP 정적 복원 문서](exe/cpp-style-text-fragments.md)를 읽었다. 두 소스의 기준선 뒤 변경은 없다. **전용 StyleText x86 fixture/독립 PE 직접 대조는 없다.** 아래 검사는 기준선 계약·독립 치수/현재 응답·실제 글꼴/HTML 연결이며 이를 원본 직접 관측으로 표현하지 않는다. CD 분기는 웹10.78 범위 밖이다.

## 같은 조각과 소유 필드

- 기존 앱의 배열형 style-text.js는 보존하고 style-text-fragments.js에 실제 안정된 Map/단조 토큰·기본/문자열/외부 가상 조각을 옮겼다. head는 메뉴 항목 등 실제 필드의 읽기/쓰기 별칭이며 중복 소유 head를 만들지 않는다. 같은 owner/key 별칭은 같은 참조 객체다.
- NewText의 명시 길이/첫 NUL·문맥/본문 값형 복사, metadata를 길이로 쓰는 CloneText, 단일 Append와 전체 체인 Transfer를 구별한다. Clear는 삭제 전에 저장한 next로 순회하고 마지막에 현재 head를0으로 쓴다. 외부 자원/반납 callback과 예외의 앞선 상태도 보존한다.
- Measure는 높이→폭0 초기화 뒤 현재 head로 상승을 구하고, 현재 head부터 다시 순회한다. 각 baseline/measure 뒤 높이→폭 쓰기·현재 next를 유지하므로 출력이 서로/next/head와 겹쳐도 원문 순서대로 반영된다. Draw는 시작 head 값을 보관하고 최대 캐시 높이/상승을 구한 뒤 현재 가상 치수/next·signedDWORD 좌표로 출력한다.
- override 문맥의 나머지 값은 복사하되 font는 현재 조각의 저장 슬롯을 유지한다. 파서의 구간/리터럴~~·미인식 문자·끝 빈 조각·대괄호 현재 context/head/metadata와 ~. 복귀를 구현했다. 10.78의 제한 슬롯 명령/짧은 색·효과 명령은 같은8DWORD에 쓴다.
- 실제 GumpText/GumpTextDraw와 PaletteColorTable의기본 별칭/58명명색, 메뉴 항목의 현재 styled 소유 필드를 바인딩했다. 실제 RawMenuControls/파생 메뉴 조립은 후속이다.

웹 UTF16 정책은 명시 코드 배열로 유지한다. 기본 문자열은 Windows1252이고 UTF16 입력은 같은 코드 단위의 조각을 소유한다. Node Buffer도 표준 Uint8Array 보기로 정규화하여 .slice의 공유 참조/미초기화 꼬리 대신 실제 값형 복사·NUL을 보장한다. 파서 구간은 필요한 접두만 복사해 원문에 없는 전체 나머지 문자열의 반복 복사를 피한다.

## 검사와 최초 교정

처음 **8/9 통과**에서 마지막 검사의 혼합 글꼴 높이 기대값14가 잘못됐다. 일반 캐시는(높이14/상승11), 굵은 캐시는(31/26)이며 두 조각의 논리 높이는14다. 원문 전체 측정 높이는14−11+26=29다. 실제 캐시 메트릭을 각각 직접 단언하고 기대값을29로 고쳤다. **제품 몸체 교정은 없었다.** signed 감김/현재 Draw 링크 검사 추가 후 새 **10/10**, 기존 대화상자/도움말/그림·Button/Stone/Gump/메뉴와 함께 **151/151 통과**했다.

1. 중간NUL·명시 길이·metadata 길이/빈 복제, 실제 Buffer/문맥 소유 복사와 충돌 토큰 건너뜀·마지막 DWORD 토큰/소진을 확인했다.
2. 단일 Append/전체 Transfer·참조 동일성/자기 이전 거부, 반납 직전 기본 가상 상태·저장한 next와 callback이 바꾼 현재 head 정리를 확인했다.
3. 외부 가상 삭제/flags BYTE·기본 조각 빈 동작·순환/미등록/선택 장치/반납 실패의 앞선 상태 보존을 확인했다.
4. 파서 조각 본문/문맥/metadata·현재 bracket·미인식/닫히지 않은 명령/끝 빈 조각, 원문 색/효과·제한 슬롯·현재 default 복귀를 독립 기대값과 비교했다. 실제 PaletteColorTable은 기존 명령 색 표와 일치하고 현재/누락 명명색 계약을 지킨다.
5. 독립 외부 치수/현재 callback으로 Measure의 현재 head 재시작과 Draw의 값형 head·기준선/시작 높이를 구별했다. 높이/폭 공통 출력·next/head 별칭이 초기화/쓰기 후 실제 순회를 바꾸는 결과도 대조했다.
6. 실제 FontSet/메모리 캐시→GumpText→StyleText→GumpTextDraw/같은 픽셀·색/guard를 연결했다. 빈 슬롯2 override에도 저장 슬롯0의 실제 글리프를 출력하며 캐시 높이14와 그림자 측정 높이15를 구별했다. 메뉴 styled 소유 필드 Clear/Parse/측정 바인딩도 확인했다. 전체 MenuGumpLayout/RawMenuControls를 생성한 시험은 아니다.
7. CPP의 HTML inline 예제를 실제 기존 HTML 변환→새 조각에 공급하여 본문·edge 색77/스타일0·open 굵기2를 대조했다. 명시 UTF16/실제 UnicodeFontProvider는 래스터라이저 응답을 공급한 메모리 시험이며 브라우저 D2Coding 렌더링이 아니다.
8. Draw callback 뒤의 현재 next로 새 조각을 방문하되 반환 높이는 첫 순회를 보존하고, signedDWORD 폭/좌표 감김과 순환 거부를 확인했다.

원본 파일 변경·게임/브라우저 실행은 없다. 전체 Node v26.13.0·v22.23.3 각각 **3993/3993 통과**, 실패/취소/건너뜀0(328.5초/360.4초). 대응표 오류0, port675·partial53·todo168이다.

## 다음

StyleTextMarkup/Buffer·TextGump·ScrollingText·GumpDecoration과 파생 메뉴 처리기를 같은 저장소에 연결한 뒤 RawMenuControls를 조립한다. 기존 앱의 배열형 메뉴/본문을 새 수명으로 교체하는 단계와 ClientRendererFrame·GameWorld·ClientGameplayGlobals/Session 연결도 남아 실제 미션은 아직 플레이할 수 없다.
