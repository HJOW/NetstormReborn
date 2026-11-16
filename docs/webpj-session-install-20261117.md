# 최초 Template·WorldRuntime와 Fort 준비 부모 (WB04-E622)

기준선 `225553df`의 생성자1866..1871와 `MakeRuntime`3474,
공유 섬 population/초기 사제/alternate Fort3389..3450·사제 준비3451..3472를
같은 owner에 연결했다. RawCampaignSession.cpp/.h·RawFortPriestPreparation·
RawIslandPopulation은 기준선 뒤 변경0이다. 외곽 Session/AI/Load는 미완료다.

## 구현

- 실제 캠페인 분기만 선택한다. localLogin+파일 선택이면 Template를 아직
  열거나 공개하지 않으며 빈 선행 이미지도 함께 있으면 원문처럼 거부한다.
  이미 읽은 파일은 같은 installFile reader와 실제 파일명으로 설치한 뒤에만
  services의 선행 이미지를 반납한다. 일반 논리 Fort도 같은 Template1에 설치한다.
- 같은 World/Player/지형·HP/내용물·Template9로 `RawCampaignWorldRuntime`을
  만들었다. E621의 raw reader를 campaignObjects에 연결하고 실제 State/가상80·
  지면 통지와 현재 puzzlePiece/noRegion 참조를 빌린다. UI 표는 원문처럼
  값 복사 후 그 사본에만 FillMissing을 채운다. 구성 자체는 Load하지 않는다.
- Fort 사제 준비는 같은 현재 전역/Player 기술 부모·Form6/Template/사제 목록을
  사용한다. contains는 후보 SID를 성공 종류로 바꾸지 않고 raw+18 WORD를
  반환한다. 기본3기술·subscriber%3 테마 기술·소유권/지역/초기 배치를 연결했다.
- 공유 population은 매 호출 실제 단일 배치와 지역 finish를 만들고 현재
  권한/Fort/역할 TYPE를 계속 읽는다. 같은 HP/기본 프레임/가상 owner·Pop·
  생산 통지를 사용한다. 초기 사제는 청크 좌표의 DWORD 감김16배/float32,
  alternate Fort는 임시 객체 배치→섬0 채우기→가상 삭제 순서를 유지한다.
- 사제 수의 narrow snprintf 경계(d/s/%%·폭/정밀도·256BYTE)와 UTF-8 본문
  바이트를 보존했다. 지원하지 않는 서식이나 실제 번역/설정 장치 누락은
  도달한 위치에서 실패하며 임의 문구/성공값을 만들지 않는다.
- typed tutorial 역할15주소와71/85 필드는 owner마다 독립이다. 역할 조회는
  현재 Map을 읽고 누락 주소를 기본값으로 바꾸지 않는다. Native 장기 역할을
  설치한 것으로 세지 않는다.

## 검증

신규9건을 포함한 집중36건 통과다. 최초 집중20건부터 실패0이며, 후속36건도
첫 실행에 통과했다. 실제 File/Template 선택·선행 이미지 반납/실패 보존,
동일 Pool/난수·Load 미수행·외곽 미공개, UI 함수 값 복사/최초 실패,
실제 기술 Form6 네 종류·중복 방지/현재 역할 변경, population 구성과
누락 섬/진단 관문을 확인했다.

보호10.78 PE(`a305414cfe0b94494bb32fef2b359e920e9daba225e0821b76aea1f9f8e7d236`)
섹션 표로15주소와71/85 초기값을 직접 읽어 대조했다. 실행하지 않았다.
Windows `msvcrt._snprintf`의 독립 계산6장면(0/1/2명, 부호/0채움/좌측폭,
0정밀도/긴 문자열 정밀도, %%/정수 정밀도)을 바이트 기대값으로 사용했다.
처음 선택한 ucrtbase._snprintf는 미수출이므로 msvcrt의 같은 순수 서식 함수를
사용했다. 이를 원본 게임 실행이나 cpppj 오라클 검증으로 기록하지 않는다.
기존 원본 사제/섬 population·파일 reader 검사를 재대조했으며 기존 fixture와
보호 원본은 바꾸지 않았다.

Node v22.23.3·v26.13.0 전체는 각각5,948건 통과, 실패·취소·건너뜀0이다.
`port-map.mjs`·`node --check`·`git diff --check` 오류0이며 대응표의
RawCampaignSession partial·session0은 유지한다. manifest806항목/신규fixture0이다.
전체 로그는 `extracted/webpj-e622-node22.log`·`webpj-e622-node26.log`에 보존했다.
검사 기술 부모/Player 기록은 원문 helper의 명시 입력이며 Player Pop이나
전체 미션 시작 성공을 가장하지 않는다. 자연 플레이·포획/희생/승리·
공중3종 새 전용 생성/생산/행동·Native 장기 설치·게임 창 실행은 하지 않았다.
실제 사제 지역 처리는 뒤에서 설치할 같은 motion/전체 MayPlace 부모를 사용하며
그 전 선택 시 기존 명시 미연결 오류를 유지한다.

다음은 생성자1872의 지역 소유자/승패·ConnectAi와 남은 생성 구간이다.
이후 외곽 Session 공개·실제 Load·로그인/L2 대조로 이어 간다.
