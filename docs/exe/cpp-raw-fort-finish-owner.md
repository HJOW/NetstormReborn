# cpppj Fort 로딩 후 소유 레지던스 확인·반복 배치

2026-10-13, **현재/마지막 정밀 디컴파일 PC VM-W11-CODEX**. 새 `RawFortObjectLoad`가 남긴 필수 `finishOwner`를 실제 `0040e1a0`/CD `0042ca70`의 자연 반환·목록·지역·반복 생성 몸체로 복원했다. AGENTS.md와 두 최신 LEFT_JOBS를 확인했고 완료904/5318 및 새 Fort1218 근거를 보존했다. 고유 source/header/tests/manifest/oracle/fixture/docs만 만들었으며 공유 CMake/LEFT/GameWorld/RawSurfaceWorld/기존 증거는 root 소유다. 원본/복사본 게임·GUI·보호 파일 수정·커밋/푸시는 없다.

## 실제 entry와 자연 반환

현재 chaff/single 모드 또는 special이 켜져 있으면 Player/목록을 읽기 전에 끝난다. 그 밖에는 해당 Player의 기본 지역(+0x78/CD+0x70)을 읽고 Fort 모드면0으로 바꾼다. 두 PE의 실제 invalidRegion 초기값은 **127**이며 이 값이면 끝난다. owner1 같은 고정값이나 임의 유효 지역으로 바꾸지 않는다.

검색 대상은 `RawAssetPostPop`의 **residences 목록**(`0059a9d8/count0059a9e0`, CD`00565ae0/count00565ae8`) 자체다. providers/factories/additional 목록 사본으로 대신하지 않는다. 활성 count를 signed로 비교하여 전체 목록 끝까지 현재 raw 타입/genus/owner BYTE를 읽는다. genus`0x40000000`이고 요청 owner와 같은 항목이 있으면 반환한다. free/dead/void/abstract를 새 필터로 더하지 않으며 중복도 유지한다.

1-1에서 실제 Residence postPop이 소유 레지던스를 이 목록에 등록하면 이 정상 분기에서 반환한다. 목록이 없는데도 성공을 반환하도록 만들지 않았다. 해당 조건의 실제 월드 연결은 root의 같은 pool/postPop/목록 등록 결과로 검사해야 한다.

## 없을 때의 실제 반복과 local 난수

선택한 region은 처음 정한 snapshot이며 반복 중 모드/Player 전역이 바뀌어도 다시 정하지 않는다. 매 생성 묶음마다 실제 지역 사각형(`0046f130`/CD`004bd8a0`)→현재 호출 시각(`00460d70`/CD`004011b0`)→현재 생성 타입을 읽는다. 실제 생성 타입 전역 `005411bc`/CD`0051caa8`의 초기값은 **89 residence**다.

실제 seed 생성자 `004bcab0`/CD`004a5a10`은 호출한 double 시각에 **1013.0**을 곱해 `__ftol`의 하위 signed DWORD를 얻고 **SAR4 + DWORD33배**로 seed를 만든다. 호출 인자1은 계산에 사용하지 않는다. 이 seed는 **지역 `GameRandom` 객체**이며 글로벌 게임 난수 상태를 덮지 않는다. 이후 PRNG는 같은0 초기값/`0x0bad0bad`/`state*0x10003+3`/상위16비트 modulo를 사용한다. limit<1은0을 반환하고 난수를 소비하지 않는다.

`PlaceRandom`은 실제 `0040ded0`/CD`0042c7a0`의 AddRandom 몸체다. 사각형 폭/높이 차를 편향 없이 CRT 절삭하고 **높이(y) 다음 폭(x)** 순서로 local 난수를 소비한다. top/left에 더해 float로 저장한 좌표와 `(local PRNG,type,x,y,region,0,force)`를 실제 단일 배치 경계로 전달한다. 성공의0 아님 반환에서만 quantity DWORD를 감소시키며 각 묶음은 **최대1000번**이다. quantity0도 첫 시도를 실행하고, 첫 시도가 성공하면 감소 감김으로1000회까지 계속할 수 있다. 반환은 마지막 배치 결과다. 타입0/-1은 원본 진단을 요청하고0을 반환한다.

각 AddRandom 실패1000회 뒤에도 owner entry는 끝나지 않고 현재 레지던스 목록→새 지역 경계/호출 시각/현재 타입으로 다음 묶음을 실행한다. 실제 성공한 생성/Pop/postPop이 **같은 residences 목록**에 등록되면 다음 전체 목록 순회에서 반환한다. 가짜 타이머·무조건 성공·제한 횟수 뒤 정상 종료를 추가하지 않았다. by-value 사각형을 C++에서도 복사해 외부 callback이 원래 사각형을 바꿔도 현재 묶음의 snapshot을 유지한다.

## 실제 경계와 아직 남은 몸체

모듈의 regionBounds/wallSeconds/place/invalidType는 모두 필수다. `MakeFortFinishOwnerHooks`는 같은 pool의 `Finish(owner)`를 Fort 로더의 owner 후처리에 연결한다. **단일 placement `0040db90`/CD`0042c3d0`의 전체 지형/소유 지역/shape·생성자/HP/프레임/Pop/postPop은 이번 독립 몸체의 명시 외부 경계**다. 실제 region bounds/tree와 현재 clock도 연결해야 한다. 해당 효과를 noop로 채우거나 이 독립 관찰만으로 실제 레지던스 생성/미션 완주가 끝났다고 처리하지 않는다.

1-1의 정상 residence 존재 분기는 하위 생성 없이 실제로 끝날 수 있다. 다른 맵/빈 지역/편집 조건에서 생성 분기가 도달하면 미복원 placement를 명시 거부하거나 실제 몸체로 이어야 한다. 이는 범위 보고이며 별도 GUI/원본 실행 허가나 완료 흐름을 대신하는 조건이 아니다.

## 독립 관찰

고유 Ghidra `fortfinishowner` **원본7/CD9** 함수 읽기 전용 내보내기 성공, `extracted/fortfinishowner-export.log` 보존. `decomp_fortfinishowner_oracle.py`는 실제 entry·전체 residence 순회·시각 seed·AddRandom·local PRNG·CD rectpoint·타입 getter/CRT를 격리 x86으로 정상 반환까지 실행했다. 지역/시각/단일 배치/진단만 명시 입력 경계다.

두 PE **719개씩·신규1,438개** 입력은 두 FPU 정밀도에서 관찰이 같다. mode/Fort/invalid region·owner0/1/2/8·틀린 owner/genus·중복/후행/free·dead/void 목록, 여러 사각형/폭0·음수, 후처리 중 현재 생성 타입 변경/local seed 변경, 성공2/5/**1002번째**(1000 실패 뒤 새 묶음), quantity0/1/3·실패1000/성공 개수, 타입0/-1, 양수/0/음수 호출 시각 seed를 포함한다. 반복 실행·두 정밀도·하위 함수 호출 수는 신규 수에 더하지 않는다. finite 입력 범위의 대조이며 모든 극단 시각/FPU 연산 차이를 증명하지 않는다.

자료: `cpppj/fortfinishowner.json`, `cpppj/tests/fixtures/fortfinishowner-x86.tsv`, `extracted/fortfinishowner-native-{initial,final}.log`, `extracted/fortfinishowner-verify-final.log`. 현재 source/PE/export/fixture SHA·입력·세 실제 진입 감사 **1,438개 통과**다. 새 C++ 집중 검사4개는 두 PE 기록 재생, 같은 residence 월드의 자연 반환·loader adapter, by-value 사각형/실제 효과·손상 목록 거부를 확인했다. **공유 Release 최종 빌드 경고0/오류0·집중4개 실패0**을 root가 확인했다. 기존 Fort 로더 집중4개도 회귀 통과했고 두 새 단위 source/fixture는 고정했다. 전체 회귀 결과는 상위 공유 작업의 최종 기록을 따른다.
