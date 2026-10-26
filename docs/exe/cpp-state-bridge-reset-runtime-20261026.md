# State 정리의 소유자 색·다리 스캔 초기화 — 단위460

2026-10-26 HJOW-Athlon에서 원본10.78을 기준으로 작업했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**이며 산출물을 변경하지 않았다. 전체 State1이나 실제 Edit 설치의 완료 기록은 아니다.

## 원문과 현재 저장소

제품 변경 전에 실제 PE의 `0043b540`·`00422490`을 두 FPU 제어값과16개 입력으로 실행해64관측을 동결했다. 게임/OS/하위 효과를 치환하지 않았고 원본 게임을 프로세스로 실행하지 않았다. 실제 store의 주소·폭·순서와 네 영역212BYTE를 관측했다. 현재 색·범위·시각뿐 아니라 이웃 canary도 기대 자료에 포함한다.

- `0043b540`은 `00531b08`부터9DWORD에0..8을 쓴다. 열째 칸, TYPE, 객체 프레임이나 현재 모드를 초기화하지 않는다. 실제 표는 이미 `RawSurfaceStorage.links.ownerColors`에 있고 World 연결·다리/받침 표시·파편·건설 표시도 이 표를 읽는다.
- `00422490`은 현재 `0052f968`의 float 지연과 `0055b4d0`의 double 게임 시각을 **첫 store 앞에서** 더한다. 그 뒤 first15000 → last23001 → cursor15000 → double 기한 순서로 쓴다. 게임 시각을 float로 줄이지 않는다.
- 새 PE 메모리 이미지의 초기값은 지연10, first/last **-1**, cursor0, 기한0이다. Source1의 새 필드는 first/last0으로 두었으나, 집중 검사 전 실제 초기 바이트`ffffffff`를 확인해 최종 제품에서 교정했다. Runtime 대여 때 매번 초기화하지 않고 같은 장기 스캔을 보유한다.

[Native 보고서](../../cpppj/state-bridge-reset-native-20261026.json), [동결 관측](../../cpppj/tests/fixtures/state-bridge-reset-x86.tsv), [관측 도구](../../tools/decomp_state_bridge_reset.py)를 본다. CD10.72의 대응은 색 초기화`00495d10`/표`005365a0`, 스캔`00449200`이다. 이번에 CD 몸체를 실행하거나 같은 상태라고 판정하지 않았다.

## Client 연결과 호출 위치

`ResetClientStateWorldStorage`가 같은 Client/권한/GameAux/표면 정체성을 먼저 확인한다. 색 표 정리`Clear43b540`은 원문상 Core0 선택과 World 반납 앞에 있으므로 현재 World가 빌린 같은 표에도 허용한다. 실제 World의 `Core().bridgeState`와 장기 표는 같은 주소다.

스캔`Reset422490`은 Pool reset 뒤에 있으므로 World 대여가 남아 있으면 기존 다른 물리 정리와 마찬가지로 store 전에 거부한다. 정상이면 현재 `Client::Time().game`과 같은 Storage의 float 지연을 읽어 초기화한다. 기존 여덟 물리 정리의 대여 관문은 유지했다. 색과 스캔을 새 World나 고정 초기값 사본으로 대신하지 않는다.

## 검증 범위

신규5검사는 Native64의 전체212BYTE, 색9칸/열째 canary와 부족 span 거부, 실제 대여 중 World의 같은 색 표, 스캔의 반납/다른 소유자 저장소 거부와 현재 지연·시각, 준비 대여 전후 주소/내용 보존을 확인한다. 기존 Native State 부모294관측·색/다리 관련 검사와 Campaign/Editor 회귀도 함께 실행한다. 최종 결과·SHA·원시 경로는 [실행 보고서](../../cpppj/state-bridge-reset-runtime-20261026.json)에 기록한다.

첫 집중에서는 새 거부 검사가 Client 두 개를 동시에 구성해 `Only one client may exist`에서 중단했다. 실제 정리 함수에 도달하기 전의 검사 입력 오류였다. 단일 Client에 독립된 다른 권한/GameAux의 표면 저장소를 넘겨 같은 owner 거부와 두 스캔의 미변경을 확인하도록 고쳤다. 기존 단언·Native 기대·제품 본문을 완화하지 않았고 최초 실패를 보존했다.

최종 Source3(2,059소스), build3 경고/오류0, 집중222고유/222관측·신규5개·실패0이다. 게임SHA는`01324a3b…`이며 기존 검사 본문·653자료(TSV650개)와 원본/CD 등 보호1,821파일을 유지했다. 현재 등록 이름3,135개는 전체 실행 수와 구분한다.

전체 CTest와 GUI는 이번 단위에서 반복하지 않는다. 마지막 전체 실행은459의3,130개 통과와 실제 Early1 ESC·메뉴 복귀·재진입이다. 기존 검사 본문/Native 기대를 변경하거나 이번 모듈 통과를 자연 플레이 완료로 확대하지 않는다.

## 이어서 연결할 범위

이 작업은 Native State1 조립에서 빠진 두 실제 Effect를 제공한다. 아직 `CleanupStateWorld`의 전체34Effect/Runtime 반납 coordinator·State1 prefix가 실제 Edit 예약을 소비하지 않는다. 별도22칸 송신버퍼의 실제 writer/반납과 남은 하위도 같은 부모 순서에 접합해야 한다.

다리 스캔의 현재 저장소와 reset은 마련했지만 원문 전체 Frame 스캔·붕괴 순회를 실제 프레임에 새로 연결한 것은 아니다. 기존 `BridgeDecayScan`/Native 대조 및 수명 처리의 복원과 실제 프레임 호출을 구분한다. 이 경계와 전체 미션 자연 완주·픽셀/음향 동일성은 후속이다.

원시 자료는 `extracted/cpp-state-bridge-reset-460-*`에 보존한다.
