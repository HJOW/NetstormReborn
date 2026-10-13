# cpppj 캠페인 자산의 실제 postPop 분배

2026-10-13, 마지막 디컴파일 수행 PC **VM-W11-CODEX**. AGENTS.md 및 두 LEFT_JOBS를 읽고 현재 PC가 최신 정밀 프로젝트와 같은 호스트임을 확인했다. 원본/복사본 게임·GUI·OS 진입을 실행하지 않았으며 보호 파일·AGENTS.md·dotnetpj·커밋·푸시를 변경하지 않았다.

## 실제 가상 슬롯과 등록

postPop은 두 PE에서 **vtable +0x20**이다. `+0x94`는 postPop이 아니다. 클래스 이름이나 타입 genus로 다른 가상 몸체를 대체하지 않고 실제 PE 주소를 읽어 분배했다.

`SquidPostPop::SetCarrierPostPop`은 이미 복원한 `RawCarrierPostPop`의 직접 Carrier/Damageable 몸체를 등록한다. 패치 Carrier의 표는 `00503d80/00503e50/0050e568`이고 CD는 `00503ee8/00506ea0`이다. Damageable은 패치 `0050a820/0050a8e8`, CD `00500a58/00500b10`이다. 다른 Carrier 파생 재정의는 이 목록으로 건너뛰지 않는다.

`RawAssetPostPop`은 아래 전용 몸체를 복원하고 `SquidPostPop::SetAssetPostPop`으로 연결한다. 모든 몸체는 내부에서 공통 장부를 한 번 호출한다. Activate에서 접두처럼 공통 body를 다시 호출하면 깊이가 두 번 줄어드므로 전체 wrapper의 호출 후 깊이를 확인한다. Pop 이전 Validate는 projected 좌표/해시 단계의 `RawGraphPop`을 전달받는다.

| 몸체 | 10.78 | 10.72 |
| --- | --- | --- |
| 제네레이터 | `0041c150` | `00459ed0` |
| 각도/그래프를 초기화하는 Gun | `0046a520` | `00436fe0` |
| 최초 Gun | `0046a600` | `00437100` |
| 캐논 계열 Gun | `0046aca0` | `00436eb0` |
| 썬 디스크 발사기 Gun | `0046a810` | `00437410` |
| Gun 장식 | `004694b0` | `004371f0` |
| 레지던스 | `0049a2b0` | `0044a4e0` |
| 배리케이드 | `0048eda0` | `004c0be0` |
| 가이저 | `004852e0` | `00415d30` |
| Vortex 공통 body | `004dc700` | `0044f440` |
| 레인 신전 wrapper | `004dce40` | `0044ff60` |
| 윈드 신전 wrapper | `004dd020` | `00450020` |
| 썬더 신전 wrapper | `004dd1f0` | `00450210` |

## 보존한 동작

Gun은 최초/abstract/전투/로딩 조건을 원본처럼 구별한다. 최초 캐논의 타입 `flags1 & 0x1000`은 로딩 중 HP WORD를 extra에 복사하며 이후 현재 extra의 하위 WORD를 HP로 쓴다. 패치 extra는 DWORD이고 CD는 BYTE이다. 썬 디스크 발사기는 기존 애니메이션 제거 후 GunProcess를 만든다. 원본 서버/로딩 assert 및 checkForms 경계도 보존했다. 각도 Gun의 `004ad3e0/CD 004adbd0`은 프레임 DWORD가 아니라 **그래프 BYTE(+30/CD+28)**를 지정한다.

제네레이터의 최초 `004ac970/CD 004ac440`은 타입 +0x98의 순수 조회이며 반환값을 버린다. 관찰 가능한 쓰기/예약 없이 Damageable로 이어진다.

레지던스는 현재 지면 소유자 → 목록 추가 → 각각의 Regular 확보 → 연결 알림을 처리한다. 패치는 사건 0/1/3을 예약하고 +42 DWORD에 현재 잔여 프레임을 쓴다. CD는 사건 0/1만 예약한다. 낮은 nibble을 지운 뒤 알림 효과가 바꾼 현재 WORD/extra를 다시 읽는다. 마지막에는 Damageable이 아닌 공통 Squid body를 직접 호출한다.

가이저는 최초 비abstract/비buried에 등록 목록/외부 등록 통지 → **type.cost의 CRT 절삭값을 +12 WORD 결정량** → Regular 사건0(0.1f)을 처리한다. 현재 위치의 실제 one-point iterator 순서로 섬 받침/종유석을 Unpop(0) → 프레임8 → Repop(0x40)한다. 마지막 Damageable를 보존한다.

Vortex는 Damageable를 먼저 호출한 다음 Player의 초기 신전·새 신전 및 복사 필드, 전역 개수, impactSound 재생, HP0, 중복 없는 목록, 선택 Vortex 배치 취소, 타입 extra, 섬 존재 검사, 영역 등록, 현재 소유자 재지정을 처리한다. boss이면 원본 iterator처럼 다음 번호를 삭제 호출 전에 확보하여 같은 영역의 다른 비buried Vortex를 가상 Destroy(0x100000)한다. CD에는 기존 섬 신전0 assert도 있다. CD 지역 조회는 float 좌표를 CRT 절삭한 signed DWORD를 16으로 나눈 정수 칸을 사용한다. 비로딩에는 Content slotType/0x10 조회·없으면 contained 생성·생산 갱신을 수행한다. 원소 wrapper는 이 공통 body 이후 extra와 사건1(레인/윈드0.08f, 썬더0.1f)을 처리한다.

## 독립 근거와 검사

`tools/ghidra/assetpostpop-functions.json`의 패치 **19개**, CD **17개**를 최신 정밀 프로젝트에서 읽기 전용으로 내보냈다. 결과는 `extracted/assetpostpop/`, 최초 내보내기와 iterator 추가 로그는 `extracted/assetpostpop-export*.log`에 있다.

`tools/decomp_assetpostpop_oracle.py`가 두 실제 PE의 전체 전용 body와 패치 SID iterator, CD body 안의 SID iterator를 실행했다. OS/API/게임 진입은 허용하지 않는다. 타입/부품/공간/새 객체의 외부 효과만 명시 입력 경계로 두며 Damageable/공통 body는 기존 독립 근거의 경계이다. 실제 body가 정한 사건 순서·raw 전체·목록 capacity/stale 메모리·Player 네 신전 필드·신전 개수·주변 받침 프레임을 fixture에 보존했다.

현재 독립 관찰은 **1,636개(판본마다818)**이고 정상 반환1,636회/assert0이다. 네 body 전투/로딩, 최초/abstract/buried, 개별 확보 실패, 가득 찬 목록, 현재 내용물/선택 취소, boss iterator/가상 삭제, 레지던스 signed /2 및 6/12/18·INT_MIN/MAX, 캐논 0x1000 유무, 가이저 비용 2^31/2^32를 포함한다. 실제 패치 iterator 시작12회/다음1,524회, 두 PE의 다른 신전 삭제 경계는 각각6회이다. 초기 잘못된 호출 규약/합성 메모리 실험 실패 로그를 보존하고 실제 기계어 스택 인자로 교정했다. 최종 공유 Release 빌드가 경고/오류0으로 완료됐으며 `--filter AssetPostPop_`의 새4개 검사/실패0을 확인했다. 두 C++ 재생 검사는 1,636개 원본 입력의 사건·raw·목록·Player 결과를 전부 대조했다. 직접 Carrier/Damageable 등록과 외부 효과 누락의 변경 전 거부도 통과했다. 집중 로그는 `extracted/assetpostpop-focused-initial.log`이다. 전체 회귀 및 상위 raw 월드 연결은 root의 후속 검사를 따른다.

자료: `cpppj/tests/fixtures/assetpostpop-x86.tsv`, `cpppj/recovery-assetpostpop-evidence.json`, `AssetPostPopTests.cpp`. 재감사 명령은 `python -X utf8 tools/decomp_assetpostpop_oracle.py --verify`이다.

## 실제 월드 연결의 남은 경계

이 복원은 전용 body를 원본 순서로 호출하는 작업이다. 다음 경계 몸체를 예약만 하고 기존 base의 payload 반환으로 처리하면 실제 전투/경제와 달라진다.

- `createGunProcess`: 실제 `0046a1a0/CD 00434090`의 생성/부착·GunProcess 프레임·탐색·발사·투사체·피해 실행을 연결해야 한다. 생성자의 +0x28 내장 SquidProcess는 gunProcessType(parentSid, form0, **flags0**)으로 부착하므로 기존 미복원 flags0 전송 경계를 0x10/0x50으로 바꿔 생략하면 안 된다. dirty BYTE0xff, 현재 double 시각(+0x10), WORD0(+0x18), DWORD0(+0x1c), 현재 group0/1(+0x20)를 저장하며 group에 원본 assert가 있다.
- Regular: 레지던스 사건0/1/패치3, 가이저 사건0, Vortex 사건1의 실제 클래스 Handle 몸체가 필요하다.
- 지면/영역/소유자: 실제 공유 해시·표면 WORD·섬 목록·영역 지도와 가상 Owner를 사용해야 한다. 복사 목록이나 단순 섬 boolean을 Vortex 섬 번호로 대체하면 안 된다.
- `typeDword`: 현재 로드 타입의 +0x74 impactSound 포인터/토큰과 +0x130/+0x134 프레임 글자 A/B의 첫 번호다. 후자는 기존 RiftTypeFrames::Run으로 조회한다. 테스트의 합성 값17/27/39를 런타임 기본값으로 쓰면 안 된다.
- slot: 현재 Content 목록 조회와 `RawContainedCreate`의 실제 Form 부착/생산 갱신에 연결해야 한다.
- 캠페인1-1 시작 지식의 썬 아비어리(sunAviary,85)는 Factory 재정의 `004538f0/CD 0047a4b0`이며 이번 지원표에 넣지 않았다. 전용 Factory 후처리·생산/비행체 실행을 후속 복원해야 한다. 썬 워커137은 직접 Carrier, 레인 배터리77은 제네레이터, 썬 캐논128은 GunSun에 해당한다.

가상 효과 후 타입/전역이 바뀌는 추가 입력, CD 기존 신전 assert 입력, 불일치 영역/소유자 경계를 넓혀 대조할 수 있다. 캠페인 초기 비표면 자산을 공유 raw 월드로 올리는 root의 작업과 같은 풀/목록/시계를 사용해야 하며, 이 body의 독립 통과만으로 캠페인1-1 플레이·완주나 원본 동일성을 증명하지 않는다.
