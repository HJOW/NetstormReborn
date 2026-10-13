# 초기 지형 프레임과 raw stamp 복원

2026-10-13, 현재 호스트와 마지막 정밀 디컴파일 PC는 **HJOW-Athlon**이다. 최신 AGENTS.md와 두 인수인계를 읽었으며 작업 중 추가된 CD10.72 주소/판본 차이 기록 지침을 반영했다. 원본 보호 파일, AGENTS.md, dotnetpj와 공통 CMake/완료 목록은 이 독립 작업에서 수정하지 않았다.

## 실제 몸체와 판본 차이

| 역할 | 패치10.78 | CD10.72 | 복원 내용 |
| --- | --- | --- | --- |
| 초기 프레임 | `004c04b0` | `0049cf20` | 현재 문자/지역·실제 두 이웃mask→독립99 CRTcache→AA J구간 또는 공유 후보 검색 |
| 초기 stamp 루프 | `004c0640` | `0049d2c0` | Y외부/X내부·반열린 bounds·alias/mapping 필터·현재 특수mapping 재설정 |
| 지면 한 객체 생성 | `004bfe50` | stamp 안 인라인 | factory→owner0→직접frame→Pop→region→새SID fringe |
| 프레임 후보 검색 | `0049aa90` | `004444c0` | 패치 초기 theme0 quartile, CD전체 후보; fringe는 두 판본 전체 후보 |
| 실제 지역 setter | `004acd20` | `004aca80` | 현재 genus 조건을 검사하고 raw `+8` WORD만 기록 |
| 초기 cache 포인터 | `005c8be4` | `00537190` | 첫 요청에서 현재 CRTstate를99회 연속 소비하고 이후 재사용 |
| CRT `_rand` | `004e64ab` | `004f23b0` | 실제 PE의 `holdrand*0x343fd+0x269ec3`, 상위15비트 반환 |

자동 match의 패치004c0640→CD0049d690 대응은 실제 stamp가 아니다. CD의 실제 wrapper0049e4c0→0049d2c0→frame0049cf20 호출을 기계어로 확인했다. 세부 주소/저장소/코드 변수를 잘못 추론한 C 문장의 교정 근거는 `cpp-initial-terrain-stamp-audit-20261013.md`에 있다.

원본 지면과 지역은 **X열 우선(x*256+y)**이다. root 월드의 Y행 우선 마스크는 명시적으로 전치하여 같은 raw 코드/지역 자료에 연결해야 한다. 초기 프레임은 owner theme36을 더하지 않으며, AA는 실제 J구간 첫 프레임을 사용한다. 비AA의 좌표hash 선택을 실제 GameRandom 선택으로 대체했다.

초기cache는 RegionPaint의99cache와 별개다. 첫 프레임 요청은 AA 여부와 관계없이 현재 공유 CRTstate를99회 소비한다. 앞 원소와 mod4가 같으면 현재 원소를1 증가시킨다. cache를 이미 가진 입력에서는 CRTstate를 전진시키지 않는다. 초기화 전의004437f0/CD0048ef00은 실제 빈 몸체이며 seed설정으로 해석하지 않는다.

stamp는 현재 x/X/= mapping을 isle97/edge98/isle97로 다시 지정한다. 지면 factory 뒤 context의 lastSid를 저장하고 owner0, 패치DWORD/CD BYTE 직접frame 대입, Pop flags **0x02000061**을 순서대로 적용한다. 섬genus면 현재 지역을 지정하고 frameflags1에서 fringe102를 별도 factory로 생성한다. 제외flags는 **signed char로 확장한 codeflags**와 비교하며 겹치면 공유 난수/실제 검색을 다시 소비한다. fringe 위치는 원본처럼 y+4이다.

## 공유 C++ 저장소와 하위 경계

`RawTerrainStamp`는 같은 SidPool·현재 types/frames·원본 code/지역/mapping/alias/bounds/context, 공유 GameRandom, 공유 CRT holdrandDWORD와 독립 전역cache를 참조한다. 기존 `RawRegionPaint::Choose`를 다시 구현하지 않고 root가 공유한 **ChooseTerrainFrame(frames,code,theme,random)**을 호출한다. 공유 몸체의 원래 코드/484개 원본 관찰은 `framechoose-lineage.json`의 기준commit **4e532fd**에 연결된다.

`TerrainStampAssertion`의 두 값에 각각 한국어 용도주석을 추가했다. 교정 전 헤더/보고서는 별도로 보존하고 `tools/analyze_terrainstamp_enum_comments.py`와 `terrainstamp-enum-comments.json`에서 **헤더 전체581 C++토큰 불변**, 나머지 원본/소스/fixture 출처 불변을 확인했다. 현재 C++출처 표를 새 헤더SHA에 연결했고 native_source_files는 그대로다. 이 주석 교정의 새 원본 관찰/동작 변경 수는0이다.

첫 C++ 전수대조의 Frame행2에서 원본보다8 작은 프레임을 발견했다. 실제 양판본의8이웃 bit표는 **{1,2,2,4,4,8,8,1}**이며 직선 N/E/S/W는1/2/4/8, 대각선 NW/NE/SE/SW는1/2/4/8이다. C++의 대각선 NE=1 해석을 실제NE=2로 교정했다. native/fixture1560개는 그대로이며 `tools/analyze_terrainstamp_mask_correction.py`와 `terrainstamp-mask-correction.json`에서 교정 전 cpp 전체/이전 출처, 제한된 세 위치의 교정 및 두 보호PE의 실제 bit표를 연결한다. enum 주석 단계의 당시cpp 불변은 그 이후 mask 교정의 `mask_successor`에 이어진다. 초기 실패 로그는 `extracted/run7-TerrainStamp_-first.log`로 보존한다. 이 교정의 새 native 관찰 수는0이다.

factory, 가상owner/Pop, 실제 region setter, 지역127의 현재 ChunkMap 조회와 assertion은 필수 hooks다. 미연결이면 생성 시 거부한다. root의 실제 factory/Pop/PostPop과 `SetSquidRegion`을 같은 pool에 바인딩하는 월드 작업은 별도 통합 단계다. 이 모듈의 hooks를 빈 효과로 채워 월드 복원이 완료됐다고 세지 않는다.

원본 fringe 검색 실패−1은 코드배열 바로 앞BYTE를 signedflags로 읽은 뒤 제외 여부를 검사한다. C++의 `fringePrefixFlags`에 그 raw 입력을 명시하여 안전한 배열 안 검색으로 해당 원본 읽기를 지우지 않는다. 모든 정상 게임 타입 프레임은 실제 코드 배열 안의 물리 프레임이어야 하며 잘못된 생성 프레임은 C++에서 진단한다.

## 독립 원본 근거와 검사 상태

최종 고유 입력은 **1560개(판본마다 Frame600/Stamp180=780)**이며 각 입력은 x87 53/64비트 정밀도에서 두 번 실행했고 **정상 반환3120회/assert0**다. 반복 반환을 새 입력으로 합산하지 않는다. 직선/대각선256mask, 지도0/255경계, CRTstate0/1/−1/7, 첫/기존99cache, nonAA/AA·빈코드·300후보 제한·sparse코드 fallback, 지역 불일치, 빈mapping/alias, 반열린 빈bounds, fringe 제외/재시도와 빈fringe −1을 포함한다.

독립 실행기는 실제 frame/mask/99cache/CRT·게임난수/Choose/stamp/region setter 명령을 실행한다. CRT thread 주소, allocation/free, factory와 가상owner/Pop, region127 ChunkMap 조회/assert는 격리된 경계다. native allocation/free는 별도 외부호출 계수에 남기며 C++ std배열/후보vector의 호스트 allocator 순서를 게임 효과로 비교하지 않는다. 실제 factory/가상효과 인자·호출순서, 생성 raw의type/frame/owner/region/좌표, lastSid, 특수mapping과 두 난수/99cache의 현재 값은 fixture에 보존한다.

자료는 `tools/decomp_terrainstamp_oracle.py`, `tools/ghidra/terrainstamp-functions.json`, `cpppj/tests/fixtures/terrainstamp-x86.tsv`, `cpppj/terrainstamp.json`, `extracted/terrainstamp-oracle-final.log`다. 양판본 smoke12개씩과 최종1560개 native 정상반환/assert0 및 현재SHA/고유입력/두정밀도 반환 감사가 통과했다. 최신 공통 Release는 종료0/경고0이며 **TerrainStamp_3 tests와 양판본1560개 전수 C++ 대조 및 cache재사용/미연결효과 거부가 모두 통과**했다. 전체983개 검사는 예정이며 아직 실행 전이다. source binding 도구 `tools/analyze_terrainstamp_cpp_binding.py`는 native 출처 SHA를 보존하고 현재 C++/공유Choose/실제SquidRegion 출처를 별도 표로 연결했으며 연결SHA 감사가 통과했다. 이 연결 단계의 새 native 관찰 수는0이다.

GUI/OS/실제 게임 창 진입은0이다. 실제 Shape/Pop/PostPop·지형 성장/경계 wrapper·전체 saved 지형 생성·캠페인 플레이/완주를 검증한 보고서는 아니다.

최종 단위07 Release 두 타깃 종료0·경고/오류0, 집중3개 및 **전체983개 검사/실패0**이다. 통합 근거는 `run7.json`, 최종 전체 로그는 `extracted/run7-all-final.log`다.
