# dotnetpj 0.549 — Content Form의 현재 정렬·필드·재부착

2026-11-15, HJOW-Athlon. R4-1의 큰 파일 `ContentFormHost.cpp/.h`를 현재 cpppj 제품1313과 재대조했다. 정렬 접두는 이미 원본41행으로 검증한 `RawContentAttach`를 사용하고, 현재 필드·진단·가상 재부착을 같은 몸체에 연결했다.

## 구현

PATCH 정렬 부착은 원문에 없는 선행 TYPE/생산 타입/포획 callback 검사를 제거하고, 부모/자신 dead 진단→현재 슬롯 재판정→실제 링크→Pop→이유1 통지 순서를 사용한다. CD는 기존 정렬 계약과 순서BYTE22를 유지하며 dead/free 입력은 무동작이다. 이전 종속 getter/setter는 TYPE로 배치를 추정하지 않고 현재 가상표의 공통 몸체를 사용한다.

TYPE 입력은 C++ vector처럼 레코드 값 사본을 보관하며, `BindCurrentTypes`로 실제 같은 표를 명시 대여할 수 있다. 통지 뒤에는 내용물 TYPE와 부모 genus부터 읽고 그 뒤 현재 전투 상태를 판정한다. `ContentFormState`의 기존 bool API는 유지하면서 `InBattleFlag`·`CheckingLinksFlag`로 현재 DWORD owner를 빌리도록 했다. 정상 복귀 진단과 현재 Content 타입 getter도 추가했다.

`Robust`는 TYPE/상태 필터 없이 목록 flags를 읽고 WORLD 진단이 복귀하면 현재 물리의 flags를 다시 읽는다. `Linked`는 현재 가상 부모 getter와 공통 전체 체인 검사로 자기 뒤 범위 실패까지 확인한다. `Unpop`은 선행 클래스 거부 없이 현재 공장 장치 또는 공통 해제로 간다. 양수 `Damage`는 부모 저장→Unpop→현재 flags의0x100 해제→그때 선택된 가상 재부착 순서다. count0/음수는 범위 밖 this도 읽지 않는다.

`BindCreate`의 마지막 부착도 생성/표면 통지가 바꾼 현재 장치·가상표를 선택한다. Content가 아닌 미연결 클래스를 정렬 Content 성공으로 바꾸지 않고, 생성과 필드 쓰기까지의 부분 효과를 보존한다. 이 API를 실제 NativeStorage에 장기 연결하는 것은 다음 작업이다.

## 검증

cpppj의 `ContentAttachCurrentTests`·`ContentLifetimeTests`·`FormTakeTests`에서 현재 계약 검사를 옮겼다. 신규17검사는 TYPE/상태 별칭96개, 진단 중 풀 재확보, count0/음수의 음수0 비트, dead 진단의 정상 복귀/미복구 무쓰기, 통지 뒤 현재 TYPE, 현재 DWORD 뷰, Content/Gump 재부착·선택 장치 실패·미지원 가상표의8경우, 실제 contained 생성 중 교체된 마지막 부착을 확인한다. 실제 사제 포획/희생 플레이는 실행하지 않았다.

최초 기존 관련12/12 통과 후 영향 범위를 넓힌78검사에서 CD 로더 하네스 입력2건이 실패했다. 기존 공장 하네스는 CD도188칸을 사용하지만 Content의 원문 계약은171개이므로, 해당 두 검사의 Content 입력만 판본 크기로 분리했다. 원본 기대값은 수정하지 않았다. 이후209/209 통과, contained 생성 중 현재 부착 검사2건을 추가한 최종 **211/211**이 통과했다. 기존 정렬41행·Form/Content 수명·실제 로더/공장 결선 검사를 포함한다. 자료는 `extracted/dotnet-content-host-0549-initial/`·`-final/`·`-final2/`·`-final3/`에 있다.

호스트 Release는 경고0·오류0이다. 전체 회귀는0.546 결과를 유지한다. L2는 바이너리 SHA가 같은 cpp34개 기준선을 재사용하고 dn31경로를 새로 실행했다. 신규31회 모두 종료0·수집 오류0이며 원본1,394파일의 이름·SHA는 전후 동일하다. dn31의 객체 상태는0.548과 같고 파일 전체 동일4경로·frame행만 다른27경로다. cpp와 전체 일치는0/34, 데모dn3경로는 미연결이다. `extracted/dotnet-l2-0549/`에 실행 인자·로그·재사용 출처와 `unit-comparison.json`을 보관했다.

## 인수인계

PORT_MAP898행(L1 425·port473), fixture303개이며 신규 원본 자료는 없다. `ContentFormHost.cpp` 대응의 기준과 신규 header행1개만 갱신했고 다른 대응·fixture해시·floor/reflected는 유지했다. 큰 현재 Host 몸체의 완료를 전체 Content 삭제/보상·NativeStorage 수명 연결 완료로 세지 않는다.

다음 R4-1은 Content 수명·삭제·보상 현재 몸체와 NativeStorage 장기 공장/공간/World 소유권, 제품 피해/해방·삭제/배치 상위 연결이다. 그 뒤 R4-2를 진행한다. 새 경로 플레이·M1 완료는 아직 아니다. 보호 원본/다른 프로젝트/금지 플레이 재현/MP 변경은 없다.
