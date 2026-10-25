# Native State의 ClockSync·Zone·MetaZone 실제 저장소 (단위449)

현재 PC와 마지막 전체 정밀 디컴파일 PC는 **HJOW-Athlon**, 마지막 디컴파일 날짜는 **2026-10-23**이다. 기준 HEAD는 `539bba9a`다. 기존 정밀 C와 보호 PE를 읽기 전용으로 사용하며 원본·기존 fixture·디컴파일 산출물은 바꾸지 않는다.

## 연결과 복원

원문 cleanup의 세 하위가 Session 밖 실제 저장소를 쓰도록 준비했다. 전체 State1 부모 접합은 다음 작업이며 이 단위로 완료 판정하지 않는다.

| 원문 | 실제 연결 |
| --- | --- |
| ClockSync `004c1f20` | 기존 정확 본문을 재사용한다. ClientLocalStateGlobals의 connection `005c8bf4`→replyMode `005c8c00`→gameTicks `0055b4ac` 세 DWORD0이다. 로그인 State의 ticks도 같은 필드 참조로 바꿨다. 전체 시각 동기화 송수신·연결22칸 구현을 뜻하지 않는다. |
| Zone `004bfd60` | 장기 Zone의 DWORD0→+4 WORD0, 정확6BYTE를 복원했다. 기존 로그인8BYTE 물리는 같은 Client가 소유하고, 뒤 상위WORD는 보존한다. State 다섯144BYTE 블록과 합치지 않는다. 로그인·Join 송신/수신·metadata도 같은6BYTE를 직접 빌린다. |
| MetaZone `0042b1b0` | 기존 부모를 같은 장기 flag `00531760`·51×144BYTE `00546908`에 연결한다. 새 Client의 flag1은 보호 PE 초기값이며 Session 생성/반납으로 재초기화하지 않는다. |
| 마커 `00479520` | signed 상태>0, +4c DWORD SID 비0일 때 현재 가상+10 삭제(flags0)→동일 SID DWORD0이다. 생략된 경로는 삭제 장치를 요구하지 않는다. 삭제가 실패하면 SID0과 부모의 전체 표 초기화를 앞당기지 않는다. |

`ClearClientMetaZoneRecords`는 같은 Client 권한·GameAux·표면을 검증한다. 실제로 선택된 삭제만 현재 Session/동일 Pool을 요구하고 기존 `RawSurfaceWorld::Destroy`를 호출한다. 선택되지 않은 빈 기록·음수 상태는 원문 관문으로 생략한다. 필수 장치가 없는 삭제를 빈 성공으로 처리하지 않는다. 가상 삭제의 bool 반환은 원문처럼 무시하고 정상 복귀 뒤에 SID를0으로 쓴다.

원시 마커 leaf는 DWORD SID를 줄이지 않고 장치로 전달한다. 제품의 같은 Pool 경계에서는 범위를 검사한 뒤에만 Sid16으로 변환한다. 원본의 임의32비트 포인터 별칭과 유효한 제품 Pool 범위를 구별하며, 다른 SID16 미복원 지점을 이 단위로 완료 처리하지 않는다.

## 먼저 고정한 원본 증거

새 마커 제품 본문 전에 `tools/decomp_state_storage_leaves.py`로 원본 PE의 **63BYTE 마커·9BYTE Zone**을 관측했다. 마커 **86입력**과 Zone **10입력**, 두 x87 정밀도에서 정상94·명시 가상 중단2를 고정했다. signed 경계·SID0·상위WORD만 있는 SID·32비트 주소 산술·삭제 중 SID/상태 변경·중단을 포함한다. Zone의 CPU store는 실제 `0:4:0;4:2:0`이며 나머지16BYTE canary를 보존한다.

두 새 fixture와 [Native 보고서](../../cpppj/state-storage-leaves-native-20261026.json)는 최초 관측 후 덮어쓰지 않는다. 가상 삭제 본체는 이 CPU 관측의 명시 경계다. 제품 검사는 별도로 실제 Session의 기존 가상 삭제를 호출한다. 원본 게임 창과 OS 코드는 실행하지 않았다.

CD 10.72의 대응은 SourceMap상 마커 `0045a070`, Zone `0040b8b0`이다. 이번 이식·CPU fixture는 PATCH 10.78이며 CD 제품 연결 검증으로 확대하지 않는다.

## 검증

신규8사례로 원본96관측 재생, short physical의 선택 조회·완료 prefix, 실제 Client의 세 DWORD·Zone 폭, 최초/반복 MetaZone 정리, 필수 Runtime 누락·외부 Pool 거부, 두 실제 Session의 수명 보존, 실제 Join writer/reader의 같은 Zone 주소, 현재 월드 가상 삭제를 검사한다. 기존 테스트 본문과 기존 Native 기대값은 유지한다.

Source1의 **2,035소스 동결**, build1 종료0·경고/오류0, 집중 **480고유/480관측·실패0**(신규8개 전부)을 확인했다. 기존229사례 본문과 관련 Native fixture3개는 보존했고 실제로 다시 실행했다. 같은 최종 게임의 Early1 실제 로그인·안내·덱·미니맵 화면 GUI1경로도 통과했다. 원본/CD 전체1,819파일 집합·내용과 허용 설정은 전후 일치했다. Source1과 제품4파일·보호1,821파일도 유지했다.

게임 SHA는 `0da8d8988db73b3dfb05b47cec62a6df73ca5aeadf380fcb3f0e46ff43356d46`다. [실행 보고서](../../cpppj/state-storage-runtime-20261026.json), 원시 `extracted/cpp-state-storage-449-*`에 빌드·집중·정밀 C 발췌·PE 초기값·GUI·보존 감사를 남겼다. 이번에는 전체 CTest·긴 Early 완주를 반복하지 않았다. 447의 전체CTest3,052·자연Early2는 그때 제품의 증거로 유지한다.

## 남은 연결

전체34개 Effect와 비enum hook, Player/GameplayUse·고정뷰 반납 coordinator, State1 prefix·실제 Edit checkpoint/HUD 설치는 남아 있다. 255DWORD·20 pool header·Pool wrapper의 두40DWORD/위치 등 남은 작은 정리도 현재 실제 소유자와 원문 본문을 확인해 이어서 연결한다. 전체 시각 동기화·MetaMap 게임을 새로 복원한 것으로 설명하지 않는다.

전체 Early·캠페인·Priest Training·데모 자연 완주와 화면·음향 동일성 목표는 계속 진행 중이다.
