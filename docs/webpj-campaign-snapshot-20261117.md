# 전체 Fort/Battle snapshot 결선 (WB04-E636)

기준선 `225553df`의 `RawCampaignSession.cpp` 5903..5910·5920..5982,
합계 71줄을 옮겼다. 해당 `.cpp/.h`는 기준선 뒤 변경이 없다. E633의
공용 Money/Mission/Badges, E635의 현재 CoreData와 앞선 기술·객체·영역·덱
writer를 같은 Session의 Template 저장소에 연결한다. Editor 전용
`editorWorldReady` 예외와 파일 저장 UI는 웹 범위 밖이다.

부모가 만든 stream의 manager/노드/생성 수명을 호출 끝까지 보관한다.
각 child에서 현재 manager와 owner를 다시 읽으며, Technology 부모는
Create 전에 보관한다. Money/Mission의 자체 Create 부모를 재호출하지
않는다. Mission 서비스 누락은 Mission 노드를 만든 뒤, Badges 서비스
누락은 최초 count BYTE를 쓴 뒤 첫 getter에서 거부한다. Battle은
TypeNames/Badges를 부르지 않고 CoreData를 마지막 통지 앞에 둔다.

신규 3검사 중 2건은 **최소 owner 계약 검사**다. 실제
CampaignTemplateStorage·GameRandom·Money/Mission/Badges writer를 사용하고,
상위 월드 child는 명시 관측 장치로 공급한다. 현재 선택을 다른 기존
노드로 바꾸어도 처음 반환된 stream에 Technology/Mission이 쓰이는지,
Mission 두 번 재조회·장부 불변·공유 난수와 선택 누락의 부분 효과를
확인했다. 전체 미션이 정상 설치되었다는 증거로 세지 않는다.

첫 실행에서 이 관측 장치가 미작성 Technology 뒤 새 노드를 만들어
이전 tail의 물리 할당을 0으로 축소했다. 원문 MemStream 진단이 발생했다.
이미 있는 노드를 선택하는 입력으로 고쳤으나 빈 Deck은 설치 시 노드가
생기지 않아 선택 진단이 났다. 명시 Deck 1BYTE 입력을 준비한 뒤 통과했다.
제품 코드/기대값을 느슨하게 바꾸지 않았다. 메타데이터 갱신 보조 명령도
manifest 배열 키를 `fixtures`로 잘못 지정해 실패했고 실제 `entries`로
고쳤다. 보호 fixture는 변경하지 않았다.

나머지 1검사는 실제 `campaignOwners`의 전체 저장 API 네 가지가
Load 전/반납 뒤 거부되고 Template 헤더·Pool·난수에 부분 효과를 남기지
않는지 확인한다. 실제 owner의 `loaded`는 계속 false다. 외곽 Session과
전체 `Load()`는 미구현이므로 cpppj `CampaignFortSnapshotTests`의 실제
미션 snapshot/재로드 통합은 **4D-2h 뒤 재대조**한다. 이 선행 때문에
4D-1b 체크는 아직 닫지 않고 다음 4D-1c callback 대상으로 진행한다.

원본 `fort-snapshot-parent` 2,728관측과 저장 자식 fixture는 기존 직접
소비 시험을 유지한다. manifest의 관련 모듈/범위만 E636으로 연결했으며
신규 결선 검사를 직접 fixture 소비자로 등록하거나 새 fixture를 만들지
않았다. 실제 게임 창·브라우저를 실행하지 않았으며 보호 원본·cpppj·
공통 인계·README·AGENTS.md는 수정하지 않았다.

최종 Node v22.23.3·v26.13.0 전체가 각각 **6,017건 통과**, 실패·취소·
건너뜀 0이다. v24는 사용자 지시대로 생략하고 호환을 가정한다.
대응표 1,160행 오류 0·manifest 823항목(신규 0)·`git diff --check`를
확인했다. 최종 로그는 Git 제외 루트의
`extracted-webpj-e636-node22.log`·`extracted-webpj-e636-node26.log`다.
