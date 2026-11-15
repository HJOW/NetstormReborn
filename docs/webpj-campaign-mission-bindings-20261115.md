# 미션 Start·Tutorial 공통 프레임 연결 이식 (WB04-E520)

4-D 구획 C의 `client/RawCampaignMissionStartBindings`와 `client/RawCampaignTutorialOneBindings`를
`webpj/www/core/raw-campaign-mission-start-bindings.js`·`raw-campaign-tutorial-one-bindings.js`로
옮겼다. 두 소스는 기준선 `225553df`와 현재 cpppj가 줄 단위로 달라 보이는 곳이 있다. MissionStart의
`(+5)`는 테스트 전용 자동 플레이 호출(아래)이고 TutorialOne은 변경이 없다.

## 옮긴 범위

- MissionStart: `start()`(prepareWorld → 원본 004c2b20), `createAi(owner)`(원본 004c2d40: 범위
  1..8·호출자가 채운 600바이트에 기본값 → 공통(-1) → owner 설정을 누적한 뒤 Session AI 생성),
  `findLivePriest`(원본 004921f0, dead 제외), `startBody`·`gate`. 설정 조회·SP·기술·AI·명령 Gump·
  랜덤 간헐천·카메라는 모두 필수 경계이며 하나라도 빠지면 최초 등록 전에 거부한다.
- TutorialOne: 공통 프레임(`frame`), 특수 명령(`event`: First → 다음 단계 → 다음 안내 → 현재 사건),
  `tellStage`·`tellAt`·`stageShown`·`gate`. 비데모 1~6에서만 전용 Early 판정기를 만들고(1은 한 인자,
  2·3~6은 UI 경계 묶음 포함), 판정기는 gate 생성 뒤 같은 주소에 연결한다. 현재 사건 세 DWORD는
  호출 동안만 빌리고 예외에도 복원한다.
- 실제 `RawCampaignSession`이 아직 없어 두 모듈은 최소 덕타입 `session` 계약을 문서화하고(각 모듈
  머리글), 4-D 구획 D에서 같은 이름으로 구현한다.

## 의도적으로 옮기지 않은 것

- cpppj `CampaignTestPlay`(테스트 전용 자동 플레이 `--test-*` 옵션, PORT_MAP `exclude-host`)와
  그 호출(`CreateLocalTestAi`·`TestPlay`의 시작 SP 변경·`o/AiAllianceSide`)이다. 일반 사용자 실행과
  편집 Test Battle에서는 원본 Start와 한 비트도 다르지 않다. `effects.testPlay`를 주면 명시 오류로
  거부하며 `testPlay()`는 항상 null이다.
- 표시 부호화(`DisplayTextInput`의 `EncodeDisplayText`·`HasExtendedDisplayText`, 6·13차 한국어/
  일본어)는 순서 10 몫이다. AI 이름 키(`...name`)는 주입된 `effects.encodeDisplayText`가 있으면
  그것을, 없으면 원문 부호화를 쓰며 확장 이름의 29바이트 제한도 `hasExtendedDisplayText` 주입과 함께
  켠다. 영어 대본의 ASCII 이름에서는 두 경로가 같다.

## 시험

`webpj/tests/raw-campaign-mission-bindings.test.mjs` 11건(실제 `RawMissionStart`·`RawMissionAiSettings`·
`RawPriestLookup`·`RawMissionFrameGate`·`mission-guide-input`, session/script는 덕타입):

- MissionStart: 필수 소비자 여섯 개·미로딩 Script·testPlay 거부, Start 순서
  (`prepareWorld → setSp:1:123 → refreshSp → clearTech → addTech → settings/createAi:2·3 →
  명령 Gump → 랜덤 간헐천 → 카메라`, started·시작 시각·tellAtList·시퀀스), prepareWorld 없는 데모,
  CreateAi의 caller 바이트(0x100·0x200) 보존과 누적 값(공통 5 → owner 9, StartMoney 777, 이름 `Bob\0`),
  범위 밖 owner 거부, FindLivePriest의 dead 제외.
- TutorialOne: 연결 계약 거부, 데모·번호 0/7의 판정기 없음, 번호 1·2·3~6의 생성 인자와 사건 전달,
  First/다음 단계(modifier 제거)/다음 안내 명령, TellStage의 단계 문자열(`A.`·`Demo`)·창 없음 결과 0,
  TellAt의 사제 dead·인덱스 증가·다음 예약, 판정기 예외 뒤 현재 사건 복원.

최초 실패와 교정: caller 바이트 보존 시험이 `bytes[0x2f0]`(범위 밖)과 `bytes[0x256]`(기본값이 쓰는
`+0x254` DWORD 안)을 읽어 실패했다 → 어떤 단계도 쓰지 않는 `0x100`·`0x200`으로 바꿨다(시험 입력 오류).

## 남은 경계

- 실제 `RawCampaignSession`·`MissionScript`(Session 쪽 현재 Config) 연결과 cpppj `session-dump` 대조는
  Session 조립(D 구획)의 몫이다. 부품 대조이며 앱 수명에 연결하지 않았다(PORT_MAP `port`).
- 다음 시작점: `RawCampaignLifetimeBindings`.
