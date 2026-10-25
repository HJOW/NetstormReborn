# TotalMade 세 집계 표의 실제 저장소 수명

2026-10-25 단위433. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 정밀본은2026-10-23이다. 이전432는bd9ea8d3의집중68개 검증 제품이다. 이번에는 기존 정밀C/PE와 Native104 fixture를 읽으며 새 원본 실행·디컴파일·Native 관측을 하지 않는다.

## 장기 물리와 지역 장부

`RawTotalMadeStorage`는005c94d0의local,005c98d0의누적local,005c9cd0의global 각256DWORD만 소유한다. 실제 `RawSurfaceStorage::totalMade`가 이를 장기 보유하고 World의 `SquidPostPopState`는 trailing 선택 포인터와 현재 배열 getter로 같은 물리를 빌린다. 기존 독립 상태의 값형 세 배열은 유지하며 두 배열을 사후 동기화하지 않는다.

실제 세 소비자는 PostPop의증가,공통삭제의local/global감소,RawTotalMade의조회·genus합산·초기화다. 누적local은 정상삭제에서 감소하지 않는다. actual World의 모든 집계 접근이 같은 getter를 사용한다. 타입/소유자 변경 통지 뒤의 현재 값 재조회와DWORD감김도 유지한다.

목록·ownerFactories9개·providers/factories·현재localOwner·깊이·비용·dirty·suppressed/graphsEnabled·로딩/AI/삭제 정책은 World 지역이다. Player binding callback을 장기 저장소로 옮기지 않는다. 다음 실제 Session은 새 목록에 같은 현재Player owner를 다시 바인딩한다.

## 부분 초기화와 대여

PATCH004c2450/CD0040ba90은local→누적local→global 순서로 각각 앞255DWORD만0을 쓴다. 각 index255는 보존한다. `RawTotalMade::ResetCounts`와 Session 없는 `ResetTotalMadeCounts(storage)`가 같은 작은 본문을 사용한다. TYPE/허용표·목록·제어상태·마지막칸을 쓰지 않는다. 전체State cleanup의호출자는 이번에 연결하지 않는다.

일반 `Acquire`의세256칸 전체0은 기존 새World의fresh 호환이다. Native 부분 reset의 재현이라고 부르지 않는다. prepared 획득/World 생성/반납/후반실패는 현재 세 표를 초기화하거나 rollback하지 않는다. 중복 대여도 쓰기 전에 거부한다. 세 배열의 주소는 같은Storage 수명에 남는다.

공유 장부를 복사하면 포인터도 같은표를 가리킨다. 기존 실제World/Session 검사에서 독립 기대값이 필요한 곳은 세 getter 결과를 각각 값으로 복사했다. 기존 기대숫자와Native fixture는 유지한다.

## 검증 범위

15소스를 Source1으로 동결한 **첫 빌드 종료0·컴파일 경고/오류0, 집중76개·실패0**이다. 신규6검사의 실제 실행을 이름으로 확인했다. 기존Native104의reset행을helper/공유두경로에서재생하고, 실제Pop/공통삭제·누적보존·감김,World 반납/다음prepared 수명,후반생성실패,Native255와일반256fresh의분리,실제Player9Factory 재바인딩을 통과했다. 새Native관측이나 자연미션완주의 증거로 세지 않는다. Pop/삭제검사는 보상·음향·UI의 외부효과를 선택하지 않는 명시입력을 사용한다.

보호1821파일·15소스·4실행 파일 SHA가 같다. 기존PostPop/삭제·지형·실제Session/Editor와 생산/기술 제한 회귀도 통과했다. [실행 보고서](../../cpppj/totalmade-storage-runtime-20261025.json)에 같은 제품 해시·실제 로그·원시 준비본 근거를 기록했다. 함께 대조한 선택/렌더러 함수와 두 화면 좌표의 주소 정정은 [장기 소유 계획](cpp-state-world-owner-plan-20261025.md)에 기록했으며 제품코드/기존fixture를 바꾼 것으로 세지 않는다.

전체 CTest와 긴 GUI는 기능 묶음으로 통합한다. 실제 State1 cleanup/prefix,사용자Edit편집·저장·테스트복귀,모든미션자연완주·진행저장/재실행·완전동일성은 계속 미완료다. 다른 장부/섬/청크/사제·커서/권한 전역의 수명도 별도 후속이다.
