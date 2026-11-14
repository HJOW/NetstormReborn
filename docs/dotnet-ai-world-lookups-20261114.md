# dotnetpj 0.505 — 실제 AI 사제 조회·Player 색 변경

2026-11-14, HJOW-Athlon. AI 생성에 필요한 원본 조회·색 변경 부품을 이식했다. 호스트의 AI 생성 호출은 아직 연결하지 않았다.

## 구현

기준은 `fffd98dc46f4f75eade8f13c01c357ba1cf206ca`의 `o/RawPriestLookup.h/.cpp`, `o/RawPlayerColor.h`와 해당 원본 관측 검사다.

`RawPriestLookup`은 기존 `PriestPostPopState.GetPriests()`를 사용해 사전 파괴와 동일한 실제 목록을 조회한다. signed 개수가 0 이하이면 목록을 읽지 않고 0을 반환한다. 활성 순서에서 현재 owner BYTE를 전체 player DWORD와 비교하며 dead 비트2만 제외한다. free·void·타입·원래 소유자·extra는 읽지 않는다. 첫 일치 뒤의 손상 SID도 읽지 않는다. 풀 스캔이나 별도 목록 복사는 없다.

`RawPlayerColor.ChangePlayerColor`는 색 표를 먼저 쓰고 SID2부터 원문 iterator 순서로 현재 섬 받침·종유석을 갱신한다. 원본처럼 free만 건너뛰고 다른 소유자의 섬도 현재 표로 프레임을 계산한다. 진입 당시 iterator 상한과 콜백 이후 현재 풀 상한을 구분한다. 프레임 소비자 누락 시 이미 쓴 색 표를 되돌리지 않는다. 호스트 프레임 장치 결선은 후속 단위다.

## 검증

- `priestlookup-x86.tsv`: 주석 제외4851입력(10.78·CD10.72·별도10.37 각1617). 반환DWORD·전체 입력 풀/목록 Adler·조회 순수성을 전수 대조한다. signed 개수·늦은 손상 SID·WORD 절단 전 범위·DWORD owner·공유 목록 교체도 검사한다.
- `player-color-x86.tsv`: 헤더 포함217행·216입력. 색 표36바이트·16슬롯800바이트·호출 순서를 전수 대조한다. 소비자 실패 순서와 콜백 뒤 변경된 현재 표/owner도 검사한다.
- 검사 직렬화는 C# 저장소의 공통50바이트 슬롯을 CD 관측의 실제36바이트 슬롯으로 경계만 맞춘다. 첫 검사에서 이를46바이트로 잘못 적어 CD checksum이 모두 달랐으나, cpp `SidPool` 생성자의 `SidLayout{36,6000,14000}`를 확인해 수정했다. 제품 풀 레이아웃 변경이나 CD 복원 착수는 아니다.
- 사제 조회에서 void도 제외하는 변이는 각판본91/1617, 색 변경에서 dead도 제외하는 변이는216/216 불일치를 검출했다. 두 변이를 원복했다. 전체 비증분 Release 빌드 경고0·오류0과 관련집중27건 재통과를 확인했다.

원본 fixture SHA256은 사제 `c18f6985bd89a33cb65951fc70253960658170b8f7f40e97f9053446f531c907`, 색 변경 `fbfe051220bccb0c2732cdb8954208b9787eb91e5216994938ca395db30d6603`이다. Link로 공유하며 원본을 수정하지 않았다. manifest는 신규2행만 추가했고 기존241개 해시는 보존했다. PORT_MAP은3행 추가해704행(L1 346·port358), fixture243개다. 산출물은 `extracted/dotnet-ai-lookups-0505/`에 있다.

새 게임 실행·신규 NB 수정·외부 접속·기존 경로 은퇴는 없다. 보호 파일·cpppj·LEFT_JOBS.md·병행 webpj 변경은 보존했다. M1은 계속0/34다.

## 다음

실제 사제 PostPop 접두를 연결해 목록 등록·중복 억제·회복 Regular·소유자 상태를 공유한다. 현재 `RawPriestPreDestroy`가 소유한 목록 상태에 필요한 필드를 더하고 같은 `GetPriests()`를 사용한다. 이후 AI 메모리/생성/초기갱신·기술 내용물·지역·색 프레임을 조립한다. 기존 호스트의 요새 사제 Pop은 아직 비어 있으므로, 빈 조회 목록을 AI 완료로 세지 않는다.
