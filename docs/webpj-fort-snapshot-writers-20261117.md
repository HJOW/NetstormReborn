# 캠페인 스냅샷 자식 writer (WB04-E633)

4D-1a의 RawFortMoneySave·RawFortMissionSave·RawFortBadgesSave·RawFortCoreWrite
네 계열(.cpp/.h 합계295줄)을 기준선 `225553df`에서 이식했다.
기준선 뒤 해당 여덟 파일의 변경은 없다. Session5924..5982의 전체
캠페인 snapshot은 기술 획득·신전 상승에서도 이 자식들을 사용하므로
`exclude-edit`를 `port`로 교정했다. 편집기 UI/파일 저장은 추가하지 않았다.

Money는 같은 GameRandom의 Next(1010)를 한 번 소비해 333.0f를 더한
float32 네 BYTE를 일반 writer로 쓴다. SP 저장소는 읽거나 초기화하지
않는다. 단독 부모는 clamp하지 않은 slot의 manager/Create 반환 stream을
사용한다. Mission은 최초 NUL 길이의 하위 WORD 뒤 같은 원문의 최초
전체 길이를 쓰고, 그 뒤 현재 Mission을 다시 선택한다. sequence 쓰기
중 Guide가 바뀌어도 flags는 이미 선택한 Mission의 현재 필드다.

Badges는 최초 count 할당/위치를 보관하고 type0..254를 전부 조회한다.
비0 count만 낮은 BYTE로 쓰며 용량 진단 뒤 현재 data/pos를 다시 읽는다.
장부는 28x220BYTE의 첫 type 일치 DWORD이며 padding과 중복을 보존한다.
CoreData는 manager/metamap 진단 뒤 post/editor 자연 반환을 적용한다.
Create 뒤 한 번 보관한 Player 할당과 각 소비 위치의 현재 전역/필드를
구별한다. seed는 같은 `{ value }` DWORD 참조이며 상위 WORD를 보존하고
공유 난수 low16이 비영일 때까지 소비한다. 하위 누락/외부 중단은 앞선
생성·쓰기·난수 소비를 되돌리거나 성공으로 바꾸지 않는다.

cpppj FortMoneySaveTests·FortMissionSaveTests·FortBadgesSaveTests·
FortCoreWriteTests의 12검사를 같은 이름으로 옮겼다. 기존 x86 관측
Money1105·Mission230·Badges1352·CoreWrite338+22+36행, 합계3083행의
전체 raw/전역/두 할당/시간순 trace를 대조했다. Mission 중단12·Badges80·
CoreWrite16+2+6과 Badges getter167834회/진단3788회/Create710회도
확인했다. 관측 fixture를 수정하거나 새 게임 창을 실행하지 않았다.
최초 집중 실행12건은 모두 통과했으며 구현/기대값 교정은 없었다.
첫 전체 검사에서는 manifest 정적 감사2건이 실패했다. 공용 테스트
도우미가 파일명을 조합해서 읽어 basename 감사를 통과하지 못한 것이다.
입력 파일명을 각 호출에 명시해 고쳤으며 fixture/제품 기대값은 바꾸지 않았다.

첫 전체는 Node v22·v26 각각6,008건 중6,006통과/추적 감사2실패였다.
교정 뒤 저장 자식/추적 목록 집중15건이 통과했고 최종 전체는
Node v22.23.3·v26.13.0 각각6,008건 통과, 실패·취소·건너뜀0이다.
v24는 사용자 지시대로 생략하고 호환을 가정했다. 대응표1,160행의
오류0·분류 port967/adapt51/제외142와 manifest822항목을 확인했다.
`git diff --check`도 통과했다. 전체 로그는 Git 제외인 루트의
`extracted-webpj-e633-node22-final.log`·`extracted-webpj-e633-node26-final.log`다.

전체 Session snapshot 본체와 실제 Template/Client 장부 결선은 다음
4D-1b다. 여기서 전체 snapshot 또는 실제 미션 Load를 완료했다고
기록하지 않는다. 앱 session0·실제 미션 플레이 불가 상태를 유지한다.
