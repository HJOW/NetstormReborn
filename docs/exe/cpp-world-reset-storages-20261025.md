# 섬·청크·사제 상태의 실제 저장소와 부분 초기화

2026-10-25 묶음434·435. 현재/마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 정밀본은2026-10-23이다. 이전 제품은 b4630287의 임시Game버튼 제거·집중14개/두화면비 GUI 검증 제품이며 마지막 전체통합은60a5c7de다. 이번에는 기존 정밀C/PE/Native fixture를 사용하고 원본 실행이나 새 디컴파일을 하지 않는다.

## 같은 물리와 실제 소비자

`RawSurfaceStorage`가 아래 데이터만 장기 보유한다. 실제 Session은 같은 참조/현재 getter를 사용하고 별도 지역 배열에 복사해 되돌리지 않는다.

| 데이터 | 실제 소비자 | 저장소 밖에 남기는 것 |
|---|---|---|
| Island3204BYTE=count4+100×32BYTE | RawIslandRuntime,지역 소유·생성·Builder·Load/Snapshot | 현재Player span·소유권 callback·finish/residence 실행기 |
| ChunkMap의mode/폭/높이와256×12BYTE | 실제 영역 배치·성장·소유·저장/Snapshot | 지형 실행기·Session 효과 |
| 사제 ownerState9DWORD | PostPop/Participation,승패,AI의현재사제판정,PlayerMenu span | 사제SID목록/count·예약·이동·실제WorldBindings |

Session의 chunks/islandBytes는 World Storage의 같은 객체/배열을 참조한다. 첫 사제 모듈을 사용하기 전에 ownerState를 연결한다. 준비된 상태를 덮던 생성자의 무조건 Island Reset은 제거했다.

## 정확한 초기화와 보존

Island의data-only Reset helper와 기존 Runtime Reset/Delete는 같은 기록 초기화 몸체를 사용한다. 100기록의 지정 필드를 쓰고 각 기록의padding2BYTE를 보존하며 마지막에count를0으로 쓴다. 기존 양판본Native 관측의 전체3204BYTE 기대를 유지한다.

Chunk Reset은 입력mode를 보관하고 현재mode1에서현재높이 바깥/현재폭 안쪽 순서로 각 활성셀12BYTE를0으로 만든 뒤 첫BYTE만7f로 쓴다. 물리주소는x×16+y이며 폭으로stride를 바꾸지 않는다. 비활성 물리셀은 보존하고 입력mode로 복귀한다. cleanup과Editor prefix가 이Reset을각각다시호출하고mode1을쓰는것은별도원문효과다. 이번에는 부모의 두 호출을 접합하지 않는다.

사제 data-only Reset은 유효한player1..8의 지정DWORD만0으로 쓴다. owner0·다른owner·SID목록/count를 지우지 않는다. 기존Participation.Refresh의검색후0/1/2갱신은 이 초기화의 대체가 아니다. PATCH00492240을 기준으로 하며 정밀본의CD대응표시00406e00은 다른Player해제몸체여서 같은Reset근거로 사용하지 않는다. 기존CD 사제PostPop fixture는 그대로 유지한다.

일반 Acquire의 섬 전체0입력/부분Reset·새ChunkMap·사제9칸0은 이전 일반 Session의fresh 호환이다. Native cleanup의 정확한 호출 순서나 최초할당 복원이라고 부르지 않는다. prepared 획득/World·Session생성/반납/후반실패는 현재 데이터를 초기화하거나rollback하지 않는다. 데이터 주소는 같은Storage 수명에 남는다.

## 병합과 검증 범위

원시434의12사본/신규9검사와435의13사본/신규7검사를 현재432활동수신·433집계표 코드에3way로병합했다. 충돌은 공통base가빈 독립삽입이었고 양쪽코드를 보존했다. 기존279검사 이름과 숫자기대/fixture도 유지했다. 독립 읽기검토에서 명확한 병합 결함을 발견하지 못했으며 정적검토를 실행통과로 세지 않는다.

**21소스 Source1의 첫 빌드 종료0·컴파일 경고/오류0, 집중121개·실패0**이다. 신규16검사의 실제 실행을 확인했다. data-only Reset·부분영역 보존·실제Session Load/Snapshot·사제효과후현재재조회/승패/참여·준비생성/반납/후반실패·일반fresh의분리와 기존Native/지형/저장/실제Session/Editor·432/433회귀가 통과했다.

보호1821파일·21소스·4실행 파일 SHA가 같다. 게임 SHA는 `9c0d1211f460639e88b9199c42ce6fba3bfd090b2cdebca27cf21c1f8251deaa`다. [실행 보고서](../../cpppj/world-reset-storages-runtime-20261025.json)에 실제 집중 로그/원시준비본/병합근거를 기록했다.

같은 최종 제품의통합batch1은종료0이며fresh전체CTest **2,931개·실패0**이다. 실제메뉴GUI 두실행·자연Early2→Tutorial3가통과했고세캡처에서안내/덱/미니맵/목록/다음미션을확인했다. 평상시Host Game버튼도없다. Early2는실제타이머1/상태주입false/기존진행값보존으로A~I·환불75·최종SP3075·다음초기SP2000·같은프레임안내를확인했다. 다음안내캡처의SP1700은설치후실제생산이진행된화면이다.

통합은429~435의최신source를7개보고서로합쳐고정하고같은최종4exe·driver/helper/PLAN·원본/CD/설정복원·실행당시보고서사본을감사했다. 새진행저장이나Early3진행,모든미션완주의증거로확대하지않는다. 원본실행/새디컴파일/push는하지않았다.

실제Tick→State1 전체cleanup/prefix,사용자Edit편집·저장·테스트복귀,전체Early/캠페인/Demo의자연완주·승패·진행저장/재실행·완전동일성은 계속 미완료다. 이번 저장소 연결만으로 그 완료를 주장하지 않는다.
