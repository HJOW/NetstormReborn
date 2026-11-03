# 연소 갱신의 중복 진단 복귀와 실제 회복 소비자

단위825를 완료했다. 제품은 **825 / 표시11.01**이다. 현재 PC와 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-11-03**이며 이전 완료 커밋은 `51fbfaf8 / 20261104-06 CP`다. 작업 전 AGENTS.md·LEFT_JOBS.md·dotnet 인계를 다시 읽었다. 최종 빌드와 집중 검사8개가 통과했으며 전체 목표는 미완료다.

10.78의 `0044c060`과 CD10.72의 `00462480`은 free bit0 또는 damageEffect0이면 검색 없이0을 반환한다. 현재 HP와 최대HP를 먼저 저장하고, signed32에서 두 배로 감긴 최대HP를4로 나눈 절반값과 비교한다. 패치는 HP DWORD, CD는 signed HP WORD를 사용한다. 첫 finder의 process SID도 지역값이다. 패치만 첫 finder 복귀 뒤 현재 checking 전체DWORD를 읽고, 현재 burningType으로 index1을 검색한다. 비영이면 `findProcess (mySid, burningSystemProcessType, 1 ) == 0`, `\Ns\O\Damageable.cpp`, **0x210(528)줄** 진단을 선택한다. 기존 인계의512줄은 오기였다.

진단이 정상 복귀하면 이미 저장한 HP·절반값·첫 finder 결과로 처리를 계속한다. 이때 TYPE·현재 bulfType·좌표는 원문 위치에서 다시 읽는다. 따라서 진단 장치가 HP를 회복시키거나 첫 검색 입력을 바꿔도 같은 호출의 분기를 바꾸지 않는다. 진단이 free bit를 설정해도 부모 시작 검사를 반복하지 않는다. 저HP·첫 결과0이면 현재 불프의 위치음 `bulfHurt.wav`를 먼저 요청하고 생성자에 진입한다. 생성자 null은 자연 반환0이다. HP가 절반 이상이고 첫 결과가 비영이면 실제 가상삭제(flags0) 뒤0을 반환한다. 진단 중단이나 선택 장치 누락은 앞선 효과를 되돌리지 않는다. 기존 제품의 일괄 생성/소리 사전검사도 위치음 뒤 생성 장치 확인으로 옮겼다.

새 `DamageableBurningHooks.assertion`은 기존 네 필드 뒤에 추가했다. 실제 Session은 별도 진단을 공급하지 않은 경우 같은 `services.ai.assertion`에 연결한다. `burningState.checking`은 이미 같은 ClientGameplayGlobals의 AiDebug DWORD에 Bind돼 있다. 같은 RawCampaignHitBindings가 생성하는 연소 몸체를 다음 경로가 함께 사용한다.

| 현재 소비자 | 같은 연소 부모로 이어지는 경로 |
| --- | --- |
| 사제 피해 | RawCampaignHitBindings의 PriestDamage.damageableRefresh → 같은 burning.Refresh |
| 일반 피해 | MakeDamageableHitBurningHooks → 같은 burning.Refresh |
| 회복/일반 Damageable 가상 갱신 | RawPriestWorldBindings.DamageableRefresh → Session의 regen.damageable → hits.Burning().Refresh |
| 검색/삭제/생성/위치음 | 같은 pool의 RawContainedLookup / World.Destroy / Particles.AddBurning(mode1) / 현재 soundNames와 playOnceAt |

독립 x86 관측기는 원본 PE를 읽기 전용으로 자신의 전용 payload에 복사하고 부모 함수만 실제 CPU에서 호출한다. TYPE/최대HP getter·finder·생성자·가상삭제·위치음·진단 장치만 명시 입력/관측 경계로 바꾼다. 원본 게임 시작/로더/IAT 초기화·창·네트워크는 실행하지 않는다. 명시 thiscall/noinline ABI를 사용한다. **1,100행(판본별550), 진단216행**이며 재관측도 정확한 바이트로 일치했다. signed 극단/감김·49/50/51·최대HP100/101·CD WORD 절삭·최초/중복 검색·생성null/성공·checking0/상위비트/ffffffff·불프/일반·free/damageEffect0·finder/진단 중 현재 전역 변경을 포함한다. fixture SHA-256은 `3a35ce8c2135ac10b224b9ec2e5e7670d09e4299be2f92d1c1edf9832e621927`다. 기존 fixture를 덮지 않았다.

실제 Client/Session 검사는 TheWarBegins 보호 자료를 화면 없이 로드한다. 명시 HP와 실제 두 연소 Form을 사용해 current checking의 중복 진단에서 HP 회복/두 번째 Form 삭제를 요청하고 정상 복귀한다. 첫 연소는 같은 호출에서 유지되고 다음 갱신에서 정리돼야 한다. 기본 생성자와 mode1·stage0·등록/Host/삭제 깊이도 확인하며, 캡처/Kernel/공격 프레임은 진행하지 않는다. 이 검사는 자연 전투/전체 시나리오 동일성의 증거가 아니다.

최종 집중 **8개(새4) 실패0**이며 CPU 비교2·장치/부분효과1·실제Client/Session1·기존 일반 피해4다. 최초 빌드는 새 검사에서 클래스 Client를 ClientMain으로 적어7컴파일 오류/종료1이었다. 첫 집중4개 중1단언은 같은 Client의 Subscriber를 전달하지 않은 검사 조립 오류로 기존 정체성 guard가 거부했다. 검사만 기존 ClientOptions·공유 Subscriber/Player/Kernel 구성에 맞춰 수정했다. 최초/중간/최종 로그를 보존했으며 제품 계산이나 기존 fixture를 실패에 맞춰 바꾸지 않았다. 최초 target Netstorm은 라이브러리로 선택됐고 실제 EXE는 최종 NetstormCpp로 명시 빌드했다. 최종 제품/집중 빌드와 검사 owner 보완 빌드·x86 프로브1회는 종료0·경고/오류0이다. EXE SHA-256은 `f0325e3b732e2d45ed5fd88c5877e0d5945ea52e1427cbdd9ed095ee1f77cc22`다. 최종 결과는 `cpppj/damageable-burning-checking-20261104.json`과 LEFT_JOBS.md에 남겼다. 기존2752개/fixture932개는 변경0이며 새 CPU fixture를 포함해 보호2753개/fixture933개다. checking 잔여는 **21=부모13+부분5+MP3**, 완료부모29다. 다음826은00472f70 마나 목록의 진단 복귀/현재 owner·실제 소비자이며 나머지1~8차 작업도 이어 간다. 실제 공격·사제 포획/희생·승리·사용자 직접 확인·새 MP/공중3종 전용·물리 화면/PCM·전체34미션/71대상/CTest/ZIP 검사는 제외했다. 원시는 Git 제외 `extracted/cpp-resume-825/`다.
