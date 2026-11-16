# 캠페인 공용 Fort/Battle snapshot 부모 (WB04-E627)

`RawCampaignSession.cpp` 기준선 `225553df`의 외곽 생성자4321·4328과
LearnNewTechnology6090 부근은 신전 상승·의식 완료·기술 획득에서
현재 Battle/Fort 모드를 각각 읽어 SnapshotBattle/SnapshotFort를 호출한다.
그 두 메서드는5940 부근의 SnapshotTemplateBody를 통해
`RawFortSnapshotParent`의 원문 부모를 공유한다. 기존 편집기 전용 제외를
이 공통 부모 한 단위에 한해 교정했다. Save as·편집기 UI·디스크 저장은
이번 범위가 아니다. 대응표 대상은1,008단위/제외152가 됐다.

## 이식

기준선의 cpp/h와 FortSnapshotParentTests를 옮겼으며 기준선 뒤 변경0이다.
Fort/Battle 전체 부모와 두 현재칸 wrapper의 네 진입을 제공한다.
현재 manager·모드·owner는 원문 위치마다 다시 읽고, 고정 this와 기술 부모,
각 Create가 반환한 stream은 다른 current로 바꾸지 않는다. Battle/Fort의
독립 if, DWORD LEA 감김 주소 별칭, manager0 자연 반환과 선택된 진단/누락
경계의 부분 효과를 보존한다. child는 실제 caller가 연결해야 하며 빈 성공
기본값을 넣지 않았다. 아직 실제 Session callback이나 Template에 연결하지 않았다.

## 검증

신규3검사의 원본 관측 재생은 최초1,364입력×두 정밀도=2,728행 전체다.
정상/부분 중단378·전역8DWORD·Template12×20BYTE·getter/child 순서를
모두 대조했다. 고정된 원본 fixture는 수정하거나 새로 수집하지 않았다.
기존 cpppj 계약의 미선택 child/선택된 누락·자연 반환과, 추가 DWORD 주소
별칭/비정렬 경계도 검사한다.

최초3검사는2통과/1실패였다. 추가 경계 입력0xffffffff가 실제로는
정상 슬롯-1 주소를 만들므로 비정렬 delta를 만드는0x10000000으로 고쳤다.
제품 코드는 바꾸지 않았고 이후 신규3건이 통과했다. 전용 부모의 실제
child adapter를 재생한 것이며 전체 저장·미션 플레이 성공으로 세지 않는다.
대응표/manifest를 포함한 집중13건도 통과했다. manifest는 원본 관측 한 항목을
추가해807항목이며 기존 E625 작업 사본 표시7항목은 도구의 규칙대로 확정
커밋 `fb6b65cde`로 정리했다(자료/근거 해시는 불변).
전체 Node v22.23.3·v26.13.0은 각각5,975건 통과, 실패·취소·건너뜀0이다.
로그는 `extracted/webpj-e627-node22.log`·`webpj-e627-node26.log`에 보존했다.
대응표1,160행/오류0, `git diff --check` 오류0이다.
v24는 사용자 지시대로 생략하고 호환을 가정한다.

## 다음

외곽 생성자4318..4330의 실제 callback/공개 수명과 Load를 계속한다.
그 callback의 실제 Template/자식 직렬화 부모는 도달 지점에서 먼저 이식한다.
다음 선행은 `RawFortSnapshotBasics`의 TypeNames/Deck writer와 실제 stream이다.
현재 공통 부모를 제외한 다른 writer의 편집기 전용 분류는 자동으로 바꾸지
않았으며 캠페인 공유 호출을 각 단위에서 다시 확인한다.
앱 session0·실제 미션 플레이 불가와 기존 자연 완주/포획·희생·공중3종
새 전용 생성/생산/행동 검사 제외를 유지한다. 게임 창은 실행하지 않았다.
