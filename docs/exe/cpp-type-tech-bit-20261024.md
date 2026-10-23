# TYPE 기술 bit 필드 복원 제안

호스트와 마지막 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 같다. 보호 원본·기존 Native 자료·공유 제품/CMake/LEFT/git는 변경하지 않았다. 사본 기준의 두 파일은 **d207940c(단위386)**과 같고, root의387 Deck/Cursor 적용과 경로가 겹치지 않는다.

원문 **0049ebb0**는 각500BYTE TYPE을0으로 만든 뒤 `base+0x9c` 기준 `+0x54 DWORD`에 `ffffffff`를 쓴다. 이는 **TYPE+1ec의 techBitIndex 초기−1**이다. 원문 **0049c3b0**의 `techBit` 속성은 대소문자 무시 비교→숫자 AST 확인→**float32 load→실제 CRT004e49c0 _ftol→0049e450 [TYPE+1ec] DWORD 대입** 순서다. 여러 속성은 앞에서 뒤로 적용한다. 타입 번호에서 bit를 계산하거나 전체0/−1을 결과로 공급하는 경로는 없다. CD의 해당 필드는+1cc이며 정밀 C의 실제 초기−1/속성 대입을 별도로 확인했다. 이번 추가 Native는10.78만 실행했다.

최초 제품 사본 전에 다음 두 독립 자료를 만들고 `--verify` 모두0으로 고정했다. 기존 [ArcherTypeOracle](../../tools/decomp_archertype_oracle.py)의 실제 전체 초기화 접두와 callback/CRT/후처리를 재사용했다.

- [합성27입력 Native](../../extracted/cpp-type-tech-bit-20261024-native.json): 정밀도027f/037f, **54초기접두+108정상 callback/후처리**, 2,255,550명령. 188초기DWORD, 속성별 실제 store순서, 선택필드와500BYTE 증거를 보존한다. 음수·소수·0·31/32/255/256·중복·대소문자·32/64비트 절삭 경계를 포함한다.
- [보호 자산 필드 Native](../../extracted/cpp-type-tech-bit-assets-20261024-native.json): 실제 PE의116이름 순서, 현재 보호본문의 **23 techBit 속성**, 같은 현재 TYPE번호의188필드를 비교한다. **2초기접두+232정상 callback**, 1,688,380명령, 두 정밀도차0이다. 각 원문 type본문 SHA/입력AST/store를 저장한다. 기하/클러스터는 기존 sunArcher35클러스터를 공급하는 미선택 필드 경계이므로 각 자산의 전체 TYPE/기하 또는 게임로딩 완료를 주장하지 않는다.

사본은 `RiftTypeRecord::techBitIndex{-1}`와 기존 속성 parser의 `techBit` 분기만 추가한다. 현재 Record에 실제 signedDWORD를 저장하고 뒤 소비자도 같은 필드를 읽는다. 기존 `ToInt`는32비트 범위 밖을0으로 바꾸므로 재사용하지 않았다. 좁은 `TechBitNumber`는 기존 double 숫자 보관을 **float32**로 좁힌 뒤 signed64 절삭의 하위DWORD를 signed 해석한다. 예를 들어 원문 `2147483648.0f`는 `80000000`, `4294967808.0f`는 `00000200`이다. 비유한/64비트 범위 밖의 실제 불확정 정수 하위DWORD는0이다. 숫자가 아닌 속성의 제품 `invalid_argument`는 원문 진단 후 잘못된 AST 접근과 구별한 호스트 입력 계약이다.

[제안 patch](../../extracted/cpp-type-tech-bit-20261024-proposed.patch)의 SHA는 **eeeef5b60066d3bb6004d37151f1c8da1d4b409e8f467e6a9b35e8a8bb73ba8e**다. 대상은 기존 `RiftType.h`/`RiftTypeTable.cpp` 두 파일과 신규 `TypeTechBitTests.cpp`뿐이다. Record 크기가 바뀌므로 통합 후 관련 전체 번역단위를 다시 빌드해야 한다.

격리 **첫 build1 종료0·/W4 경고0**, **2검사/실패0**이다. 첫 검사는 기존 lexer/table로27입력의 선택71번과 나머지187초기필드를 모두 비교한다. 두 번째는 같은 보호 TAFF116본문 전체를 기존 lexer로 읽어 실제 Record188필드를 최초 Native와 전부 비교한다. 전체 기하의 Native 동등성을 검사한 것은 아니다. 공유 lib는 사용하지 않았고, 미선택 SquidFactory 전체의존성 대신 기존 `TypeConstructorAddress` 함수/주소표를 그대로 추출했다. [격리 SHA/로그](../../extracted/cpp-type-tech-bit-20261024-isolated/verification.json)에 근거가 있다.

[사본 적용 검증](../../extracted/cpp-type-tech-bit-20261024-proposed/apply-verification.json)은 Git 조상 없는 **C:Temp cpp-type-tech-bit-zo9pqz6z**에서 check/apply0, 실제3결과의 전체 정규화문자열/양쪽 rawSHA·새검사 생성·공유2SHA 불변을 확인했다. 첫 Native/patch/제품의 검증 범위를 서로 합산하지 않았다.

root 등록 시 `TypeTechBitTests.cpp` 두 검사와 **NETSTORM_TYPE_TECH_BIT_FIXTURE**, **NETSTORM_TYPE_TECH_BIT_ASSETS_FIXTURE**, **NETSTORM_TYPE_TECH_BIT_ORIGINALS**에 두 TSV/보호 디렉터리 절대경로를 공급한다. 기존 RiftType/Table/lexer/archive 몸체만 사용한다. 적용 순서는 **이 필드 patch→별도 CoreRefresh Session patch**이며 Session/Cursor 사본 전체를 덮어쓰지 않는다.

동료 fury는 사본3파일/두검사를 읽고 float32→signed64하위DWORD, 초기−1/속성순서, 실제188Record와 definition 수명에 명백한 오류를 찾지 못했다. Native의 기하 투영 한계도 유지된다. 추가 빌드/Native실행을 한 검토는 아니며 patch/사본과 두 최초 Native자료는 동결했다.

CoreRefresh의 정상 타입 전역은 **005412ec=165(다이스)**이며 VT507950의 가상+80은 기존 **RawTempleRitual::Count(004478f0)**다. 앞서 Player152의 RawSquidLevel 재사용으로 해석한 제안은 폐기한다. 실제 Editor checkpoint는 LocalLogin 없이 만들어지고 `004beca0(0)`가 editor/post 관문에서 자연 반환하므로 현재 Core를 임의 owner1로 선택할 근거가 없다. Editor의 CoreWrite도 현재 editor/post면 자연 반환한다. 따라서 기술필드 복원 자체가 Client Corealias/StateCleanup 수명 연결을 완료하지 않는다. 후속 Session 제안은 실제 `Login.currentCore/cachedCore`가 존재하는 선택만 연결하고, 그 외 선택된 현재 Core의 부재를 명시 필수누락으로 유지한다.
