# 보상·SP 연결 문장의 현재 판정 — WB04-E1466

2026-11-28 HJOW-Athlon. 11월 7일 부품 이식 당시의 “재시드·실제 지갑·알림/소리·보상/후처리 장부 연결 필요”를 현재 앱과 Session에 대조했다. **단독 미션에서 필요한 연결은 이미 구현·검증돼 있다.** 제품·기존 검사·CPP·원본·번들은 변경하지 않았으며, 새 포획/희생·적 공격·승리 검사도 하지 않았다.

## 현재 소비자

| 예전 잔여 | 현재 연결과 근거 |
| --- | --- |
| 재시드·사제 추적 offset | 앱 초기화가 같은 `ClientRandomGlobals`·현재 `ClientTimeSource.unixSeconds()`로 `ClientSpGlobals`를 만들고 저장소를 명시 초기화한다. Session은 `_campaignSharedServices`의 같은 `spGlobals.store()`를 빌린다. 별도 미리보기 저장소를 쓰지 않는다. `GameRandomSeed`와 추적 상자는 Client 수명이다. E336·`client-sp-globals.test.mjs`의 원본 초기화48행과 명시 시작/현재 시각·독립 수명 검증을 유지한다. |
| 실제 AI 지갑 | `RawCampaignSessionAi.bindBrains`가 같은 AI memory·Player 기록으로 `RawAiWallet`을 만들고 `hp.bindPatchWallet`을 실제 `credit`에 연결한다. AI 건설 선택도 같은 `balance`와 분산 SP를 읽는다. 기존 Session AI 검사는 실제 본체+0x2d8의507과 모드/데모 조건을 대조한다. |
| 보상·후처리 장부 | Session의 `SquidReward`는 같은 `world.core()`의 Pool·types·bookkeeping과 현재 SP를 받는다. `SquidRewardState.bindGameAux` 및 현재 World battle/사제 TYPE 공유가 설치돼 있다. 내용물 수명도 같은 `hp`를 넘기므로 실제 지갑/SP를 사용한다. 보상2400행·SP가산 비트504행·HP579행과 Content 수명198행 등의 기존 원본 관측은 E1458 전체에 포함된다. |
| 수입 알림·위치 소리 | 앱의 `contentLifetime.localNotice`는 실제 입력 큐에 넣고 위치 소리는 같은 Client 재생기의 `guy.wav`로 간다. Session은 현재 커서 DWORD/로컬 owner/너겟 TYPE과 실제 ContentForm·Player 통지를 연결한다. 서비스 존재가 청취 결과를 입증하는 것은 아니다. |
| 의식 보상·Tell | 다이스의 SP 지급/타입 획득과 `SacrificeForStormpower`·`AltarUpgrade`는 같은 Core/SP·입력 큐·현재 Tell 서비스에 연결돼 있다. 기존 Session 검사는 실제 저장소의123→5123 및 기술 비트·두 안내 키를 대조한다. 보상 선택 UI와 원격 의식 선택 명령은 E1180의 단독 미션 제외 결정을 유지한다. |

## 검증 승계와 한계

고른 소스·검사 **19개는 E1458 Node26 전체6,932/6,932 시점과 원시 해시가 같다.** 앱 파일은 E1461 DPR 복구로 전체 해시가 달라졌지만, 관련 두 메서드 `_campaignSharedServices`·`_campaignGameplayServices` 본문은 E1458과 LF 정규화 바이트가 같다. [개별 출처·해시](webpj-e1466-evidence/provenance.json)를 남겼다. 이 감사에 새 집중 검사 통과 수를 더하지 않는다.

연결된 원격 `5a` 수입 패킷 송신은 현재도 명시 미지원 오류다. 단독 미션의 실제 NULL/접속 관문에서 생략되며, 웹 제외인 LAN 기능을 새로 구현하지 않는다. E1457의 Native/typed 판정과도 충돌하지 않는다.

1-1·1-2·1-5 및 스커미시의 기존 실제 아군 AI 플레이/승리와 의식의 모든 경우가 같다는 주장은 구분한다. 사용자가 맡은1-3·1-4·1-6·그 뒤 미션, 실제 청취/위치 음향은 그대로 남는다. 오래된 “아직 연결 없음”만 현행 인계에서 제거하고 원본 증상·전체 자연 플레이의 미입증을 완료로 바꾸지 않는다.

E1463/1464 입력 고정 중 문서만 마감했다. 두 전체 종료 뒤 E1465와 합쳐 문서2단위를 정기 주기에 이월한다. 원래 부품 문서는 당시의 역사 기록으로 유지한다.
