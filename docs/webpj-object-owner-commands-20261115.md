# 객체 소유자 명령 이식 (WB04-E531)

6단계(입력·게임 메뉴·경제·건설)의 `client/ObjectOwnerCommands`를 `webpj/www/core/object-owner-commands.js`로 옮겼다.
Session 없이 하위 장치(가상 74·Player Core 선택 등)를 훅으로 받는 순수 순서 규칙이라 선행 부품으로 먼저 처리했다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 `git diff --numstat 225553df HEAD`로 확인했다.

## 옮긴 범위

- `dispatchObjectOwner`(원문 4d817e..4d81cb): `0x020000cc`(상위 플래그 제거 비교)가 아니면 false, release는 소비만 한다.
  편집 또는 미션 후처리 모드에서만 현재 selected를 한 번 읽어 **보관**하고(가상 74 뒤 현재 selected로 바꾸지 않음)
  비영이면 owner(보관 SID, `event.y` DWORD)를 부른 뒤 **뒤늦게 읽은** editor가 참이면 follow(보관 SID)를 부른다.
  모드 안이면 선택 유무와 무관하게 월드 재그리기 revision(0059a8b0, DWORD 감김)을 1 올린다.
- `followEditorObjectOwner`(4d5290): 현재 unsigned owner BYTE(+34)를 읽어 0이 아니면 `SelectPlayerCoreData(owner, 1)`와
  revision +1(owner 0은 무동작). 0번/예약/free/dead도 원문 물리 안이면 읽는다. 10.78 풀과 SID 범위(<capacity, ≤65535)를 지킨다.
- `finishEditorPrimarySelection`(4da2e8..357): Editor가 꺼졌거나 선택이 0이면 아무것도 읽지 않고, control이면 owner가
  로컬과 다를 때 assign 후 follow, 같으면 flags2의 다리 비트(4)가 있으면 cycleBridge 후 follow. owner 변경 뒤 Editor를 재조회하지 않는다.
- 선택하지 않은 하위는 요구하지 않고 선택된 하위 누락은 앞선 효과를 보존한 채 명시 거부한다.
- revision은 `{ value }` 상자이다(C++ `uint32_t&` 대응).

## 시험

`webpj/tests/object-owner-commands.test.mjs` 4건:

1. 원본 관측 `object-owner-command-x86.tsv` **256행 전부**: 효과 순서(`O`·`C`)와 최종 `(revision,editor,selected,owner BYTE)` 상태, 보관 SID의
   owner BYTE 밖에서 풀이 불변임을 확인했다.
2. 원본 관측 `editor-primary-selection-x86.tsv` **256행 전부**(`F`·`O`·`B`·`C` 순서와 최종 상태). 두 파일 모두 **첫 실행에서 일치**했다.
3. 편집 후처리의 미선택/누락 계약(cpppj `UnselectedHooksAndReachedFailure`).
4. release·다른 명령·선택된 하위 누락·revision 감김·Follow의 무동작/거부/범위.

`webpj/tests/fixtures/manifest.json`에 `object-owner-command`·`editor-primary-selection` 항목(원본 10.78, 각 256행)을 추가했다.

## 남은 경계

- Edit 기능은 웹 제외이므로 편집 전용 함수(`followEditorObjectOwner`·`finishEditorPrimarySelection`)는 웹에서 호출하는 소비자가 없다.
  분류(`port`)와 cpppj 계약 보존을 위해 옮겼다. 실제 일반 입력(`UserInput`)에서 `dispatchObjectOwner`를 부르는 연결은 6단계 입력 본체의 몫이다.
