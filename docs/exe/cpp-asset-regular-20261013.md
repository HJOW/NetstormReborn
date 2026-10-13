# 레지던스·가이저·신전의 실제 Handle 복원

2026-10-13, 현재 호스트와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. `AGENTS.md`와 두 인수인계를 읽었고 기존 `RawAssetPostPop`, `RawFrameAdvance`, `RawSquidFinder`, `SquidProcessHost` 몸체를 재사용했다. 보호 원본·AGENTS.md·dotnetpj를 편집하지 않았다.

## 복원한 가상 분배와 원본 필드

| 몸체 | 10.78 | CD 10.72 | 처리 |
| --- | --- | --- | --- |
| 레지던스 Handle | `0049a4e0` | `0044a710` | 주민 low nibble 토글→표면 통지→짝수/홀수 프레임→공유 난수15~35초, 실제 주변 장식 finder/코드flags, event2, 패치event3 지면 owner/첫 같은 owner 신전의 테마 |
| 가이저 Handle | `00485640` | `00416070` | 패치 가상 표시 갱신→결정량WORD/cost 비율→직접DWORD 프레임·0.03/0.06/0.09, CD 실제 Advance→`min(0.19f−ratio*0.12f,0.03f)`, 공통 Damageable |
| 신전 Handle | `004dc220` | `0044efd0` | event0 Ascend 정리, event1 실제 Advance/편집 역방향, event2 매32회 휴지·현재 프레임/난수, event3 이탈/섬조회/요청, event4 지역·애니메이션 통지, 기본 Damageable |
| Damageable Handle | `0044b900` | `00461a40` | event6699 좌표효과→가상+98→현재genus, event6700 abstract 건설 재시도→type30 조회→Start(parent,0), 실제 base payload 반환 |

`RawAssetRegular`은 부모의 실제 raw vtable 값으로 분배한다. `RiftTypeRecord`, 현재 프레임 표, 타입+98 원소 값, `GameRandom`, `AssetPostPopState::temples`, pool/hash와 Frame/Advance를 참조하며 복사본 월드를 만들지 않는다. `Handler(fallback)`는 기존 사제·보호막 처리기와 합성하고 미복원 부모를 정상 payload 반환으로 처리하지 않는다. 필수 효과가 없는 사건은 예외로 진단한다.

Regular 생성자 float는 최초 지연이 아닌 payload다. 실제 최초 예약 시각은 **현재 게임 시각**이며 다음 Kernel 프레임에 최초 호출한다. 이 동작은 기존 `SquidProcessHost::AddRegular`가 처리하며 새 코드가0.08/0.1/8초를 최초 대기로 적용하지 않는다.

기계어에서 확인한 레지던스 event1의 범위는 X의 `ftol(X−48)..ftol(X+32)`, Y의 `ftol(Y−32)..ftol(Y+32)`다. **패치만 둘째 범위 하한을 X−32로 읽는 원본 결함**이 있으며 C++도 이를 보존한다. 검색 좌표는10..246으로 제한하고 사각형 끝 Y는 현재 Y+32를float로 저장해246과 비교한 뒤 절삭한다. 장식 변경 뒤에도 Next를 한 번 실행한다. 최대 조사 개수는 같은 전역에 기록한다.

패치 가이저 구간은 `0.75 < ratio <= 1`에서 첫 구간, `0.5 < ratio <= 0.75`에서 둘째 구간, 나머지에서 마지막 구간이다. 1/0.75 동일점, cost0·음수·NaN과 signedDWORD 프레임을 별도 입력으로 관찰했다. CD는 frameBYTE와 실제 방향 구간 Advance를 사용한다. 현재 단계1/1×1 합성 SHP의 표시 경계로 기존 `SquidFrame`을 조합한다.

두 PE의 장식 타입 초기값은102, 신전 event4 삭제 통지 대상 초기값은 자산 타입 번호가 아닌 **토큰 `0x02000013`**이다. 타입 테마는 theammode에서 원소2→6,0→12,1→18프레임 보정이며 모드0/다른 원소는0이다.

## 이름을 교정한 실제 하위 효과

`004db8e0`/CD`0044ee30`은 신전 **생산**이 아니다. Ascend 정리가 count32 초과를64로 바꾸고, 현재 섬의 조건에 맞는 **다른 owner** 자산 중 최대20개를 모아 게임 난수 하나로 선택하여 `vtable+10(flags0x100000)` 삭제를 호출한다. 이 몸체와 후보`004db680`/CD`0044e230`은 별도 `RawVortexAscend` 후속 단위로 복원/검증한다. Handle 원본 관찰에서는 명시 외부 경계다.

`0046f480`/CD`004bdba0`도 지역 해제가 아니다. 존재하는 섬 중 현재 owner와 다른 nonzero owner의 개수를 반환한다. 신전 event3은 반환을 버린다. 같은 섬목록 조회를 사용해야 하며 원본을 빈 콜백으로 대체하지 않는다.

event6700의 `00443fe0`/CD`004d06a0`은 이미 복원된 **`RawConstructionProcess::Start(parent,0.0f)`**다. 프로세스type30 조회는 현재 종속 chain과 실제 `RawContainedLookup`/Form 분배를 사용한다. 삭제 또는 HP 채움으로 바꾸면 안 된다.

event6699의 좌표 효과`00470ac0`/CD`00455b40`은 snap→Dude 사건`00451470`→게임 난수100 소비→`0045a750` 시각 효과 사슬이다. 가상+98의 실제 일반Damageable 피해`0044c8e0`·신전abstract 재지연`004dba40`·사제의 각 재정의를 분배해야 한다. `RawPriestWorldBindings::SetHitPoints`만으로 일반 피해를 모두 대신할 수 없다.

레지던스event2의 `00450a90`/CD`004e8cb0`은 편집 모드/주민 수 전역→Dude 생성/`0x5102` 명령이다. 캠페인 전투 모드에서는 원본 조건으로 자연 반환한다. 미복원 Dude 경로는 명시 경계로 유지한다.

## 독립 근거와 검증 범위

전용 정밀 내보내기는 패치14/CD14개이며 `extracted/assetregular/`에 있다. `tools/decomp_assetregular_oracle.py`는 보호 두 PE를 읽어 Unicorn32비트에서 Handle/Damageable, 현재 raw getter, 공유 난수, **실제 일반 finder/FrameAdvance**를 실행한다. 표시/프레임 지정·owner·Ascend·섬조회·알림/송신·애니메이션·가상피해/건설 시작은 명시 관찰 경계이며 반환/인자/현재 raw 재조회 순서를 기록한다. OS/게임/GUI 진입은 없다.

고유 원본 입력은 패치1054/CD922, 합계 **1976개**다. 각 입력의53/64비트 두 x87 결과/스택·정상 반환 일치를 확인하며 반복 실행은 새 고유 관찰에 더하지 않는다. 전체 raw 슬롯, 반환 float 비트, 난수 상태, 최대 조사 전역, 이웃 프레임과 효과 순서를 커밋 fixture에 저장한다. CD의 범위 밖 프레임에서 원본 배열 밖을 읽는 입력은 정상 관찰에서 제외한다.

`AssetRegularTests.cpp`의 두 전수 재생은 같은 raw pool/hash와 실제 Frame/Finder/Advance에서 native1976개를 비교한다. 실제 Kernel/ProcessHost 검사는 시각17.25에서 event1 payload0.08의 최초 즉시 호출→17.25+0.08f 예약→그전 재호출 억제→두 번째 프레임을 확인한다. 누락 효과/부모·다른 풀 지정기 거부 검사도 포함한다.

**최신 Release의 집중4개 검사와 원본1976개 전수 C++ 대조 및 최종 SHA 감사가 통과했다.** 로그는 `extracted/run4-AssetRegular_-final.log`이며 전체 통합 검사958개도 통과했다. 최초 실패에서 원소+98을 덮던 합성 프레임 입력을 교정했고, CD unordered NaN 비교의 cap0.03 선택을 원본대로 복원했다. 독립 행의 free 슬롯 재초기화는 검사 전용 입력 접근에서만 허용한다. 초기 실패 로그는 그대로 보존했다.

실제 월드 postPop/소유자/삭제/생산/건설/피해·GUI/캠페인 종단 완주는 이번 단위의 완료 근거가 아니다. DirectDraw 전체화면 오류 수정은 이번 목표에서 제외한다.
