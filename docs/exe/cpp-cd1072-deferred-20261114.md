# CD 10.72 복원 보류 인계 (2026-11-14)

- 중단: 2026-11-14, 사용자 요청에 따라 새 CD 분석·복원을 중단했다.
- 마지막 디컴파일 호스트: HJOW-Athlon. 기존 `extracted/refined/originalCD/NETSTORM.c`와 `extracted/refined/originals/Netstorm.c`를 읽었으며 재디컴파일하지 않았다.
- 제품 소스, 공유 인계 문서, 기존 원본/fixture를 수정하지 않았다. 빌드·테스트·게임·멀티플레이어·통신을 실행하지 않았다.
- 작성된 파일은 이 메모와 `prepare.py`뿐이다. **prepare.py는 미실행·미완성**이며 존재하지 않는 `gump-tests.fragment`와 `factory-tests.fragment`를 읽으므로 실행하면 실패한다. 준비 소스 여섯 파일 및 `cd1072-html.patch`는 아직 생성하지 않았다. 이 스크립트는 적용 가능한 완료 패치가 아니다.

## 완료한 정적 조사

- CD HtmlWindowGump 생성 `004c2210`, 소멸 `004c2300`, 입력 `004c23c0`, 본문 `004c2500`, ClearButtons `004c2680`, AddButton `004c26d0`, Fit `004c27c0`, Idle `004c2da0`, Metadata `004c2e00`, Factory `004c3430`.
- CD 가상 표 `00506478`, 실제 크기 `0x11e8`. HTML은 `+0xc4`부터4096BYTE, 제어행20개는 `+0x10c4`, count/body/update/requestedWidth/requestedHeight/menuColumns/aux는 `+0x11b4..+0x11cc`, timeout double은 `+0x11d0`, timeoutCommand/Parameter/OnExitCommand/Parameter는 `+0x11d8..+0x11e4`다. 정규화8192배열을 그대로 CD물리크기로 간주하면 안 된다.
- 생성자는 html[0]만 지우고 timeoutParameter/onExitParameter를 덮지 않는다. instance 증가 뒤 signed kind `00535f14`를 기록한다. instance `0053dd9c`, 최근닫힘시각 `00583e10/14`.
- CD SetBody의 LF 보충 지역배열은1024BYTE라 LF 없는 입력 최대1022BYTE다. PATCH는1008BYTE/1006BYTE. 버튼/본문 할당 크기는 같은0x170/0xd8다.
- Factory는4096BYTE 두 지역배열과 **단일 Parse**를 사용한다. 첫 Parse 뒤 내용 검사→시각→Metadata→필요 시 OkButton Metadata→현재 첫 버퍼에 LF 추가→Body→Attach→Modal→현재serial 반환이다. PATCH의 LF→BR변환 및 두번째 Parse를 CD에 적용하면 잘못된다.
- Factory empty-body는 `xxx`, root `0056b748`. 중심 계산은 screenWidth `00516b10` 읽기→현재 node.baseWidth 차분→root읽기→x계산→screenHeight `00516b14` 읽기→현재 node.baseHeight 차분→y계산 순서다.
- CD Metadata는 Button/ButtonInstant/Menu/Checked/Timeout/OnExit/Key의7패턴이다. Input/Zsend를 PATCH로 대체 처리하면 안 된다. Menu의 특수키는 FORTLIST/TECHLIST/THEMETECHLIST와 `@`뿐이며 SOLOLIST, `#`, `%`는 CD에 없다. Checked는 userCheat `0053dda0`, cheat `00537e8c`를 사용한다.
- CD 명령조회 `0041ab40`, 실제 표 `00512c78`는 이름포인터와 명령DWORD의 **포인터** 쌍이다. Config6d/DoNothing89/MissionBegin8b/Abort8c/Restart8d/Fork8e/End8f/Tell95/MenuJustClosedbe/StartDemoc0/LeaveBattlec5/SetOwnercc/ViewSpotd9/MoveDialoged는 공통02000000접두어를 쓴다. Warning은CD표에 없다.
- RawMenuHtml.inc BindHtml/ParseHtml에는 PATCH판 전제 게이트,8192링크범위/alias/복사범위가 남아 있다. 게이트만 해제하면 CD판을 올바르게 지원하지 못한다. HtmlTextParser와 HtmlConditionEffect는 기존 판본 선택 기능이 있다.

## Fit 관련 루트 전달 근거

- viewport width는 `0053f698`(초기640). 화면 상한은 고정640×480이며 `+40`, `+65` 여백과 초과 시600 target clamp를 사용한다. PATCH의 현재 화면크기 일반화와 다르다.
- 상수: `005063dc=.5`, `005063ec=40`, `005063f0=50`, `005063f4=20`, `005063f8=2`, `005063fc=4`, `00506400=640`, `00506404=65`, `00506408=480`, `00506414=10`.
- `004c2a0f`에서600.f의DWORD `0x44160000`를 loaded targetDWORD와 **signed integer cmp/jle**로 비교한다. 이 부분을 일반 float min으로 바꾸면 비정상 float 입력 의미가 달라진다.
- auxiliary를 reference `00492490`로 찾은 경우 screenWidth→node.width→x, screenHeight→node.height→y 순서로 중심 계산한다.
- auxiliary가 없으면 mapBounds right `00583e28`−left `00583e20`, bottom `00583e2c`−top `00583e24`의32bit 차분을 먼저 보관하고, x 계산 때 현재 left, y 계산 때 현재 top을 다시 읽는다.
- 최종 Attach `00493070`는 진입 parent/mode를 보존한다. depth는 `-80-node.depth`를 `00494300`에 넘기므로 PATCH와 -80은 같다. Fit 자체는 root 전역을 읽지 않는다.

## 미완료 범위

HtmlWindowGump/Factory 제품 통합, CD 단위검사 작성·실행, 테스트 기대값 fixture 추가, Fit/Metadata/RawMenuHtml 연결, 전체 BindHtml 게이트 해제 모두 미완료다. 새 복원 작업은 진행하지 않았다. 후속 착수 시 `prepare.py`를 그대로 실행하지 말고 사용자의 재개 지시와 당시 제품 상태를 먼저 확인해야 한다.

## 제품 경계와 재개 조건

사용자 요청으로 CD10.72 C3~C5 복원을 보류했다. 확정1310: CD 메뉴·로컬/설정 CDDir·Player 초기화는1309까지 실제 확인했으며1310의 Start/AI 설정은 순수 몸체·집중 검사만 완료했다. 튜토리얼·실제 월드·CD 일반 전투·CD LAN은 미완료다. 사용자 재개 지시 전에는 이 작업을 다음 필수 작업으로 선택하지 않는다. 남은 비CD 필수 작업은 M8 배포 관문이며, 선택/결정·수동 확인 항목은 LEFT_JOBS.md의 별도 표를 따른다.
