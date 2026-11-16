# 신전51h·섬 상승 완료 부모 (WB04-E587)

`225553df`의 `o/RawVortexDeparture.cpp`117줄·`.h`48줄 전체를 옮겼다.
두 파일과 `VortexIslandBindingTests.cpp`는 기준선 뒤 변경0이다. 후자는
RawVortexAscend의 섬 목록 연결 시험이며 Departure 직접 시험이 아니다.
기준선의 `cpppj/tests` 전체에서 Departure 직접 시험·원본 fixture를 찾지
못했다. 이번 검증은 소스 계약/독립 입력 대조이며 전용 원본 관측 일치로
표기하지 않는다. cpppj 수정·디컴파일·게임/브라우저 실행은 하지 않았다.

## 현재 물리와 원문 호출 순서

- 같은 PATCH Pool/Hash/현재 TYPE·ChunkMap·RawIslandRuntime·RawVortexAscend를
  빌린다. localOwner/server/revision은 `{value}` 참조이고 효과 함수는 값
  사본이다. 별도 섬/후보/지형 월드를 만들지 않는다.
- Fort 지역 조회는 float32→CRT `_ftol` 하위 signed DWORD 변환을 Fort
  진단보다 먼저 수행한다. 현재 표면 WORD/타입과 진단 뒤 현재 noIsland를
  읽고, 존재 진단이 복귀하면 물리100개 안의 signed 지역 BYTE를 반환한다.
- 6BYTE 기록 인수는 첫 진단 전에 고정한다. 현재 y행/x열 순서로 같은 섬
  항목의 record를 덮고 실제 생산 갱신을 호출한다. 기존 배열 주소를 바꾸지
  않는다. 자기 섬 저장은 원래 record 사본과120BYTE Territory를 구분한다.
- 일반 record는 첫 빈20슬롯 중 하나에 넣고, home record는 기존20행의
  home 비트만 지운다. read/write의 owner 값 인수는 고정하지만 마지막
  소유 섬 조회의 localOwner는 현재 전역을 다시 읽는다. snapshot 실패는
  이미 끝난 기록 쓰기를 보존하고 후속 owner/섬 정리를 실행하지 않는다.
- 표면 iterator는 반환 자산의 x보다1 큰 커서를 보관한다. 네 이웃도 이
  현재 커서에서 계산한다. 각 삭제 뒤 다음 이웃의 현재 genus를 읽으며,
  일반 finder는 후보 효과 전에 다음 SID를 확보한다. 추가 후보는 y−4 청크,
  첫 genus, 섬 조회 뒤 직접 청크 덮어쓰기와 보호 genus 거부를 유지한다.
- 51h는 원문처럼 헤더 검증을 추가하지 않는다. 짧은8BYTE 앞단, free 진단,
  없는 섬/완료 로그, 서버 중계, 현재 패킷 SID의 Regular, 최초 선택 신전의
  현재 owner와 소리 순서다. 중계/로그가 패킷을 바꾸면 원문 위치에서 다시
  읽는다. 삭제 후 revision DWORD 증가도 감긴다.

`requestEdit`는 원문의 하위 효과 경계이며 웹 편집기를 구현한 것이 아니다.
웹 소비자의 제외/대체 정책과 실제 저장/상태 장치는 Session 바인딩에서
처리한다. 연결되지 않은 효과는 선택된 위치에서 명시 오류다.

## 검증과 추적

신규13검사는 실제 Pool/Hash/ChunkMap/3204BYTE 섬 목록/Ascend를 사용한다.
현재 전역·함수 사본·패킷 변경·raw WORD/지역 BYTE·6BYTE 사본과 배열 주소·
기록 순서·home20행·표면 커서/삭제·추가 후보·진단/예외 접두·revision 감김을
독립 입력/명시 기대값으로 대조했다. 첫11검사 모두 통과했고 경계2검사를
보강한 최종 신규13검사도 모두 통과했다. 하위 VortexAscend/IslandRuntime의
기존 원본 관측 검사까지 포함한 집중21건도 모두 통과했다. 실패/단언 완화는 없다.

새 fixture는 없고 manifest804항목을 유지했다. `vortexascend`·`islandruntime`
계산 메타데이터만 갱신했다. 완료 단위는1개/165줄이며 port843·partial35·
todo128·제외154, 남은163단위/34,380줄이다. 4-D에 이 선행 체크 항목을
추가해30/45가 됐고 재기준선 잔여는64단위/4,747줄이다. 웹 JS129,805줄·
검사757파일이다.

최종 전체는 Node22(v22.23.3)·Node24(v24.22.3)·Node26(v26.13.0) 각각
**5,594건 통과, 실패·취소·건너뜀0**이다. 전체 glob과
`--test-concurrency=4`를 사용했다. 대응표1,160행·오류0이다. 로그는
`extracted/wb-e587-node22.log`·`wb-e587-node24.log`·`wb-e587-node26.log`·
`wb-e587-port-map.log`다(Git 제외).

## 남은 경계·다음 시작점

이 모듈의 몸체 상태는 port이며 실제 Session/Native 장기 저장소/앱 수명에는
아직 연결하지 않았다. 신전 상승 명령·전체 세션 저장/생산 갱신·Client 상태
전환과 L2/L3는 다음 통합에서 대조한다. 사용자 확인 대상인 자연 완주·실제
포획/희생·특정 적 공격/승리·공중3종 전용 관측은 실행하지 않았다.

다음은 `RawCampaignSession.cpp`1537의 `InstallVortexDeparture`를 제공하는
`client/RawCampaignVortex.inc` 바인딩이다. 실제 Template/Runtime/이름/전역
음향·설정/Tell·Regular·방송을 연결하고, 그 뒤 거주 구역 Dude/자산 처리기로
진행한다. 전체 Load·앱 연결은 계속 잔여다.
