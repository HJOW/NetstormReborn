# 힐 주문 대상 본문과 버스트 정기 이식 (WB04-E553)

`o/RawBombHeal`(원문 10.78 41ebb0 자격·41ebd0 현재 대상 상태 복원)과 같은 파일의 힐 버스트 정기(41f9d0)·그라비테이션 버스트 정기(41f760)를 `webpj/www/core/raw-bomb-heal.js`로 옮겼다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 같은 raw 풀·현재 188칸 TYPE/프레임 표·`SquidFrame`(4acee0)·`SquidProcessHost`를 빌리고 독립 회복 수치나 임시 월드를 만들지 않는다.

## 옮긴 범위

- `RawBombHeal.mayTarget(target)`(41ebb0): 현재 genus & 0x81574204를 0/1로 바꾸지 않고 그대로 돌려준다(HP·소유자·동맹·free/dead 관문 없음).
- `RawBombHeal.apply(parent, target)`(41ebd0): 운반체 대상이면 끝낸다. HP가 있으면 **최초 HP**를 저장한 뒤 최대 HP를 조회해 DWORD 합의 signed 0쪽 절삭 평균을 쓴다(32비트 감김). extra의 은신/마비 두 비트(0x30)만 해제하고,
  현재 TYPE이 펜스이면 재연결, 보행 genus(0x200000)는 Unpop(0)→Repop(0x40)(두 가상 표를 각각 다시 읽음), 아니면 표시 갱신을 한다. 버스트 금지 genus(0x1000004)가 아니면 현재 버스트 객체를 만들어 대상의 현재 발자국 중심(단정도 저장)에 놓는다.
- `handleHealBurstRegular`(41f9d0): signed count가 현재 'A' 구간 길이−1보다 작으면 현재 프레임+1을 지정(첫 프레임으로 되돌리지 않음)하고 payload를 돌려준다. 아니면 이벤트 2/0.05의 일반 Regular(flags 0x50)를 먼저 붙이고 `destroy(sid, 0x100048)` 뒤 −1을 돌려준다.
- `handleGravityBurstRegular`(41f760): 같되 count가 한계 이상이면 현재 프레임이 0일 때만 종료하고, 아니면 32비트 감김의 역방향 프레임(`total+total−count−2`)을 지정한다.
- 선택한 효과가 빠졌으면 앞선 HP/상태 변경을 되돌리지 않고 명시 오류로 알린다.

## 시험

`webpj/tests/raw-bomb-heal.test.mjs` 7건:

1. 원본 관측 `bomb-gravity-regular-x86.tsv` **122행**: signed count·프레임 감김/역방향·종료(일반 Regular 2 선행 뒤 삭제)·NaN payload·표시 갱신 순서(S/R/ND). 실제 `SquidProcessHost`·`RawSquidDestroy` 장면. **첫 실행에서 일치**했다.
2. cpppj `ClientBombHealOwner_*` 계약: 자격 마스크와 QA 선반환, signed DWORD 감김 평균 7가지와 최초 HP 저장, 상태 두 비트만 해제, 펜스→보행 순서와 현재 가상 표 재조회, 갱신 뒤 버스트 중심, 누락 효과 보존, 버스트 진행(NaN payload 보존)/종료.
3. 변형 확인에서 드러난 경계: 버스트 한계(길이−1)·signed count(0xffffffff)·종료 Regular의 부착 인자(이벤트 2·0.05·공유 아님·flags 0x50)·판본/표 크기/Host 풀/setter 풀 계약(메시지까지 확인)·TYPE 범위·갱신에 현재 가상 표 전달·버스트 중심의 단정도 저장.

**변형 확인:** 상수·오프셋·genus 마스크·HP 평균·효과 순서·정기의 한계/역방향 프레임/Regular 인자 75가지를 넣어 71가지가 시험에서 잡혔다(처음 17가지가 살아남았고 3번 시험으로 13가지를 잡았다). 남은 4개는 동등 변형이다:
`readDword`의 부호(`>>>0`↔`|0`)와 HP 합의 부호 확장(`maximum>>>0`↔`maximum`), 그라비테이션 역방향 프레임의 count 부호(`>>>0`↔`|0`)와 지정 인자 부호는 모두 32비트 감김 뒤 같은 DWORD이다.

`webpj/tests/fixtures/manifest.json`에 `bomb-gravity-regular`(122행) 항목을 추가했다.

## 남은 경계

- 가상 48/4c/88/90·457890 펜스·4af530 공장·004adc60/b0 HP 조회의 실제 소비자와 주문 시전 몸체·앱 수명 연결은 호출자가 공급한다(PORT_MAP `port`). 미티어도 같은 힐 버스트 정기를 쓰므로 연결 때 공유한다.
