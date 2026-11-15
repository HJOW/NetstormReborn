# 아웃포스트 목록 수명 이식 (WB04-E556)

`o/RawOutpostLifecycle`(원문 10.78 004557c0 `PostPop`·00455840 `PreDestroy`)을 `webpj/www/core/raw-outpost-lifecycle.js`로 옮겼다. 2026-11-11 사용자 결정(스커미시 구현 때 Outpost도 구현)에 따른 12차 Outpost 범위 2번이다.
기준선 `225553df`와 현재 cpppj 소스가 같음을 확인했다. 같은 raw 풀과 팩토리 목록과 다른 전역 두 목록(`SquidPostPopList` 추가 목록·거리 후보 목록)을 빌리며 CD 10.72/1037 판본은 웹 범위 밖이다.

## 옮긴 범위

- `postPop(sid, flags)`(004557c0): 자산 상태/TYPE(free·contained·70 미만·188 이상)를 효과 전에 거부한다. 첫 Pop(flags 비트 0)이고 extra가 abstract/buried(9)가 아닌 일반 객체만 두 목록을 **먼저 모두 검사**한 뒤 중복 없이 추가(가득 차면 생략)하고
  현재 x,y로 지역 통지(`regionChanged`)를 한다. 어떤 경우에도 작업장 `postPop`을 같은 flags로 부른다.
- `preDestroy(sid, flags)`(00455840): 같은 선검사 뒤 일반 객체는 두 목록에서 모든 중복을 앞으로 압축 제거(비활성 꼬리 보존)하고 지역을 통지한 뒤 Damageable `preDestroy`를 같은 flags로 부른다(abstract/buried는 목록/지역만 생략).
- `RawOutpostLifecycle.preDestroyNative`(실제 455840): 자산/두 번째 목록/부모를 앞에서 검사하지 않고 extra → 첫 목록 압축 → 둘째 목록 압축(signed 현재 count·매 반복 현재 배열·음수 count 보존) → 현재 Y→X 좌표 → 지역 통지 → Damageable 부모(extra와 무관) 순서로 도달한다.
- `makeOutpostDamageablePreDestroyHooks(pool, damageable, hooks)`: 부모 삭제 경계를 같은 풀의 실제 `RawDamageablePreDestroy` 선검사/본문으로 교체한다(풀이 다르면 거부).

## 시험

`webpj/tests/raw-outpost-lifecycle.test.mjs` 6건:

1. 원본 관측 `outpostlifecycle-x86.tsv` 10.78(originals) **768행**: P/D 접두의 두 목록 전체(꼬리 포함)·개수·지역 통지(R)/base(B) 순서·raw 슬롯 쓰기. **첫 실행에서 일치**했다.
2. cpppj 계약: 같은 실제 `RawPlayerPlacementAnchor`/`RawDamageablePreDestroy`/`SquidFactory`에 연결한 등록·제거가 Player 기준점을 즉시 바꾸는 흐름, 손상 연결·누락 효과를 목록 변경 전에 거부하고 원본이 읽지 않는 손상 목록은 막지 않는 것.
3. 변형 확인에서 드러난 경계: `preDestroyNative` 순서·음수 count·물리 끝 중단·extra 단독 비트, 자산 TYPE 70..187 경계, 두 목록 선검사(첫 목록 손상/둘째 손상 모두)와 삭제 선검사 위치, 실제 Damageable 선검사(보드 밖 좌표) 연결.

**변형 확인:** 오프셋·마스크·검사·추가/삭제·지역/부모 호출 순서·native 압축·훅 연결 55가지를 넣어 52가지가 시험에서 잡혔다(처음 13가지가 살아남았고 3번 시험으로 10가지를 잡았다). 남은 3개는 동등 변형이다:
추가 시 가득 참 검사(`>=`↔`>`)는 `append`가 같은 검사를 되풀이해 같은 결과이고, native 압축의 목적지 물리 검사는 목적지가 항상 읽는 인덱스 이하여서 도달할 수 없으며, `object`의 최소 SID(5 미만)는 TYPE 검사가 먼저 같은 오류를 낸다.

`webpj/tests/fixtures/manifest.json`에 `outpostlifecycle`(10.78 768행; 파일은 세 판본 2,304행) 항목을 추가했다.

## 남은 경계

- 지역 소유(`RawRegionOwnership`)의 outpost 통지 분기 연결(12차 Outpost 3번)과 Factory/운반/AI 등 기존 Outpost 분기의 통합 장면 대조(4번), client 쪽(5번)·금지 유닛/건설(6번)은 스커미시 단계에서 한다.
