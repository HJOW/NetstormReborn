# 아비어리 공유 수신 선택 필드 decoder 이식 (WB04-E530)

`o/RawAviaryProcessData`(원문 0045b0f0: Sun/Rain 아비어리가 공유하는 선택 필드 decoder)를
`webpj/www/core/raw-aviary-process-data.js`의 `readAviaryProcessData`로 옮겼다. 재기준선에서 추가된
`[rebaseline-port]` 단위이며 기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다.

## 옮긴 범위

- 패킷 9바이트 접두 부족은 거부. `packet[8]`의 dirty 비트로 선택 필드를 접두 뒤에 이어 붙은 순서대로 읽는다:
  비트 0 → 부모 DWORD(반환으로만 조립하고 outer 이미지에는 쓰지 않음), 비트 1 → `fields[16..23]` 8바이트 복사,
  비트 2 → `fields[24..27]` 4바이트 복사. 계산 없는 복사와 원문 선택 순서를 유지한다.
- 선택 필드가 패킷에 부족하거나 outer 이미지에 실제 저장이 없으면 명시 오류이며, 앞선 부분 쓰기는 되돌리지 않는다.
  선택 필드가 없는(dirty 0) 호출은 빈 outer 이미지에서도 오류가 아니다(Native NULL this).

## 시험

`webpj/tests/aviary-process-data.test.mjs` 2건: 비트별 순서/폭/보존 바이트/부호 없는 부모, 접두·선택 필드 부족과 저장 없는 필드의
거부 및 부분 쓰기 보존. **이 단위에는 전용 원본 관측 fixture가 없다**(cpppj는 `RawCampaignSession` 수신 경로로만 소비). 따라서
계약 시험이며 원본 동일성의 추가 증거는 없다.

## 남은 경계

- 실제 수신 경로(Sun/Rain 아비어리 프로세스의 `RawAviary*` 수신과 Session의 패킷 분배)는 4-D D 구획과 7단계의 몫이다.
  앱 수명에는 연결하지 않았다(PORT_MAP `port`).
