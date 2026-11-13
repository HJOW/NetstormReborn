# CD 10.72 튜토리얼 설치 경계 조사 (2026-11-14)

마지막 디컴파일 PC: HJOW-Athlon(변경 없음). 기존 `extracted/refined/originalCD/NETSTORM.c`와 10.78 산출물, 현행 소스를 정적으로 대조했다. 이 조사는 튜토리얼 실행 성공이나 게임 규칙 복원 완료를 뜻하지 않는다.

## 현재 호출 경로

`UberGump::StartMission` → `BeginMission` → `CreateMission` → `LoadMissionWorld` → `GameWorld`의 캠페인 생성자 → `RawCampaignSession`이다. CD에서는 `guided=false`로 두고 기존 동기 어댑터로 fort와 플레이어 표시 자료를 읽는다. `SessionAssets`는 CD를 명시적으로 거부하고, 그 뒤 World 생성도 `Patch1078`로 고정돼 있다. 실제 CD `RawSurfaceStorage`를 빌리는 상태에서 이 검사만 없애면 Pool·TYPE·프레임 판본 검사가 실패한다. 옛 관찰용 `GameWorld` 생성자로 우회해서 진입 완료로 처리하지 않는다.

| 경계 | CD 10.72 원문 | 현행 10.78 가정 |
| --- | --- | --- |
| TYPE | 전체 171행, 원문 레코드 stride `0x1d4` | 전체 188행, `0x1f4` |
| SID | 36바이트 슬롯 | 50바이트 슬롯 |
| Player | stride `0xac`, 신전 SID `+0x74`, brain `+0xa8` | stride `0xb4`, `+0x7c`, `+0xb0` |
| 전투 옵션 초기화 | 8바이트·기본 선택 7개 | 87바이트·기본 선택 22개 |
| 시작 SP | `00414650(float)` 직접 설정 | 소유자별 scrambled 저장소·PlayerKnown 관문 |
| AI 기본 자금 | `DAT_005120d0` 그대로 | `startingSp+4000` |
| 단계 예약 | float 현재 시각+float 지연을 double로 저장 | double 현재 시각+float 지연 |

`RiftTypeRecord`는 정규화된 공통 값형이므로 자산 판독 자체의 행 수와 원문 포인터 연산의 stride를 구별해야 한다. `SidPool`은 양판본을 지원하지만 `ClientPlayerRecords`는 현재 9×180바이트다.

## 원문 대응과 후속 순서

| 몸체 | 10.78 | CD 10.72 |
| --- | --- | --- |
| 미션 초기화 | `004c3290` | `0040f6e0` |
| 미션 시작 | `004c2b20` | `0040f840` |
| 공통 프레임 | 현행 RawMissionStart 계열 | `0040fef0` |
| Early 1 판정 | `004c3a20` | `00410c60` |
| 단계 증가 | `004c33f0` | `00410be0` |

자동 대응표의 CD `00411570`은 지정 단계로 이동하는 함수이며 단계 증가 함수로 그대로 쓰면 안 된다. CD 초기화의 전투 옵션 부모는 `00453520`→`00453540/00453570`, 24바이트 간격 기본 표는 `005200f8`이다. CD 시작의 AI 순서는 `004d6e60` 기본값→`0040fad0` 공통(-1)→소유자 설정→`004d7720` 생성이다. 기존 일반 Pop `004ad490`의 부분 복원을 활용하되 공유 Pool·Graph·후처리의 실제 소유자를 유지한다.

다음 순서로 작은 단위를 마무리한다.

1. CD Init/Start 기초: 8바이트 전투 옵션 초기화, 시작 SP와 600바이트 AI 설정 차이, 공통 프레임 정밀도. 이 단계에서는 Guide를 활성화하지 않는다.
2. CD Player·Early 1: 실제 Client 기록의 172바이트 지원과 신전·brain·기술/목록 접근, 같은 기록을 읽는 Early 1 판정.
3. CD 월드 설치: SessionAssets·World에 실제 CD 판본을 전달하고 CD 공장·Pop·Graph를 같은 저장소에 연결한다. 도달하는 전용 생성자는 원문 순서로 복원한 뒤 Guide·Start를 활성화한다.

최소 진입 이후의 전체 전투·모든 미션·CD LAN 호환 범위는 결정 D1과 구별한다. 현행 10.78 LAN 복원을 CD 호환 검증으로 표시하지 않는다. 사제 포획/희생·특정 적 공격·승리·MP 및 사용자 직접 확인 검사는 이 조사에서 실행하지 않았다.
