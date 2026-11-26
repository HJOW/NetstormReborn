# 초기 미션 다음 단계 안내 — 조건 조사1371

2026-11-27, HJOW-Athlon. 사용자 지시로 LAN 후속보다 튜토리얼 안내를 먼저 진행한다. 마지막 디컴파일 호스트도 HJOW-Athlon이며 기존 정밀 디컴파일과 복원 본체를 읽었다. 새 디컴파일·원본 실행·원본 파일 변경은 없다. 이 단위는 TUT-1 조사만 완료하며 제품은1370이다.

## 대상과 표시 원칙

공식 Early Missions의 Tutorial1~6만 대상이다. 일반 캠페인도 missionType=Tutorial이므로 유형만으로 범위를 결정하면 안 된다. 비데모·비편집·공식 미션 이름과 tutorialNumber1~6을 함께 확인한다. 각 단계의 실제 판정은 기존 RawTutorialOne/Two/Later와 공통 RawMissionFrameGate가 계속 수행한다. 안내는 현재 상태를 읽기만 하며 완료·SP·입력 사건을 만들지 않는다.

안내는 현재 지도 영역 우측 하단에 배경 없는 글자로 계속 표시한다. 생산 창·미니맵을 피하고 긴 문구는 여러 줄로 나눈다. 기존 글꼴/8비트 글리프·언어 선택을 공유한다. 단계 예약 중에는 다음 안내를 기다리는 문구를 사용하고, 현재 안내 창을 읽는 동안은 창과 겹치지 않게 숨긴 뒤 닫으면 다시 표시한다. 완료 단계와 미션 종료·다른 미션/메뉴 전환에는 제거한다.

## 단계별 실제 다음 조건

다음 열은 다음 단계에서 여는 섹션이다. A1. 등 같은 단계의 HTML 다음/뒤로 버튼은 단계 전환이 아니며 안내 목표도 바꾸지 않는다. 마지막 단계의 섹션은 완료창이다.

| 미션 | 현재 | 실제 조건 | 다음 |
| --- | --- | --- | --- |
| 1 | A | Home 명령(F4/H) 또는 다리 누적 제작 수>0 | B. |
| 1 | B | 다리 누적 제작 수>7(총8개) | C. |
| 1 | C | 다리 누적 제작 수>18(총19개), 뒤이어 간헐천 생성 | D. |
| 1 | D | 최근 생성 간헐천 중 하나와 내 신전의 Graph 일치 | E. |
| 1 | E | PlayerKnown이고 SP≥200 | F. |
| 1 | F | PlayerKnown이고 SP≥600 | G.(완료) |
| 2 | A | 신전 부류 누적 제작 수>0 | B. |
| 2 | B | 워크샾 부류 누적 제작 수>0 | C. |
| 2 | C | 썬 디스크 발사기가 생산 목록에 있음 | D. |
| 2 | D | 썬 디스크 발사기 누적 제작 수>0 | E. |
| 2 | E | 썬 디스크 발사기 누적 제작 수>1(총2개) | F. |
| 2 | F | 신전 선택 시 게임 시간4초 예약, 이후 기한 도달 | G. |
| 2 | G | 썬 디스크 발사기 누적 제작 수>3(총4개) | H. |
| 2 | H | Salvage 명령 시 게임 시간2초 예약, 이후 기한 도달 | I.(완료) |
| 3 | A | 크리스탈 수입 완료 사건 | B. |
| 3 | B | 내/적 사제 Graph 일치. 어느 한 사제가 없어도 원문 전환 | C. |
| 3 | C | 썬 디스크 발사기가 생산 목록에 있음 | D. |
| 3 | D | 윈드/레인/썬더 제네레이터 중 하나가 생산 목록에 있음 | E. |
| 3 | E | 썬 디스크 발사기 배치 사건, 내 시작 지역 밖이며 해당 좌표에 islandThreeByThree 부류 없음 | F. |
| 3 | F | 적 사제 없음 또는 적 사제 HP<최대HP/2 | G. |
| 3 | G | 내용물 사건의 첫 SID가 사제 부류이고 raw+0x22=0 | H.(완료) |
| 4 | A | 썬 디스크 발사기·윈드 제네레이터 모두 생산 목록에 있음 | B. |
| 4 | B | 썬 캐논 Global−Local=0 또는 DWORD 차의 signed 값<0 | C. |
| 4 | C | 썬 캐논이 생산 목록에 있음 | D. |
| 4 | D | 썬 디스크 발사기 Global−Local 조건(위와 같음) | E. |
| 4 | E | 사제 내용물 사건(raw+0x22=0), 운반자 TYPE133(골렘) | F. |
| 4 | F | 제단 TYPE165의 Local 수 signed 값>0 | G. |
| 4 | G | 제단 포로 배치 captureCommand 사건 | H. |
| 4 | H | 희생 완료 ascendCommand 사건 | I.(완료 안내, I1.은 HTML 다음 버튼) |
| 5 | A | 피격 사건의 투사체 TYPE132·피격 대상 TYPE78(스톤 타워) | B. |
| 5 | B | 썬 디스크 발사기·썬 캐논·스톤 타워·윈드 제네레이터 모두 생산 목록에 있음 | C. |
| 5 | C | 훨리베이스 선택 또는 적 썬더 캐논/훨리베이스 없음 → 게임 시간5초 예약 → 기한 도달 | D. |
| 5 | D | 썬더 캐논·훨리베이스 모두 Global−Local 조건 | E. |
| 5 | E | 내용물 사건의 첫 SID가 bomb 부류(주문 종류 제한 없음) | F. |
| 5 | F | 희생 완료 ascendCommand 사건 | G.(완료) |
| 6 | A | 에어 쉽·윈드 제네레이터의 Global 수 각각 signed 값>0 | B. |
| 6 | B | 크리스탈 수입 완료 사건 | C. |
| 6 | C | 크로스보우가 생산 목록에 있음 | D. |
| 6 | D | 크로스보우 Global 수 signed 값>0 | E. |
| 6 | E | 썬 디스크 발사기·훨리베이스·썬더 캐논 Global 수 모두0 | F.(완료) |

Global−Local은 단순 적 수의 수학적 비교가 아닌 원본 DWORD 감김 판정이다. 안내에서는 공식 배치에 맞춰 적 건물을 모두 파괴하는 행동으로 표현하되 조건 본체를 바꾸지 않는다. Built는 누적 제작 수이므로 살아 있는 현재 개수로 안내하면 안 된다. Tutorial6의 Global 검사도 임의로 로컬 개수로 바꾸지 않는다.

## 별도 알림·타이머와 근거

- Tutorial2 C에서 신전을 선택하면2초 뒤 NotVortex 안내가 나오며 선택이 지워진다. 이는 올바른 생산 목표를 대체하지 않는 선택 교정 안내다.
- Tutorial5의 윈드 신전 소실 VortexDestroyed와 B 이후 네 생산품 중 하나 소실 FactoryDestroyed는 단계 밖 일회성 경고다. 현재 단계 목표는 유지하고 경고창을 닫은 뒤 다시 표시한다.
- Tutorial2 F/H·Tutorial5 C의 타이머는 한번 예약하면 선택을 바꿔도 취소되지 않는다. 예약 뒤에는 기다리라는 문구를 사용한다.
- Advance는 즉시 창을 열지 않는다. stage 증가→lockFrames10→현재 게임 시간+delay 예약 후, 공통 프레임이 기존 다이얼로그 종료와 기한을 확인하여 TellStage한다. 안내는 이 순서를 보존한다.
- RawTutorialLater의 rangeCommand는 실제 Session에서 크리스탈 수입 완료 notice에 연결된다. technologyCommand는 투사체 hitCommand이고 priestCaptured는 HP 절반 미만 조회다. 멤버 이름만 번역해 안내를 만들지 않는다.

복원 본체: cpppj/src/o/RawTutorialOne.cpp(Handle), RawTutorialTwo.cpp(Handle), RawTutorialLater.cpp(Three/Four/Five/Six), RawMissionFrameGate.cpp(RunFrame/Advance). 현재 사건/TYPE 연결: cpppj/src/client/RawCampaignSession.cpp의 MakeTutorialOne/Two/Later(6044~6084행). 기존 디컴파일: extracted/refined/originals/Netstorm.c의004c3f00/004c40f0/004c42b0/004c45e0 및 Early1/2 대응 함수. 원본 본체 회귀는 TutorialOneTests/TwoTests/LaterTests의 기존 독립 PE fixture를 사용하며 변경하지 않는다. 완료 섹션과 HTML 세부 단계는 assets/lang/korean/d/tutorial1~6.korean으로 대조했다.

다음은 TUT-2 문구·언어 자산과 TUT-3/4 실제 UI 결선이다. 실제 실행·화면 검증은 아직 하지 않았다. 포획/희생·특정 적 공격/승리가 필요한 튜토리얼 후반 자연 전환은 사용자 확인으로 남기며, 단순 진입·대기·생산·건설로 확인 가능한 부분은 실행한다.
