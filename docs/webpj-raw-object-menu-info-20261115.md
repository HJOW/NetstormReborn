# 객체 메뉴 제목/정보 이식 (WB04-E546)

G(UI 프레임워크·메뉴)의 "객체 우클릭 메뉴" 중 `client/RawObjectMenuInfo`(원문 10.78 제목/정보 4인자 004ad780·요약 004ada70·현재 상태 경고·포획 문구 0042a2f0·`4a8980` 로마숫자·`4a8940` 원소 이름표)를
`webpj/www/core/raw-object-menu-info.js`로 옮겼다. 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. E543(`RawObjectMenuOpen`)의 `summary` 훅이 이 단위의 `summary`이다.

## 옮긴 범위

- `describe(type, count, owner, sid)`(004ad780): 네 인자를 각각 보존한다. 제목(타입 설명) → 제단이면 count−1 → 클래스가 있고 flags2 `0x80074000`이면 ` Level %s`(음수 count는 0, 로마숫자 `levelName`) 제목 → 너깃은 owner 0 → 이름(양수 owner면 owner 이름, 아니면 번역된 `Nobody`; 80바이트 한도) →
  현재 로컬 소유자(이름 조회 뒤 읽음)로 `Owner`/`Ally`/`Enemy` → 편집이고 sid가 −1이 아니면 `SubID`와 객체 좌표(`_ftol` signed64 하위 DWORD) 두 줄 → 관계/이름 줄 → 클래스가 있으면 `Alignment`(theme<5는 원소 이름표, 아니면 `None`)와
  `Class`(번역→설정 치환 1024바이트) 줄 → 끝 줄바꿈.
- `summary(object)`(004ada70): owner/TYPE를 count의 외부 효과 전에 보관 → 가상 count → `describe` → 현재 extra 비트 0x10/0x20이면 Invisible!/Paralyzed! 경고(그림 TYPE는 현재 전역) → `captureStatus`.
- `captureStatus(object)`(0042a2f0): 492090이 거짓이면 아무것도 읽지 않고 반환, 참이면 미션 비영·Player 상태 비트에 따라 `Not Connected!`/`Capturable!`.
- 정적 `levelName`(0..9 로마숫자와 범위 밖 sentinel)·`themeName`(8문자열, 범위 밖 거부), `bindObjectMenuInfoControls`(제목/머리 위임).
- 번역/치환/현재 게임 조회는 사용 위치마다 현재 값을 읽고 도달한 미연결은 성공으로 대체하지 않는다(`/미연결/`). 서식 256·이름 80·치환 1024·클래스 23바이트 한도를 지킨다.

## 시험

`webpj/tests/raw-object-menu-info.test.mjs` 4건:

1. 원본 관측 `object-menu-info-x86.tsv` **1,434입력**: 다섯 연산(Describe·Summary·CaptureStatus·LevelName·ThemeName) × 20 DWORD 독립 입력과 콜백이 슬롯/전역/클래스/TYPE를 바꾸는 변경 조합에서 반환 문자열·번역/서식/GUI 효과 전체 순서·콜백 뒤 현재 필드.
   **첫 실행에서 1,434입력 전부 일치**했다.
2. 계약: 거짓 492090은 상태를 읽지 않음·비영 미션은 Capturable!·미연결 제목 소비자와 풀/표 크기 거부·원소 이름표 범위.
3. 계약(변형 확인 뒤 보강): Level 플래그 마스크·signed 소유자(0x80000001은 Nobody)·이름 79/80·서식 255/256·치환 1023/1024·클래스 23/24바이트·편집 SID 범위.
4. 연결: 제목/머리 위임.

**변형 확인:** 제단 count·Level 조건/플래그/음수·너깃·이름 양수/한도·소유 관계·동맹·편집 SubID·좌표 오프셋/순서·정렬 조건·치환 한도·snapshot 순서·Invisible/Paralyzed 비트·경고 그림 TYPE·포획 단락/연결/비트·클래스 한도·서식 한도·원소 범위·`_ftol` 비유한 등
27가지를 넣어 시험이 모두 실패함을 확인했다. 단 하나(편집 SID 범위 65535/65536)는 두 경로가 모두 같은 `RangeError`라 동등 변형이다. 처음 살아남은 7개는 3번 시험으로 잡았다.

`webpj/tests/fixtures/manifest.json`에 `object-menu-info`(1,434입력, 원본 10.78) 항목을 추가했다.

## 남은 경계

- 실제 번역/설정 치환(`441740`)·Player 이름(`484480`)·`492090`·Gump 제목/머리 작성은 호출자가 공급한다. 객체 메뉴 계열 중 `CampaignObjectMenuClient`(Client 연결)가 남았다(PORT_MAP `port`).
