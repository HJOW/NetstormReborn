# 신전 의식 상위 몸체와 공통 HP 공유 — 2026-10-13

현재/마지막 디컴파일 PC는 **HJOW-Athlon, 2026-10-13**이다. 최신 같은 정밀 프로젝트의 `templeritual` readOnly 내보내기는 **10.78 패치43 / CD10.72 36 함수**, 종료0이다. 보호 원본/AGENTS.md와 앞07 완료 보고서를 보존한다.

## 원본 순서와 실제 재사용

| 역할 | originals 10.78 | originalCD 10.72 | 구현/연결 |
| --- | --- | --- | --- |
| 의식 상위 전체 | `00447e40` | `00481fc0` | `RawTempleRitual::Ritual` |
| 의식 단계 | `00447890`, 상위 인라인 | `004812a0` | 현재 count별 첫 프레임을 빼서 단계0..5 |
| 실제 dais 가상 count | `004478f0` | `00481310` | 기존 `RawPathAnimation::Direction` 재사용 |
| 단계 Anchor/사제 목표점 | `00447aa0/00447b30` | `00481c80/00481d10` | 실제 SHP signed short 핫스폿 |
| 목표점의 Rune | `00447c00` | `00481de0` | 현재 단계 핫스폿을 빼고 기존 일반 finder |
| 강한 같은 owner 사제 | `004479c0` | `004817e0` | 기존 실제 Footprint·일반 finder·MaxHitPoints |
| 추상 Rune 존재 | `00447dc0` | `00481ee0` | 현재 발자국과 일반 finder |
| Rune 생성 상위 | `00447ca0` | `00481e50` | Factory→공통HP→현재frame→abstract→owner→다시Anchor→Pop |
| 공통 HP 직접쓰기 | `004adca0` DWORD | `004aed20` WORD | 공유 `SetRawSquidHitPoints` |
| Rune 파생 postPop | `00448930` | `00484520` | 별도 필수 소비자 경계 |
| 신전 Handle | `0044b150` | `00482c40` | 0x307 후속에서 의식mode1; 전체 등록은 후속 |

현재 신전 프레임은 패치 DWORD/CD BYTE다. dais count는 **현재 방향N→1/M→2/L→3 우선**, 그 밖에는 프레임64/120 경계의1/2/3이다. raw state 상위나 hash level로 대신하지 않는다. count1..3 첫 프레임은 **3/64/120**, 단계 오프셋은 **0/11/22/33/44**다. 현재 프레임에서 count별 첫 프레임을 뺀 결과가11/22/33/44/55 경계를 통과하면 단계0..5가 된다. count0은 이 가상 몸체가 반환하지 않는다.

Anchor는 현재 신전 위치에 **첫 count1의 프레임3+단계오프셋 SHP 핫스폿**을 더한다. 사제 목표점은 이어서 현재 Rune 타입/단계의 핫스폿을 더한다. 단계4만 **좌표를 그냥 절삭한 칸**에서 spot&6/!8일 때(-9,-6)픽셀을 더한다. 일반 사각형 finder의 우하단 거의올림과 구분한다. 픽셀 배율은 패치0059a92c/930, CD00565c44/48의 현재 float이며 정상 입력16/11을 사용한다.

강한 소유 사제 조회는 실제 신전 발자국을 **±1** 넓힌다. PE의 실제 float **005012d0/CD00504034=1.0**을 확인했다. 최초 정적 조사에서 추정했던±2는 원본 상수 확인 후 교정했다. 같은 현재 타입158/owner와 **signed 현재HP>=현재MaxHitPoints/2**가 조건이다. 사제 자체의 free/dead/void/abstract는 새 제외 조건으로 넣지 않으며 generic finder는 buried 후보를 제외한다.

Ritual은 현재 단계0..4에서 사제 위치가 현재 목표점 또는(단계0..3이면)다음 목표점과 정확히 같고 현재 Rune이 없는 경우만 진행한다. mode0은 기존 `RawPriestRitualActions::Begin`→현재 사제 위치음향→new0x28→신전event0x307 순서다. 단계별 음향은 `forWind2/forSun2/forRain2/forThunder2/forStorm2.wav`, payload는 **1.96/2.25/1.88/2.22/2.41의 원본 float 비트**다. 나머지 mode는 현재 강한 같은 owner 사제에게 기존 Continue/604를 요청한 다음 현재 boss/추상 Rune 존재를 다시 읽어 Rune을 생성한다.

SpawnRune은 생성 후 HP0을 쓰고 현재 신전 단계를 다시 읽어 Rune 프레임을 쓴다. void가 아니면 원본 assert에 대응해 실패한다. extra1을 켜고 현재 신전 owner를 요청한 뒤 단계를 다시 읽어 현재 Anchor로 Pop한다. 하위 효과가 상태를 바꿔도 최초 단계값을 재사용하지 않는다.

## 공통 HP 몸체 이동의 출처

기준은 앞07 **b76adf9**, 공유 Release 경고0/전체983개 실패0이다. `RawConstructionProcess::SetHitPoints`의 기존 private 본문을 새 **RawSquidHitPoints.h/.cpp**의 `SetRawSquidHitPoints(SidPool&,Sid,int32_t)`로 옮겼다. private 함수는 같은 API를 호출한다. 원본의+26 DWORD/WORD 저장만 수행하며 가상 사제 HP 전환/notify/표시/회복 효과를 추가하지 않는다. `RawFortObjectLoad`는 그대로 보존한다.

`tools/analyze_squidhitpoints_lineage.py`는 새include와 위임을 되돌리면 기존 전체 .cpp의 문자/토큰이b76adf9와 같고 기존 .h가 그대로이며, 이동한 본문은 pool_→pool 매개변수 이름만 다름을 증명한다. 앞 건설 **2868개 원본 관찰 보고서/fixture/PE/export/tool**와07 통합근거를 수정하지 않고 현재 API SHA를 별도 보고서에 연결한다. 몸체 이동의 새 원본 입력은0개다.

## 대조 준비와 정확한 범위

`tools/decomp_templeritual_oracle.py`는 상위 의식과 count/단계/SHP/좌표/사제·룬 일반finder/공통HP setter의 실제 PE 몸체만 실행한다. 프레임 setter/new/Shared/Create/음향/가상owner/Pop는 이름/인자를 기록하는 명시 경계다. 기존 pathcleanup의 같은 일반finder/helper 내보내기는 읽기만 재사용한다.

전체 **정상6720개 고유 입력/assert0개**가 두 x87 정밀도에서 같은 결과로 반환했고 현재SourceSHA 감사도 성공했다. 각 고유 입력의 조회출력7종은 같은 관찰의 일부이며 별도 신규 합산하지 않는다. 사제와 Rune의 실제 입력 좌표 비트는 fixture 입력 칸에 직접 저장해 C++ 준비가 기대 조회결과를 사용하지 않게 한다. C++ 공유 빌드 완료 후 최종 상태를 기록한다.

`TempleRitualTests.cpp`의 전수 검사는 실제 조회/의식 요청 순서, raw5/6/7/8의 vtable 주입 DWORD를 제외한 모든 바이트, callback 뒤 현재 권한을 비교한다. 실제 mode0 소비자 검사는 기존 Factory/Owner/공통HP/SquidFrame와 같은 Host/Kernel의0x307 payload/부모/시각을 연결한다. 이 검사는 Rune Pop을 실행하지 않는다.

08 첫 공유 대조는 실제 mode0 소비자에 성공했고 전수 대조에서1330개 check가 실패했다. 진단 결과 기존 검사기의 네 해시 배열(`0x11300000~0x1132a200`)과 상속 지면 주소(`0x11320000`)가 겹쳐 `setup`이 지면 입력을 지웠음을 확인했다. 새 Temple 전용 지면 주소`0x11600000`을 원본의 현재 지면 포인터에 연결했다. production/test는 전체 문자/토큰이 그대로다. 같은 입력 식별자6720개를 재관찰하며 실제 목표점 좌표도 fixture 입력으로 다시 보존한다. 수정 전 도구/fixture/보고서/소스는 `cpppj/hist/temple-rune-afcc3`에 원경로로 보존한다. 이전6720개를 현재6720개에 더하지 않는다.

별도 `tools/analyze_temple_rune_runtime8_lineage.py`와 `cpppj/temple-rune-runtime8-lineage.json`은 위 메모리 연결만 되돌리면 도구 전체가 이전과 같고, 기존 Temple production/test와 Rune의 명시 교정 외 본문·검사·공통 근거가 보존됨을 증명한다. 임시 진단 출력은 최종 소스에서 제거했다. 재관찰/새 집중 C++ 결과는 root가 통합한다.

교정 후 **6720개 정상/assert0개 전량 재관찰과 현재 SHA 감사에 성공**했다.972개 행의 입력 좌표나 출력이 지면 교정에 따라 바뀌었으며 같은6720개 식별자를 유지하고 추가 고유 입력은0개다. 공유 Release빌드71712 종료0 후 **TempleRitual의2검사,6720개 전수+실제 Frame/Host307 소비자 모두 실패0**이다. 집중 로그는 `extracted/run8-rawTempleRitual_final.log`이다. 전체 캠페인의 의식 완주 성공을 뜻하지 않는다.

## 남은 필수 흐름

실제 Rune Pop의+20은304 Regular, flags2이면 반경1의 같은 owner dais 조회 및 건설 시작, 공통PostPopBase를 포함한다. 별도 `RawRunePostPop`와 SquidPostPop의 명시 Rune 등록 및 실제 Pop→Construction→완료 재등록→304 검사 준비까지 이어 작성했다. [Rune 후처리 기록](cpp-rune-postpop-20261013.md)의 C++ 공유 대조를 마치기 전에는 관찰 Pop 요청만으로 실제 Rune 생성 완료를 세지 않는다.

신전 Handle0x305/306/307, 룬304 사건의 실제 실행, 사제 명령/채굴/운반/도착의 실제 GameWorld 연결과 캠페인 UI·완주 동일성은 계속 후속이다. Rune 건설시간은 altar 원래 ownerWORD가 가리키는 현재 playerFlags bit1에 따라7/25(그 밖은10)이며 패치는BYTE/CD는DWORD를 검사하고 별도 전역 조건을 읽는다. 실제 코드에 없는 playerFlags의 의미를 추가 추정하지 않는다.
