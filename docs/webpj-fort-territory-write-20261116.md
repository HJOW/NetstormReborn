# 캠페인 공유 Territory 쓰기 (WB04-E588)

`225553df`의 `o/RawFortTerritoryRecover.cpp/.h`73줄 중 공유 함수
`WriteFortTerritoryRecords`(004bd600)와 Session의 `WriteEditorTerritory`
3489..3505를 옮겼다. 두 파일·Session·FortTerritoryRecoverTests는 기준선
뒤 변경0이다. cpppj 수정·디컴파일·게임/브라우저 실행은 하지 않았다.

## 범위와 실제 저장소

`RawCampaignVortex.inc:24`의 단독 섬 상승도 이 writer를 사용한다.
파일 전체를 편집기 전용으로 제외했던 분류를 `shared-territory-write`로
바로잡았다. 같은 파일의 `RecoverFortTerritory`(004b5ec0)는
ExistingEditor 경로 전용이라 웹 함수 범위에서
제외한다. 파일 상태 port는 이 공유 쓰기 범위의 완료를 뜻한다.

고정 Template 슬롯을 유지하면서 manager를 진단 전·Create 전·current 전
다시 읽는다. manager0은521, NULL 기록은522 진단이며 진단 복귀 뒤에는
원문 순서를 계속한다. Create 반환값을 사용하지 않고 현재 manager의
현재 스트림에 일반 MemStream Write120을 수행한다. 별도 지형 저장소나
성공 대체 장치는 없다.

Session adapter는 같은 CampaignTemplateStorage9의 실제 헤더·Create·
현재 노드와 생성 수명을 빌린다. 실제 선택/쓰기 실패는 이미 생성한 노드를
되돌리지 않는다. owner는 저장소를 빌리고 반납 후 호출을 거부한다.
이름의 Editor는 원문 함수 이름이며 편집기 UI/파일 저장 기능을 추가한
것이 아니다. 전체 Session은 계속 partial이다.

## 검증과 최초 실패

보호된 `fort-territory-recover-x86.tsv`974행 중 writer200행만 선택했다.
회복774행은 제외임을 검사에서도 명시한다. 입력만으로 외부 장치 메모리를
구성하고 제품의 실제 writer/일반 MemStream을 실행한 뒤 결과·헤더180BYTE·
manager/stream/버퍼 전체·미선택 State·입력 원문·호출 trace를 대조했다.
정상66건과 NULL 원문/manager0/521·522 진단 중단134건이 모두 같다.
관측을 새로 수집하거나 fixture를 수정하지 않았다.

신규5검사는 위200행 대조와 실제 Template/owner 통합이다. 첫 실행에서
원본200행과 정상 Template 검사는 통과했고, 빈 Territory를 읽기 선택하면
진단한다는 기존 계약을 무시한 검사2건과 필수 Unix 초 공급자를 빠뜨린
owner 검사1건이 실패했다. 제품 코드는 바꾸지 않고 빈 노드의 실제 cursor
관측·진단 단언과 명시 시각 공급자를 보완했다. 최종 집중5건 통과다.

manifest는805항목(original-x86 767)이며 기존 fixture를 참조한다.
완료 대상은1단위/73줄 늘었고 남은163단위/34,380줄은 같다. 웹 JS129,851줄·
검사758파일, 4-D 체크는31/46이다.

전체 검사 첫 실행에서 Node22·Node24는 각각5,599건 통과했다. Node26은
`player-core-records.test.mjs` 파일 단위가 상세 원인 없이 실패로 종료되어
5,597건 중5,596건 통과/1건 실패였다(하위3검사 결과 대신 파일 실패1건이 집계됨).
같은 파일의 단독 재실행3건은 모두 통과했고 이 파일/제품 코드는 수정하지
않았다. 원인은 확정하지 않으며 TAP 형식의 전체 재실행으로 최종 확인한다.
최종 TAP 전체 재실행에서 Node26도5,599건 통과했다. Node22(v22.23.3)·
Node24(v24.22.3)·Node26(v26.13.0) 모두 최종 실패·취소·건너뜀0이다.
전체 glob과 `--test-concurrency=4`를 사용했다. 대응표는1,160행·오류0이다.
로그는 `extracted/wb-e588-node22.log`·`wb-e588-node24.log`·
`wb-e588-node26.log`(첫 실패)·`wb-e588-node26-rerun.log`(최종 통과)·
`wb-e588-port-map.log`이며 Git에서 제외한다.

## 남은 경계·다음 시작점

다음은 생성자1537행의 `RawCampaignVortex.inc` 실제 바인딩이다.
현재 Template writer는 연결했으나 상승 명령/현재 Runtime/생산 갱신·
snapshot·Client 상태·전체 Load와 앱 수명은 잔여다. 사용자 확인 대상인
자연 완주·실제 포획/희생·특정 적 공격/승리·공중3종 전용 검사는 하지 않았다.
