# 실제 AI 본체 생성과 목록 소유·소멸

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 보호 PE와 정밀 프로젝트를 읽기 전용으로 분석했다. 원본/복사본 게임이나 GUI를 실행하지 않았고 `originals`/`originalCD`/AGENTS를 변경하지 않았다. 구현은 PATCH10.78 전용이다.

`RawAiBrainConstructor`는 원본 **`00416860`의 실제0xb08바이트 본체 초기화**, **17개 실제 목록 할당**, **같은 Player/섬/방향별 동맹의 효과**, **`00410100`의 목록 소멸**을 복원한다. `RawAiBrainMemory`가 각 본체/목록의 실제 host 바이트를 단독 소유하고 32비트 핸들로 등록한다. 이 핸들은 실제 소유 저장소를 조회한다. 포인터 없는 dummy brain이나 active 표시용 토큰을 만들지 않는다.

원본 x86 포인터를 host64 포인터로 역참조하지 않는다. 각 핸들의 원본 가상주소와 동일성, allocator 내부/OOM, 생성 중 SEH의 OS 예외 프레임 자체는 대조 범위가 아니다. 같은 실제 저장소/원본 쓰기/수명을 복원하는 범위다. owner별 Player brain 부착과 초기 갱신은 별도 `RawAiCreate`의 원래 순서에 남긴다.

## 본체와 목록의 실제 초기화

생성자는600바이트 설정을 그대로 복사한다. 본체나 목록의 나머지 바이트를 통째로0으로 지우지 않는다. `RawAiBrainMemory::Fill`은 caller의 할당 직후 입력 바이트를 공급하며 이후 원본이 실제 쓰는 위치만 변경한다. 검사는 본체/매 목록에 서로 다른 패턴을 공급해 미초기화 바이트 보존도 확인한다. Game UI의 기본 미초기화 바이트가 이 검사 패턴과 같다고 주장하지 않는다.

| pointer offset | capacity/count | 실제 할당 크기 |
| --- | --- | --- |
| `284` | `288=10`, `28c=0` |40바이트 |
| `2dc/2e8/2f4` | 각+4=500/+8=0 | 각2000바이트 |
| `300` |4/0 |16바이트 |
| `30c/318` |5/0 | 각20바이트 |
| `324` |40/0 |160바이트 |
| `330/33c/348/354/360/36c` | 각500/0 | 각2000바이트 |
| `378` |40/0 |160바이트 |
| `384` |2/0 |8바이트 |
| `ae4` | `adc=10`, `ae0=0`, `ae8=0`, `aec=owner` |120바이트 |

앞16개는 DWORD 배열이다. `ae4`의 원소 stride/후속 의미는 아직 명명하지 않으며 원본 크기120과 주변 필드를 그대로 유지한다. 마지막 배열을 실체 없는 빈 벡터나 pointer0으로 대체하지 않는다.

원본의 -10000.0f 비트 `c61c4000`/0/1/3/6/10과 double0 필드, owner260, 지갑2d8=설정22c, graph298/2ac=현재00540410, ad8=2 등의 쓰기를 복원했다. `6d4`부터255DWORD만0으로 쓴다. 끝 `b04..b07`을 포함해 원본이 쓰지 않은 위치는 caller 입력을 유지한다. 같은 설정이라도 할당 입력이 다르면 그 byte가 보존된다.

능력 mask248의 bit0이 없으면 **실제 할당한36c/378의 pointer는 유지하고 capacity370/37c만0**으로 만든다. 소멸자는 capacity와 무관하게 두 실제 메모리도 해제한다.

## 같은 현재 Player·지역·타입·방향별 동맹

`RawPlayerWorld::Records()`의 같은9×180바이트를 사용하며, 별도의 firstRegion/active/brain 사본을 만들지 않는다. 같은 `RawIslandRuntime`/Pool을 생성 시 검증한다. 참여 모듈과81DWORD 표, 현재localOwner 참조는 상위가 동일한 세계로 공급해야 한다.

1. 현재 본체owner260의 Player+78이 현재 sentinel0050c038(기본127)이면 실제 사제를 찾는다. SID가1 이상/실제 pool capacity 미만인 경우에만 현재 raw 좌표14/18을 읽는다. free/dead/현재타입을 임의로 추가 필터하지 않는다.
2. 실제 지면 지역0046f530을 구하고 섬 존재0046ee30이 비제로일 때 실제 `RawIslandRuntime::Owner(region,currentOwner)`를 호출한다. 그 뒤 다시 현재 본체owner를 읽어 Player+78에 지역DWORD를 쓴다. 외부 효과가 owner/지역을 바꾸어도 필요한 시점에서 다시 읽는다.
3. 본체280을0으로 만든 뒤 TYPE0부터 순서대로 `(flags2&40800)!=0`이고 **TYPE+a0 요구에너지 문자열이 대소문자를 무시한 `ss`**인 첫 타입을 저장한다. 타입 이름/assetName이 `ss`인지로 바꾸지 않는다.0번 타입이 일치하면 실제로0을 저장하고 검색을 끝낸다.
4. 실제 `RawPlayerParticipation::SetAlliance(currentOwner,currentMask23c)`를 실행한다. 이후 현재 `allies[ai][local]`이 비제로일 때만 `AddAlly(local,currentAiOwner)`로 로컬의 반대 방향을 더한다. 전체 표를 대칭화하지 않는다. 두 하위의 실제 Chat/packet/assert 분기는 기존 참여 모듈이 담당한다.

현재 `RiftTypeRecord`는 +a0 요구에너지 문자열을 복원하지 않았으므로 생성자는 이 값을 **필수 `requiredEnergy(type)` 경계**로 받는다. 실제 postprocess0049b0d0는 원소/레벨에 따라 문자열을 보충하고 소문자로 바꾼다. 초기 갱신도 이 문자열을 읽으므로 상위가 같은 실제 metadata를 공급해야 한다. 없는 값을 type.name이나 임의 `ss`로 채우지 않는다.

이 생성자 자체는 Player+b0나 flags20을 쓰지 않는다. 정상 `RawAiCreate`의 호출 순서는 실제 new/생성자→`AttachBrain`→`00414710 initialUpdate`이며, 현재 초기 갱신 전체가 없으면 `RawAiCreate`가 생성 전에 명시 오류를 낸다. 생성자 복원만으로 Outcome/AI 전략 자동 활성화를 켜지 않았다.

## 소멸과 실제 수명 연결

`Destruct(token)`은 `00410100`처럼 ae4가 비제로이면 먼저 해제하고,384/378/36c/360/354/348/33c/330/324/318/30c/300/2f4/2e8/2dc/284 순으로 현재 pointer 필드를 해제한다. 원본처럼 본체의 pointer DWORD를0으로 덮지 않고 본체도 아직 해제하지 않는다. 이후 `FreeBody(token)`이 상위 `00413ae0`의 CRTfree에 해당하는 본체 해제를 수행한다.

기존 참여 hook은 `destructBrain=Destruct`, `freeBrain=FreeBody`에 연결할 수 있다. 같은 brain 토큰은 실제 목록 소멸/본체 free가 끝날 때까지 Player에 남고 그 뒤 기존 `RemoveBrain`이+b0=0을 쓴다. 원본처럼 flags20은 해제 뒤에도 남는다. 아직 살아 있는 brain은 등록부의 같은 본체/목록을 읽고 수정해야 하며, 후속 전략의 reallocate도 같은 소유 등록부를 갱신해야 한다.

선택된 실제 지면/metadata/Chat 등이 누락되어 생성 중 예외가 나면 구성한 목록만 역순 해제하고, 상위 Construct가 본체도 해제한다. 이미 실행한 세계 효과를 합성 rollback하지 않는다. `RawAiBrainMemory`의 host 컨테이너 파괴가 자동으로 backing 메모리를 정리한다는 사실은 실제 `Destruct/FreeBody`의 게임 수명 처리를 대신하지 않는다. 상위 세션은 참여자를 제거한 뒤 이 등록부/빌린 세계를 해제해야 한다.

## 원본 대조와 검사

[선택 export2함수](../../tools/ghidra/ai-brain-functions.json), [격리 관찰 도구](../../tools/decomp_ai_brain.py), [SHA 보고서](../../cpppj/ai-brain.json), [fixture](../../cpppj/tests/fixtures/ai-brain-x86.tsv)를 추가했다. 같은 입력 재실행을 중복 합산하지 않는 **160고유입력**에서 실제 생성자/소멸자 및 기존 SetAlliance/AddAlly/owner getter 기계어를 실행한다. 모든 함수는 정상 반환했다.

본체전체0xb08, Player9전체1620바이트, 방향별81DWORD전체324바이트, 섬기록전체3204바이트, 미리 확보한 본체+실제17목록의 크기/전체바이트Adler, 실제 역순17free 및 사제/좌표/에너지 조회 순서를 대조한다. 본체는 소멸 뒤에도 그대로 살아 있으며 전체 raw가 변하지 않은 사실과 별도 본체free를 검사한다. owner/local1·8, sentinel/지정지역, 능력bit0, 선언동맹/단방향표, 메타데이터 첫일치/미일치/0번 일치, SID0/경계32767/32768/65535와 없는지역을 포함한다.

힙/사제/지면/섬owner/TYPE에너지 반환은 명시 외부 경계다. C++ 쪽은 실제 `RawIslandRuntime::Owner`와 현재 Player 기록을 사용하고 원본 쪽 가상 외부 응답과 전체 변경 원문을 비교한다. 검사가 섬owner 효과 trace를 제품 호출처럼 합성하지 않는다. pointer는 명시 allocator가 응답한 같은 opaque32 핸들로 비교하며 원본 실제 주소를 주장하지 않는다. 최초 격리에서 지면0046f530을 cdecl로 잘못 정리해 엄격 guard가 SEHhandler004f91ee 진입을 중단했다. 원본 assembly의 **ret8**을 확인해 수정한 뒤160개 전체 정상 반환/SHA 감사가 통과했다. 검토하지 않은 handler나 OS 코드를 실행하지 않았다.

`AiBrainTests.cpp`의 집중2개는 원본160입력 재생과 실제 소유자→참여 부착/소멸/free, 초기update 누락의 생성 전 중단, 요구에너지 누락의18블록 정리/참여 미부착을 검사한다. root가 **통합 게임/검사 Release 빌드 경고0·오류0, 집중2개 모두 통과**를 확인했다(`extracted/run47-50-build.log`, `extracted/ai-brain-root-focused.log`). 이후 production 변경 없이 최종160입력 고유성/SHA/UTF-8/새파일 포함 행끝공백 감사도 통과했다. 기존전체1183개 검사와 구분하는 새집중 결과이며 미복원 초기update/전략까지 검증한 결과로 합산하지 않는다. Session/GameWorld/CMake/LEFT/커밋은 이 하위 agent가 변경하지 않았다.

**남은 실제 큰 몸체**는 초기 갱신00414710의 같은 RawPlayerTechnology/TYPE분류·가격/빌더 목록, 전략00417d30/00417de0 및00414af0/00416eb0/004139f0/00417c70, 피해·게임사건이 호출하는 AI handler다. 실제 brain 메모리가 생겼다고 이 경계에 빈 성공 함수를 공급하지 않는다. 실제 시작/Outcome 자동 호출은 필요한 몸체/수명/자료 공급이 갖추어진 뒤 연결한다.
