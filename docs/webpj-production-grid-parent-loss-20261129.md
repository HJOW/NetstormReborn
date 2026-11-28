# 생산 격자 부모 소실 — WB-GRID-1·WB-GRID-2

2026-11-29, HJOW-Athlon. CP1386 `0cca2c8dd`의 [원인·보완](exe/cpp-production-grid-parent-loss-1386-20261129.md)을 웹 제품 구현과 대조한다. 웹 이식에는 디컴파일 자료를 수정하거나 원본 게임을 실행할 필요가 없다.

## WB-GRID-1 재현 — E1491 완료

`tests/production-grid-parent-loss.test.mjs`는 실제 `ClientProductionPanel`·두 `ClientInventoryGrid`·`SidPool`·`SquidFactory`·`RawSquidDestroy`·`GumpFormHost`를 사용한다. 공개 자료의 TYPE80을 생성하되 공간에 Pop하지 않고 부모로 붙여, 해당 부모의 실제 공통 삭제가 UI 통지 → 종속 Form 삭제를 수행하도록 한다. 예산 출력만 기본 Gump 경계로 제공하며 빈 부모이므로 항목 생성은 선택 시 명시 실패한다. Session·실제 플레이 검사는 아니다.

| 경로 | 수정 전 관측 |
| --- | --- |
| 부모 삭제 직후 | 두 격자 모두 `0x40=64`·object=5·form=6/7을 보유한다. 두 Form은 공통 삭제로 이미 반납됐다. |
| ① 즉시 패널 닫기 | 예외 없음. 반납 슬롯의 TYPE3 기록과 dead 비트 때문에 재삭제가 자연 반환한다. |
| ② FIFO 번호 재사용 뒤 패널 닫기 | 두 번호가 TYPE80으로 재사용되면 `Client Gump의 실제 Form 삭제 장치 누락` 예외. 재사용 슬롯 바이트는 보존되지만 패널 닫기는 실패한다. |
| ③ 새 기술 부모 → `handleHudMode(3)` | 남은 `0x40` 때문에 `Client Gump 객체 부착의 현재 수명 오류` 예외. 기존 부모·Form 번호도 남는다. |

Form 통지 전후에 슬롯의 free 비트를 읽어 둘 다 false임을 확인했다. 따라서 통지 안에서 Form을 먼저 삭제할 필요가 없고, 원래 공통 삭제 순서에 맡길 수 있다. 재현 검사 3/3 통과(`webpj/extracted/webpj-e1491-grid-before.log`). 첫 준비 실행은 장기 Storage의 표시 view 장치를 요구해 도달하지 못했으며, 수명 검사에 필요한 실제 Factory·Unpop을 같은 Pool에 직접 조립해 바로잡았다. 첫 실패는 제품 결함으로 세지 않는다.

이 단위의 검사는 수정 전 결함을 고정한다. WB-GRID-2에서 성공 계약으로 단언을 바꾸고 실패 재현 → 수정 후 통과를 함께 남긴다. 제품 소스·운영 번들은 아직 바꾸지 않았다.

## WB-GRID-2 보완

다음 단위에서 격자 `bind(token,0)` 경계의 `0x40`·Form만 지우고 object와 공통 Form 삭제 시점은 유지한다. 일반 `ClientGumpRoot.bindObject(token,0)`의 원본 무동작도 보존한다.
