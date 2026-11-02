# D/d 최근 생산 항목 취득 (단위756)

2026-11-03, HJOW-Athlon. 마지막 전체 정밀 디컴파일은 HJOW-Athlon / 2026-10-23이며 산출물 변경은 없다.755를 `5cb5ba37`로 로컬 커밋한 뒤 진행했다.

## 복원

원문10.78의 두 jump table `004da5b4`/`004da570`은 문자 D/d를 `004d96fa`에 보낸다. 디컴파일 C가 잃은 인자는 실제 명령에서 확인했으며, 현재 `00542b10`의 getlastitem(초기 `02000107`)을 두 인자0/0으로 일반 FIFO에 예약한다. 일반 정지 관문 뒤이며 수정키를 별도로 거부하지 않는다. 뗌·가상키·일반명령 종류를 같은 문자로 축소하지 않는다.

기존 InventoryItemGump의 반복 명령 몸체는 이미 있었다. 이번 수정은 기존 생산 단축키의 같은 현재 명령/큐 경계에 별도 repeat getter를 추가하고 UserInput에 연결한다. QWASZX/Y와 E의 여섯 슬롯은 기존 명령 getter를 계속 사용하며, 최근 항목은 별도 getter를 호출해0/0만 예약한다. 기존 aggregate의 command/queue 순서는 보존했다. 선택한 명령 조회가 성공한 뒤 큐 장치가 없으면 조회의 부분 효과를 유지하고 진단한다.

반복 명령은 기존 InventoryItemGump에서 현재 lastType과 같은 실제 항목을 찾아 현재 contents/serial·지식/disabled 조건을 읽는다. 키에 새 생산 상태나 마지막 항목 사본을 만들지 않았다. 대본 getlastitem 별칭이나 기본 메뉴 행을 임의 추가하지 않았다. CD10.72의 입력 부모0041ae80은 별도 판본 범위이며 이번 새 관측/접합은10.78에 한정한다.

## 검증

원문452입력의 두 정밀도904관측은 차이0·스택 차이0이다. 현재 명령·수정키/뗌·선행 소비·정지·고리 위치/포화·연속 사건을 포함하고, 별도904회 재실행에서도 fixture 전체 바이트가 일치했다. Guide 가상+24·Gump 전달·일반 정리와 OS는 명시 경계다.

Release 제품/기존 선택키 집중 대상은 경고/오류0이며 새4개를 포함한65개가 모두 통과했다. 공개 InputQueue의 실제 FIFO/포화·현재 repeat getter/부분 실패와 기존 InventoryItemGump의 실제 Handle 소비를 확인했다. 기존 생산 슬롯2개와 Inventory 항목2개도 같은 대상에 추가해 기존 키와 반복 명령을 함께 검사했다. 이는 게임을 실행한 생산/플레이 재현이 아니다.

로그·SHA·보호 결과는 `cpppj/repeat-item-key-20261103.json`과 `extracted/cpp-resume-756/`을 따른다. 새 게임/GUI·독립 CLI·물리/PCM·공중3종 행동·포획/희생·특정 적 공격·승리·자연 완주·MP·전체 C++/34/ZIP 검사는0이다.

실행 제품756의 EXE SHA-256은 `b2a6b98c89153b2831762a7a7b5ccf3ea3001777b794140b2d9c31d0d41a8d59`이다. 기존 보호2665개는 모두 불변이고새 fixture1개를 더한 현재 기준은2666개/fixture846이다. 다음 기준은 `extracted/cpp-resume-756/protected-after.json`이다.

## 입력 어댑터의 남은 차이

원문 helper는 outside를 마스크로 제거하지만 실제 UserInput::Event 맨앞에는 기존 outside 반환이 남아 있다. RawCampaignCursor의 공통 outside 거부와 C/c·Tab의 별도 거부도 후속 감사에서 찾았다. 일반 WM_CHAR/가상키 큐는 outside를 붙이지 않으므로 OS 비활성 정책을 일괄 제거할 근거로 삼지 않는다. 숫자 카메라의 슬롯 계산은 Shift 외 비트를 보존하는 다른 계약이므로 모든 키를 한꺼번에 허용하지 않는다.

이 차이는761에서 Guide→Gump→일반 키/명령→커서/세계 마우스의 각 원문 단계로 복원한다. 이번452행 helper 결과를 실제 UserInput 어댑터의 outside 전체 통과로 확대하지 않는다. D/d만 예외로 우회하는 처리는 넣지 않았다.

다음은 전체 BaseFile 열기757, 추가 지식 메뉴758, kind17 생성자759와 일반명령/입력 어댑터 후속이다. 전체 목표는 미완료다.
