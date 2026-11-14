# webpj 마나 볼트 주기 RawManaBoltRegular (WB04-E468, 2026-11-15)

기준선 `1f00a25a`의 `o/RawManaBoltRegular.cpp/.h`(원문 `45a8e0`)를 `www/core/raw-mana-bolt-regular.js`로 옮겼다. 5-B의 `o/` 마지막 todo 단위이며(카메라 투영 선행 `RawCameraView`는 E465에서 끝남), 마나 볼트의 주기 프레임과 건설 대상 옆 화면 위치를 정한다.

## 이식한 것

- `ManaBoltRegularState`(다음 실행 간격 `0.05f`, 아이콘 간격 `16`)와 `RawManaBoltRegular(pool, types, frames, shapes, arrival, state, hooks)`: 판본(10.78)·188 TYPE 표 크기·같은 월드의 `RawManaBoltArrival`만 허용한다(`TypeError`).
- `run(sid, event, count)`: 현재 프레임에서 방향 글자를 읽어(`pathDirection`) 그 글자 구간의 첫 프레임과 `count`의 **signed 나머지**로 `setFrame`을 호출한다(구간 표/글자 오류는 거부). `event 0 && count 0`일 때만 위치 소리(`influenceIcon.wav`)를 낸다. `event !== 1`이면 간격 `0.05`를 돌려준다. `event 1`이면 같은 finder의 현재 대상(`arrival.target`)을 찾아, 없으면(또는 extra 비트 1 없음) 자기 삭제 뒤 `-1`을 돌려주고 raw를 다시 읽지 않는다. 있으면 대상 좌표의 현재 화면점에서 (SHP frame0 폭+핫스폿+간격)을 X에서, (WORD+1)×간격을 Y에서 **DWORD 감김 산술**로 빼고, 현재 카메라 역투영으로 월드 float를 구해 `unpop → pop`한 뒤 간격을 돌려준다.
- 자기 TYPE frame0은 기본적으로 `shapes[type]`(적재 여부·frameCount·frames·maxHitPoints sentinel 두 값을 검사)에서 읽고, 선택 장치 `frameZero(type)`가 있으면 그 결과를 쓴다.

## 기준선 뒤 판정 (3.6절 ②)

`dfde4ad3`(20261110-49, "Fountain 정기와 도착의 현재 장기 소유자 연결")이 이 단위를 바꿨다: `Handles`가 TYPE 바이트(105)를 선검사하지 않고 가상 표 `0x509810`만 보고, 자기 TYPE frame0을 읽는 선택 장치 `frameZero`가 생겼으며, 호출 중 교체돼도 안전하게 선택한 함수의 사본을 쓴다(웹은 JS 참조 의미라 영향 없음). 세 변경은 모두 원본 관측(기준선 196행의 입력은 TYPE 105 + 가상 표 쌍)과 충돌하지 않아 **② 반영**했다(base `dfde4ad3`). 나머지 TYPE 검사 제거는 `handles` 시험이 TYPE 바이트가 달라도 가상 표만으로 받는 것을 고정한다. Session의 Fountain 정기 연결과 장기 소유자는 4-D 몫이다.

## 검증

- 원본 10.78 관측 **196행**(`mana-bolt-regular-x86.tsv`, 두 정밀도)의 반환 float 비트·최종 프레임/위치 DWORD·호출 순서(`S/F/U/P/K`)가 첫 실행에서 모두 일치했다. 카메라는 행의 관측 바이트를 직접 쓰고(`CameraView.setPixels`가 만든 월드 비트와 일치함도 단언) 투영/역투영은 기존 `projectCameraCoordinates`/`CameraView.screenToWorld`를 쓴다. manifest `mana-bolt-regular`로 연결했고 기존 fixture를 수정하거나 다시 실행하지 않았다.
- 독립 시험 5건: 생성/분배/선택 장치·`frameZero` 계약, raw 읽기 폭(가상 표 DWORD 뒤 바이트·WORD 필드가 인접 바이트를 섞지 않음), 프레임 구간(낮은 비트·`A`..`P` 경계·signed 나머지), 대상 이동과 SHP 검사 항목별 거부, 생성 검사 조건별 거부.
- 변형 확인: 자동 변형 76개 중 첫 라운드 생존 15개를 시험 보강 뒤 6개로 줄였고 남은 6개는 모두 동등 변형이다(`DataView` 길이, 방향 범위 검사 세 곳은 직후 `run`의 글자 검증이 같은 오류를 내므로 중복, `run.count <= 0`은 자기 프레임이 항상 자기 글자 구간에 속하므로 도달 불가, 대상 extra 검사는 `arrival.target`의 필터와 중복).
- 첫 실행에서 어긋난 곳은 없었다(관측 196행과 계약 시험 모두 일치). 모듈은 고치지 않았다.

## 남은 것

`mana-bolt-display-x86.tsv`(433행)는 `CursorManaTests`의 `RawCursorMana` 표시 시험이 읽으므로 그 단위(todo)를 옮길 때 연결한다. 실제 Fountain 정기/Session 연결과 효과 장치(`setFrame`·`pop` 등)의 결선은 4-D/5-C 이후다.
