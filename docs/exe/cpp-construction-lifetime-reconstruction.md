# 건설 표시 등록·삭제 수명 복원

2026-10-11, **HJOW-Athlon**. AGENTS.md와 두 LEFT_JOBS 문서를 읽고 현재 PC와 마지막 디컴파일 PC가 같음을 확인했다. `constructionlifetime` 목록 **7/13/13개**를 세 Ghidra 프로젝트에서 읽기 전용으로 내보냈다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 원본 게임·복사본·analyzeManager·클론 창·장치는 실행하지 않았다.

`RawRenderRegistry`는 건설 표시의 상위 RenderProcess 등록/제거를, `DestroyConstructionVisual`은 소유 버퍼와 공통/파생 삭제의 순서를 복원한다. `MakeConstructionRegistryHooks`로 기존 표시 생성의 부착 경계를 실제 등록 몸체에 연결한다. Create→Run/Draw→abstract 해제의 Kill 요청→버퍼/등록 해제와 같은 슬롯 재사용도 연결 검사했다. **BaseProcess의 실제 Kernel/form 부착·클라이언트 자동 실행/Draw 목록·실제 SHP 캡처/팔레트 생성/화면 출력·건설 GUI는 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.**

## 원본 몸체

| 역할 | 10.78 패치 | CD / 추가 판본 |
|---|---|---|
| RenderProcess 생성·목록 등록 | `00487fd0` | `0045c6c0` |
| RenderProcess 소멸·등록 제거 | `00487880` | `0045c840` |
| BaseProcess 소멸 | `0041bd00` | `0048f570` (`0045c8f7` 경유) |
| 건설 표시 공통 소멸 | `00422d00` | `004ea360` |
| 공통 scalar deleting destructor | `00423540` | `004ead20` |
| 즉시 건설의 파생 삭제 | `00423820`→`00423810` | `004ead40`→`004ead99` |
| 일반 건설의 파생 삭제 | 위와 같은 몸체 | `004eadb0`→`004eae09` |
| CD 입자 저장소 소멸 / 상위 소멸 helper | 패치에 인라인 | `004ea3d0`→`004ead00` / `004ea400` |

패치의 즉시/일반 건설 vtable `00503680`/`005036ac`는 같은 삭제 함수를 가리킨다. CD `00507410`/`00507440`는 별도 wrapper를 가리키며 두 wrapper 모두 공통 소멸자를 호출한다. Ghidra가 CD의 작은 helper를 독립 함수로 나눈 것까지 실행 허용 범위에 포함했다. base 소멸자를 응답 경계로 바꾸지 않았다.

외부 응답 경계는 BaseProcess 부착 `0041bd20`/CD `0048f590`, `new`·`free`·확보 뒤 검사뿐이다. 부착은 원본 인자 **타입, 부모, 지정 form 0, flags 0x10**과 this를 확인한다. 실제 등록/확장/복사/번호·깊이 읽기/목록 제거/버퍼 분기/base 필드 쓰기는 원본 명령을 실행한다.

## 등록과 슬롯 재사용

등록 생성자는 form/parent와 표시 사각형 네 float를 0으로 만든다. 기존 목록에서 **0번부터 첫 빈 칸**을 찾고, 없으면 **40칸을 더 확보**한다. 새 배열은 먼저 0으로 채운 뒤 기존 항목을 같은 번호에 복사하고, 기존 배열이 있으면 반납한다. C++는 64비트 포인터 대신 0이 아닌 DWORD 토큰을 저장하지만 확보 경계에 전달하는 바이트는 원본대로 **새 칸 수×4**다.

목록 칸을 먼저 채우고 객체에 slot 번호를 쓴다. 최초 zOrder는 raw **SID 0**의 signed BYTE다. 진입 부모가 0이 아니면 BaseProcess 부착을 호출한 뒤 **진입 때 받은 부모의 현재 BYTE**로 깊이를 다시 정한다. 부착이 객체의 현재 parent를 다른 번호로 바꿔도 깊이는 진입 부모로 조회한다. 경계에서 깊이를 바꾸는 경우에도 현재 값을 읽는다. 패치 깊이는 raw +35, CD는 +33이다.

등록 제거는 slot이 범위 안이고 그 칸의 토큰이 현재 객체와 같아야 한다. 해당 칸만 0으로 만들고 목록을 압축하거나 줄이지 않는다. 뒤의 BaseProcess 소멸자는 form/parent DWORD에 **-12345 (`0xffffcfc7`)**를 쓴다. slot·zOrder·표시 사각형은 그대로다. 삭제 뒤 다음 생성이 같은 첫 빈 칸을 재사용하는 연속 상태도 관찰했다.

원본 배열의 포인터를 실제 호스트 주소로 취급하지 않는다. 잘못된 슬롯/중복 삭제/0·중복 토큰/살아 있는 필드 재사용·범위 밖 부모는 호스트에서 거부한다. 원본의 확보 실패 후 null 접근과 signed 크기 감김도 실행하지 않는다. 확보 실패 때 기존 목록은 보존하지만 생성 중 필드는 초기화될 수 있다. 이 오류 경로를 원본과 같은 동작이라고 판정하지 않는다.

## 버퍼 소멸과 생성 연결

공통 소멸자는 null이 아닌 **캡처(source)→출력(output)→입자(particles)**를 차례로 반납하고 상위 표시 등록을 제거한다. 파생 삭제 wrapper는 뒤에 **flags 하위 BYTE의 bit 0**이 켜져 있을 때 객체 자체를 반납한다. flags 2·256·상위 bit만으로는 객체를 반납하지 않는다. 공유 입자 팔레트는 소멸자의 소유 버퍼가 아니므로 반납하지 않는다.

원본 free는 버퍼 포인터/입자 카운터를 지우지 않는다. C++는 같은 반납 순서 뒤 vector의 저장 용량까지 해제하며, 해제된 원본 포인터의 바이트를 호스트에 남겨 두지는 않는다. release 콜백은 반납 순서를 관찰하는 경계이며 버퍼는 함수가 소유권을 소비한다. 콜백은 예외를 던지거나 span을 보관하지 않아야 한다. 등록이 손상된 소멸 경로는 정상 계약 밖이며 원본의 버퍼 반납 뒤 assert를 실행하지 않는다.

`MakeConstructionRegistryHooks`는 `RawConstructionVisual::Create`의 상위 부착 경계를 목록 등록으로 교체한다. 나머지 캡처/팔레트/확보 경계는 호출자가 공급한다. registry/등록 필드/같은 부모 raw 자료는 hooks보다 오래 살아야 한다. registry가 참조하는 풀과 표시 생성/프레임의 풀은 같아야 한다.

새 연결 검사는 **두 판본×두 생성 방식**에서 실제 Create→첫 Run→Draw→abstract 해제→Kill 요청에 `DestroyConstructionVisual`을 연결했다. 캡처/출력/입자 반납 시점에는 등록이 남아 있고, 객체 반납 시점에는 등록이 사라졌는지도 확인한다. 세 버퍼의 저장 용량은 0, 공유 팔레트는 보존, 다음 생성의 slot은 다시 0이다. 검사에서 Kill 요청을 수명 함수에 직접 연결했으며 실제 SquidProcessHost/Kernel이나 클라이언트 자동 실행에 부착한 것은 아니다.

## 독립 근거와 검증

`tools/decomp_constructionlifetime_oracle.py`가 세 PE의 원본 몸체를 두 x87 정밀도로 실행했다. 판본당 **459개**(Construct 138·Render 64·Build 64·DeleteBase/Instant/Normal 각 64·Sequence 1), 새 **1,377개**·누적 **498,660개**다. 연속 재사용의 중간 루트까지 정상 반환은 **2,766회**다. CTest는 저장 관찰만 읽으며 PE/Python/게임을 실행하지 않는다.

관찰은 빈 목록/1·39·40·80칸, 가득 찬 배열과 첫/중간/마지막 빈 칸, 부모 0/50, 깊이 0·127·128·255, 부착 중 현재 부모/깊이 변경, 세 버퍼의 8가지 null 조합, flags 0·1·2·3·256·257·상위 bit·모든 bit, 연속 등록→삭제→재등록을 포함한다. 전체 슬롯, 공통 DWORD/float 필드, 중간 상태, 확보/반납/부착 인자를 비교한다. 기대 결과를 C++와 같은 계산으로 재생성하지 않는다.

ABI·보존 레지스터·x87·SEH, 실제 함수 범위/쓰기 허용 목록, PE/도구/내보내기/fixture SHA, 입력 순서·폭·루트 진입/정상 반환·경계 사건 집계를 감사했다. 원본 assert·게임·OS 호출은 0이다. 원본 포인터 alias·손상 목록·비정상 크기·확보/반납 중 재진입은 판정 범위 밖이다.

새 C++ 검사는 **6개**다. Release 경고/오류 **0**, CTest 내부 **680개·실패 0**(164.59초, 전체 164.64초), 저장 근거 감사 **105종 모두 통과**. 별도 사본의 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과): 40칸 확장·첫 빈 칸 재사용·signed 깊이·부착 뒤 진입 부모 재조회·base -12345 표식·버퍼 반납 순서·scalar bit 0·부착 flags. 사본 소스는 원래 바이트로 복구했다. UTF-8/BOM·보호 경로와 변경 목록을 확인했다. 로그: `extracted/constructionlifetime-{build,oracle,ctest,audits,mutations}.log`.

관련 파일: `cpppj/src/o/RawConstructionLifetime.{h,cpp}`, `cpppj/tests/ConstructionLifetimeTests.cpp`, `cpppj/tests/fixtures/constructionlifetime-x86.tsv`, `cpppj/recovery-constructionlifetime-evidence.json`, `tools/decomp_constructionlifetime_oracle.py`, `tools/ghidra/constructionlifetime-functions.json`.

재검사: `python -X utf8 tools/decomp_constructionlifetime_oracle.py --verify`, `cpppj/build/bin/Release/netstorm_tests.exe --filter ConstructionLifetime_`, `ctest --test-dir cpppj/build -C Release --output-on-failure`. 내보내기: `powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name constructionlifetime`.

다음은 **표시 프로세스의 실제 Kernel/form 부착·클라이언트 Run/Draw 목록·종속 삭제와 소멸 연결**, 실제 SHP 캡처/팔레트 생성/화면 출력이다. 타입 준비 소리 자료, 부모 공간·소유 사제 contained/경로, 집합/지연 패킷 분배와 건설 GUI도 남아 있다. AGENTS.md·보호 원본·dotnetpj 변경과 커밋/푸시는 없다.
