# 시간 알림과 임시 Chat 서식 이식 (WB04-E526)

`client/RawMonitorTime`의 남은 본체를 `webpj/www/core/raw-monitor-time.js`에 옮겼다(상태 구조체
`MonitorTimeState`는 E335에서 끝났다). 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.
이 단위로 `RawMonitorTime`이 partial → port가 된다.

## 옮긴 범위

- `stepMonitorTime`: 최초 guard(initialized bit0, 기한 0) → monitor·alert가 모두 0이면 끝(비영 기한을 지우지
  않음) → 기한 0이면 벽시각 + 180초 예약 → 새 벽시각이 기한 이상이면 달력·이름·한 줄·서식을 만들어 제출하고
  전달 뒤 벽시각 + 180으로 재예약. 매 시계 호출은 서로 다른 현재 벽시각이고 도달한 장치만 요구한다.
- `formatMonitorTimeLine`: 원문 `%s: !%s\n`(NUL 앞만, 두 끝 개행 보존, 256BYTE 미만).
- `monitorCalendarText`: asctime 25글자(끝 개행, 일은 공백 채움 두 칸, 32비트 시간 범위). 브라우저 현지 시간을
  쓰는 웹 적응이다. 달력 문자열은 원본 관측에서 주입되므로 결과에 영향이 없다.
- `submitTemporaryChatFormat`: 최초 front를 저장하고 판본별 위치(10.78 `+0x14`·60BYTE, CD `+0x10`·16BYTE)에
  임시 서식을 쓴 뒤 정상 submit 복귀에서만 현재 manager에 복원한다. submit 예외에서는 앞선 서식 변경을 남기고
  front가 지역 80BYTE를 넘거나 물리가 부족하면 거부한다. 판본 인자는 CD 판 12행 재생을 위해 함께 옮겼다.

## 시험

`webpj/tests/monitor-time.test.mjs` 4건:

1. 원본 관측 `monitor-time-hardware-x86.tsv` **1176행 전부**(originals와 originalCD를 같은 단계 로직으로 재생,
   CD 전용 atexit 열은 제외): 최종 initialized·deadline 비트(BigInt)·monitor·alert·벽시각 호출 수·요청 순서
   (`W`·`T;L;A`·`P`·`Q`·`E`). **첫 실행에서 전부 일치**했다.
2. 원본 관측 `temporary-chat-format-hardware-x86.tsv` **12행 전부**: 판본별 60/16BYTE 절삭·패딩과 현재 manager
   재조회, 두 manager 전체 256BYTE.
3. submit 실패의 부분 쓰기 보존(원본 관측 행과 같은 manager 상태), 지역 80BYTE/물리 부족 거부, 장치 미연결 거부.
4. 줄·달력 계약과 도달한 장치만 요구.

`webpj/tests/fixtures/manifest.json`에 `monitor-time`·`temporary-chat-format` 항목(원본 10.78, 각 1176·12행)을
추가했다.

## 남은 경계

- Client의 Config writer(`monitor`·`alert` 옵션이 두 DWORD를 갱신)와 실제 Chat manager 부모
  (`SubmitTemporaryChatFormat`을 부르는 채팅 관리자)는 4-D Client/채팅 조립의 몫이다. cpppj 시험
  `MonitorTime_ActualClientOptionsAndDeadlineLifetime`은 그때 대조한다.
- 다음 시작점: 4-D C 구획의 `RawCampaignAssetBindings`·`RawCampaignConstructionBindings`.
