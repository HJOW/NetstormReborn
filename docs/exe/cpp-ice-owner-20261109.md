# 아이스 타워의 장기 본체·현재 역할·가상 부모 연결

제품 **1207 / 11.01**, 2026-11-09, HJOW-Athlon. 문서 정리 1208에서 보존한 미완료 구현을 마감한 단위다. 전체 목표는 미완료이며 [현행 인계](../../LEFT_JOBS.md)의 M6-n을 이어간다. [결과 보고서](../../cpppj/ice-owner-20261109.json)에 소스·제품 SHA와 원시 경로를 기록한다.

## 구현

`RawSurfaceStorage`가 같은 Pool·현재 TYPE/프레임·권한·로딩 깊이·실제 Host를 사용하는 `RawIceTower`를 소유한다. Native Session은 이 본체와 역할을 빌리며 별도 세션 사본을 만들지 않는다. 기존 독립 typed 경로는 독립 본체를 유지한다.

| 원문 | 현재 연결 |
| --- | --- |
| `48ead0` | 성장형 PostPop. 실제 40바이트 Regular 예약 성공 뒤 현재 프레임 수로 `40/numFrames`, event `3a6`을 같은 Host에 부착하고 공통 PostPop을 호출한다. |
| `48edf0` | 완성형 PostDestroy. 현재 역할·extra/flags·로딩/지면 조건 → 실제 Unpop/Create/WORD/QA/가상 Owner/프레임/Pop → 기존 Damageable Post 순서를 공유한다. |
| `48eb80` | 성장형 PostDestroy. 실제 일반 탐색기로 받침을 찾고 선택된 섬 드롭 뒤 공통 Post를 호출한다. 이 경로에 Damageable Post를 추가하지 않는다. |
| `48ebe0` | 실제 Host의 성장 Regular. 현재 프레임·정지 비트·최종 생성 권한을 읽고 현재 완성형을 생성한다. 다른 event의 payload는 그대로 반환한다. |

- 3역할은 `GameAux.RainBlockerType`/`GrowingRainBlockerType`와 `links.noIslandType`의 **현재 DWORD 주소**를 빌린다. 결합 시 초기값을 덮어쓰거나 Player/월드를 초기화하지 않는다.
- 재성장 조건은 원문의 **서버 `00540bc0`**다. 전투 모드 `00594fbc`와 구별한다. 기존 RawIceTower 첫 bool의 `battle` 이름은 호환상 남아 있으나 Native/typed 조립 모두 실제 서버 값을 공급한다. 최종 타워 생성은 별도 권한 `00540bc4`다.
- 새 자식의 Owner는 원문 가상+74(`48eec5/48ec34`)를 호출한다. 현재 역할이 신전으로 바뀐 경우에도 같은 실제 Player 등록·생산 갱신·영역 변경을 수행한다.
- 보호 PE의 아이스 4VT에서 가상48/74/90이 각각 기존 공통 `4afe50/4adf00/4b02d0`임을 추가 대조했다. 현재 TYPE 역할이 다른 파생 객체를 가리킬 때의 Owner 분배는 별도로 유지한다.
- 지면 조회는 원문 `40eaf0`의 plain CRT 절삭·0~255 경계를 사용한다. 배치용 거의 올림과 구별한다. 일반 genus 조회는 기존 `RawDamageablePostDestroy::GenusAt`를 재사용한다.
- 큰 섬 드롭은 선택 시 기존 실제 장치의 함수 사본을 호출한다. 실제 Runtime이 필요한 하위를 빈 성공이나 임시 월드로 대체하지 않는다.

기존 83개 생성자 VT 중 `50e888/50e940/50ea08/50ead0`의 4부모 쌍을 추가했다. **Pre/Post 분배 70/83, 잔여 13, 전용 통지 잔여 6**이다. 이는 선택될 수 있는 큰 하위 효과까지 전체 NoWorld 연결을 완료했다는 수치가 아니다.

## 실패 보존과 교정

첫 조립 빌드 뒤 C/PE를 다시 대조해 재성장 조건을 전투 모드에서 실제 서버로, 새 자식의 단순 공통 Owner를 실제 가상 Owner로 교정했다. 첫 집중 결과는 **191/194·3실패**였으며 문서 정리 1208에서 [미완료 인계](cpp-in-progress-1207-20261109.md)에 보존했다.

3실패의 원인은 검사 입력이었다. 실제 `50e940`은 TYPE78과 TYPE147이 공유한다. VT 첫 항목을 고르는 검사 helper가 TYPE78을 만들었으므로 원문의 현재 역할 147 조건에 들어가지 않았다. 재성장 2건과 서버/전투 독립 관문 검사를 **실제 현재 역할 TYPE147 생성**으로 교정하고 VT/TYPE 단언을 추가했다.

성장형 받침 검사는 매몰 상태의 noIsland를 hash0에만 넣고 있었다. 일반 finder는 매몰 객체와 해당 표면 단계를 건너뛴다. 실제 rubble genus를 가진 **TYPE94**를 만들고, 매몰되지 않은 상태와 실제 SHP 크기에서 구한 `ObjectLevel`/`Bucket`에 연결했다. 삭제 전 실제 `GenusAt`가 이 SID를 찾는지도 확인했다. 원문 탐색기나 기대값을 약화하지 않았다.

`extracted/cpp-resume-1207/first-observed-tests.json`, `first-observed-test-*.log`, `diagnostic-*.log`, `before-diagnostic-*`, `corrected-input-*`에 교정 전후 원시를 보존했다. 금지한 외부 효과는 계속 예외로 남기며, 검사는 그 선택 지점과 앞선 실제 Pool/WORD/QA/Owner/Player/삭제 깊이를 확인한다.

## 검증

- 최종 Release/x64 제품 2·집중 5대상 **빌드 89137 종료 0**, 42.95초, 경고·오류 0, 소스 11파일 고정.
- 집중 **16289 종료 0**, 비MP **194/194**, 44.23초. 신규 ClientIce 10건과 기존 IceTower fixture 1건을 이전 183건에 추가했다. 기존 fixture는 독립 원문 **1,426행**의 효과 순서·반환·세 슬롯 전체 바이트를 대조한다.
- 새 검사는 현재 역할/같은 장기 수명, 4부모 분배, 실제 성장 Regular 예약/재진입, 알 수 없는 event/NaN payload, 최종 생성 권한, 빈 받침/선택된 드롭 실패, 현재 역할 재성장과 가상 Owner를 확인한다.
- 같은 최종 EXE로 Tutorial4에서 정상 UI 생산/건설 요청 → **TYPE75/owner1 실제 생성·요청 0→1 → 600프레임 대기**를 확인했다. 게임 시각 13.645→24.199, 정상 종료 0. `015-generator-settled.png`를 시각 확인했다.
- 같은 EXE/Client 800×600의 Edit TEST01↔Main 2회 왕복과 정상 종료, 실행 자료·보호 파일 불변은 결과 보고서의 `editor`와 원시 `live-checks-fixed.json`으로 확인한다.
- 원문 9함수(CD 대응 2개 포함)의 C/PE 범위를 보존했다. 마지막 정밀 디컴파일은 Client 양판본 **2026-11-03 / HJOW-Athlon**, R **2026-11-07 / 같은 호스트**다. 원문 변경·새 디컴파일은 0이다.

## 남은 범위

큰 건설 효과 16개와 Client 장치 10개의 기존 수명 계약은 그대로다. 큰 섬 드롭·비영 AI·다른 Form/Regular/배치 호출자·Fort 상위 Finish·M6 전수 owner 감사와 l/m은 남는다. 다음은 썬 펜스의 이웃·표식·연결선·재생성·수명 묶음을 같은 장기 owner에 연결하는 작업이다.

성공한 실제 전체 재성장/전체 자연 완주·건물 완성/Test Battle 검증으로 확대하지 않는다. 특히 이 연결을 **NB33 원인 조사/수정 완료**로 세지 않는다. 사용자 제외 포획/희생·특정 공격·승리·공중 3종 전용 관측·MP 단위/패킷/통신/게임/기록 검사는 실행하지 않았다. 상위 16계획·등록 버그 34건·최신 전체 진입 제품644/검증645·최신 ZIP1092·최신 전체 솔루션 통과 기록 없음은 유지한다.
