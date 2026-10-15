# SP 저장소의 현재 상태와 Player 내용물 통지

작성일과 현재/마지막 정밀 디컴파일 PC는 **2026-10-15, VM-W11-Codex**다. AGENTS와 양 LEFT, 같은 PC의 기존 정밀 C/선택 export/보호 PE 근거를 다시 읽었다. 이번 단위의 신규 정밀 export/실제 게임/GUI/OS 실행은0이다. 격리 x86 원본 관찰만 재수행하며 보호 원본·AGENTS·dotnetpj·Session·공용 CMake는 변경하지 않는다.

이 단위는 `RawPlayerContentNotice`의 SP 정합성 접두에 실제 저장소의 현재 상태를 연결한다. 기존 값 반환 `Offset()`의 임시 참조, 초기값 사본, 프레임별 동기화, 상수 배열존재 토큰을 만들지 않는다. 등록표와 최종 UI 처리는 기존 현재 Gump 등록표/실제 직접 handle 경계를 유지한다.

## 실제 원본 필드의 소유

| PATCH 전역 | 실제 의미 | 현재 단일 소유/조회 |
| --- | --- | --- |
| `00568af8` | SP 읽기/쓰기/회전의 실제 비트 offset | `ScrambledSpStore::offset_`, `OffsetCurrent()` 안정 참조 |
| `0054d4e0` | 호출자가 별도로 증가시키는 추적 offset | 기존 `PriestRegenPatchState::sequence`, 저장소는 `BindTrackingOffset(int32_t&)`로 주소만 빌림 |
| `00594fd0` | 첫9DWORD 분산배열 주소 | `FirstPoolCurrent()`의 실제 `pools_.data()`, 초기화 전 NULL |
| `00531890` | 순열32개 중 첫 signed WORD | `Permutation().front()`, Get/Set/Rotate도 같은 실제 순열 배열 사용 |

`0054d4e0`을 저장소 안에 다시 값으로 소유하지 않는다. `BindTrackingOffset`는 첫 Initialize 전에 같은 외부 주소를 빌리며 늦은 설치/다른 주소로 교체를 거부한다. 연결하지 않은 독립 SP 사용은 기존처럼 동작하지만 `TrackingOffsetCurrent()`와 실제 Notice 바인딩은 오류를 낸다. 저장소와 외부 추적 상태는 조회기/Notice보다 오래 살아야 한다. 실제 배열 주소와 추적 주소의 수명을 유지하려고 저장소 복사/이동도 금지한다.

배열 저장은 기존과 같은 연속288DWORD다. 원본의32개 `operator_new(0x24)`를32개의 host 힙 블록으로 바꿔 배치하는 작업은 이번 범위가 아니다. 첫 포인터는 값1이나 원본처럼 보이는 임의 토큰이 아니라 **현재 실제 SP 몸체가 읽고 쓰는 첫 배열의 주소**다. 기존 성공 초기화/합성 힙 채움 모형을 유지하며 힙 할당 실패는 새로 모사하지 않는다.

## 초기화와 회전의 순서

실제 `0040eba0`은 다음 순서로 쓴다.

1. 실제 시각 재시드 경계 → `GameRandom::Next(100)` → 실제 offset.
2. 순열0..31 작성 → 같은 추적 offset에도 현재 offset 쓰기.
3.32회 `Next(32)`와 실제 WORD 교환.
4. 실제 offset을 유지하고 순열0..31을 다시 작성.
5. 분산배열 준비 → 초기화 완료.

이전 구현은 최종 항등이라는 이유로 순열 배열을 없애고 난수 소비만 남겼다. 이제 실제 배열을 보관하고 Get/Set/Rotate가 모두 이 배열을 따라 물리 DWORD를 선택한다. 난수 소비 순서와 저장 DWORD/논리 SP는 유지한다.

실제 `00491680`은 **actual offset만**3 증가시킨다. signed DWORD로9600보다 클 때 한 번9600을 빼고 각 현재 순열 배열의9DWORD를3비트 회전한다. 추적 offset은 원본 호출자가 따로 갱신하므로 `Rotate()`가 둘을 자동 일치시키지 않는다. 따라서 실제 불일치도 Notice가 현재 상태에서 관찰한다. 재초기화는 다시 두 offset을 함께 쓴다.

## 현재 읽기와 실제 통지의 연결

`PlayerContentNoticeRead<T>`는 lvalue의 주소 또는 실제 getter를 보관한다. 값 자체를 보관하지 않고 원본 읽기 위치에서 `Read()`를 한 번 수행한다. mutable/const 임시값의 직접 연결은 컴파일 단계에서 금지하고 빈 getter도 거부한다. 기존88행 raw fixture는 같은 lvalue 직접 연결로 유지한다.

`battleShown`도 같은 현재 조회형이다. 실제 Session의 `AssetPostPopState.inBattle`은bool이므로 `Bind([&]{ return assetState.inBattle ? 1U : 0U; })`로 비제로 조건을 읽는다. bool을 DWORD로 바꾼 임시값의 참조나 별도 DWORD 사본을 전달하지 않는다. 현재 localPlayer는 기존 DWORD 소유자 참조를 그대로 쓴다.

`BindSpStoreNotice(store)`는 네 현재 조회기를 구성한다. actual/tracking 비교는 DWORD 비트를 유지하고 signed 추적값은 `bit_cast<uint32_t>`로 읽는다. 첫 배열은 실제 포인터의 NULL 여부만 의미상 변환한다. 이 조회 결과를 원본 포인터 토큰으로 저장하거나 전달하지 않는다. 순열 첫 항목은 실제 signed WORD다.

```cpp
// 기존 사제 상태가0054d4e0을 소유하며 첫 초기화 전에 같은 주소를 연결한다.
sp.BindTrackingOffset(priestState.regen.patch.sequence);
sp.Initialize();

// 네 읽기는 자체 값/프레임사본을 만들지 않는다.
const auto currentSp = BindSpStoreNotice(sp);
PlayerContentNoticeState noticeState{
    currentSp.cursorA, currentSp.cursorB,
    priestState.regen.patch.changedSample, priestState.regen.patch.sample,
    priestState.regen.patch.remaining,
    PlayerContentNoticeRead<std::uint32_t>::Bind([&] { return assetState.inBattle ? 1U : 0U; }), actualLocalPlayer,
    currentSp.firstSpPool, currentSp.firstPermutation, actualGumpFormType, actualContentEvent
};
// 실제 SP money/현재 Gump 등록표/직접 가상 handle은 기존 필수 경계로 공급한다.
RawPlayerContentNotice notice(pool, noticeState, actualNoticeHooks);
```

위 코드는 루트 Session 조립의 API 예시이며 실제 Session/UI 설치 완료를 뜻하지 않는다. 특히 money가 lazy 초기화한 경우 같은 Notify의 뒤쪽 배열/순열 검사는 즉시 그 현재 상태를 읽는다. callback 앞에 읽은 네 값을 묶어 snapshot으로 재사용하지 않는다.

## 독립 근거와 검사

기존 `tools/decomp_sprotate_oracle.py`의 **61고유입력**을 그대로 실제 PATCH PE에서 다시 실행했다. 실제 초기화/읽기/쓰기/회전/난수 몸체이며 malloc/시각 재시드만 기존 경계다. 재생성 전후 fixture SHA-256은 모두 `4497a9bc627a0a0ce2dc2da721c4c5e2599e5f07796a1009e3bbfcbc54d4fefc`로 전체 출력이 같다. 저장소 소스 SHA를 포함하는 기존 보고서는 최신 소스로 갱신했고 `--verify`61행이 통과했다. 생성기의 기존 날짜 필드는 보존하고 보고서 `revalidated`에 실제 재실행 날짜/PC와 신규고유0을 별도로 기록했다. 기존61을 신규 고유입력으로 더하지 않는다.

Player 통지88입력도 동일 PE에서 재생성해 정상반환88·wrapper82/본문88/등록getter26회를 확인했다. 재생성 전후 fixture SHA-256은 `a1959c5abc986ade068e4973b4dbd7c9202052ad9add1c648afb0602b783a68e`로 같고 `--verify`가 통과했다. 이88 역시 신규 고유입력이 아니다.

새 집중4개는 다음을 검사한다.

- actual/추적 offset/순열/첫 포인터의 안정 주소, 초기화/회전/재초기화의 현재 값, 실제 `pools_.data()`와 동일 주소.
- 61개 기존 원본 관찰의 매 명령 끝점에서 actual/tracking DWORD를 별도 사본 없이 현재 조회해 대조.
- 실제 Player 내용물 접두에서 초기화 전 NULL, 같은 money callback 안의 lazy 초기화, 현재 SP7000.5, 현재 추적 불일치/회복을 같은 저장소로 관찰.
- 미연결/늦은 추적 연결/다른 주소/빈 조회기/임시값 연결 거부.

루트의 **Release 게임/검사 빌드 경고0·오류0·새 집중4개 실패0**가 통과했다(`extracted/pause-final-build.log`, `extracted/pause-final-focused.log`). 이후 페이지 강제 재컴파일도 경고·오류0이다(`extracted/pause-metadata-final-build.log`). 기존 SpRotate4개와 CampaignLifetime3개/보상 회귀를 포함한 최종 전체 CTest도 **1/1·내부1,233개·실패0, 112.41초**로 통과했다(`extracted/pause-final-ctest.log`). 이 독립 모듈 자체의 직접 GUI 검사는0이며 Session/UI 조립의 GUI 결과는 별도 문서로 구분한다.

## 남은 경계

`RawPriestRegen` 자체의 `observedSequence`/`gate`/`sentinel`은 기존 독립 fixture의 값형 필드다. 이번 Notice는 이 사본을 읽지 않는다. 사제 회복 몸체까지 실제 저장소를 공유하려면 patchAudit 뒤 다른 실제 효과가 끝난 현재 전역을 getter로 읽는 별도 연결이 필요하다. 매 프레임/매 callback 끝에 offset을 복사해 원본의 현재 읽기와 같다고 주장하지 않는다.

사용자의 일시 중단 요청으로 이 후속은 **원문 조사/설계 단계에서 중단**했으며 Regen 제품/검사/도구의 새 편집은0이다. 재개 때 다음 내용을 확인한다.

- 실제 `00494580`의 event`0x25a`는 `sample=Next(9)`와 추적 offset+3 → 조건부 HP효과 → 실제 `00491680` → 현재 중립 조건/`00491db0` → actual/tracking 비교 → 조건부 `0040ee90` 측정/현재 잔여 감소 → 현재 첫 배열 → 현재 signed WORD 순서다. 중립 효과 뒤 actual offset을 읽어야 하며 측정 뒤 offset을 한 번 더 비교하는 새 효과를 넣으면 안 된다.
- 제안한 작은 선택 API는 `RawPriestRegen::BindSpStore(const ScrambledSpStore&)`다. 같은 외부 추적 주소 `&store.TrackingOffsetCurrent()==&state.patch.sequence`와 PATCH판을 검증하고 actual/첫배열/순열의 세 읽기를 위 원본 위치에서 같은 저장소에 직접 보낸다. 값형 필드는 기존 독립 fixture backend에만 남긴다. 불필요한 공개 template를 추가하지 않는다.
- 기존 C++의 `gate==0 || sentinel==-1`을 실제 원본의 별개 if 두 개/순서대로 읽고 쓰도록 복원한다. 현재 sample/잔여/진단대상은 기존 같은 사제 상태의 소유 주소를 유지한다.
- 집중 검증은 일부러 틀린 fixture용 observed/gate/sentinel이 설치 backend에서 무시되는지, 중립 action이 실제 저장소를 다시 회전한 뒤의 현재 비교, 측정 callback 뒤 첫배열/순열의 현재 읽기, 측정 뒤 추가 offset비교가 생기지 않는지를 다룬다. 기존1513행×3판본의 native 회복 fixture는 별도 근거이며 새로운 입력으로 더하지 않는다.

이 설계는 아직 구현/통과 결과가 아니다. 공유 Session/LEFT/커밋과 일시 중단 상태 기록은 루트 담당이다.

실제 Gump 등록표/nonnull handle/UI, 공통삭제·건설Confirm/Notice·AI/시작, 건설/다리/수확·운반·승패 후 저장·Early Missions/캠페인 완주와 전체 동일성은 별도 진행 중이다. DirectDraw 전체화면 문제는 요청대로 제외한다.
