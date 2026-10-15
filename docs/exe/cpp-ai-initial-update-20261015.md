# 실제 AI 초기 기술·분류 갱신

현재/마지막 정밀 디컴파일 PC는 **VM-W11-Codex, 2026-10-15**다. 보호 PE와 정밀 프로젝트를 읽기 전용으로 분석했다. 원본/복사본 게임이나 GUI를 실행하지 않았고 보호 파일/AGENTS를 변경하지 않았다. 구현은 PATCH10.78 전용이다.

`RawAiBrainInitialUpdate::Update`는 원본 **`00414710`**을 같은 `RawAiBrainMemory`의0xb08본체/실제 builder 목록, 같은 Player9/기술 부모와 `RawPlayerTechnology`, 기존 `RawContainedFinder` 및 현재 TYPE 비용/flags2/theme에 연결한다. `004709d0`의 실제 원소별 워크샾 스위치도 복원했다. 초기 분류를 비워 둔 성공 callback으로 대체하지 않는다.

## 실제 이름과 기술 초기화 순서

1.600바이트를 복사하기 전에 본체260의 owner를 저장한다. 같은 본체에 새 설정600바이트를 복사한다. mission00594fa4가 비제로일 때만 **저장한 owner**의 Player+c에 이름을 쓴다. 설정 첫 바이트가0이면 실제 언어 `004410f0("defaultAiName",0,0,0)`이 필요하다. 비제로이면 본체의 NUL 문자열을 그대로 복사한다. 이름의 나머지 공간을0으로 지우지 않는다.
2. 이후 기술 처리는 매 원본 위치에서 **현재 본체owner**를 다시 읽는다. 실제 `RawPlayerWorld::Technology(owner)`가0이면 `Create(owner)`를 실행하고, 다시 Technology를 조회한다. 그래도0이면 나머지 분류를 실행하지 않는다. 같은 실제 PlayerKnown 진단/Player152 생성 경계를 재사용한다.
3. 기술 부모가 있고 새 설정의Tech+1e가 비어 있지 않으면 실제 `RawPlayerTechnology::AddList`를 호출한다. 같은40개 문자열 목록/중복제거/Ensure/contained 생성 사슬이다. 기존 기술을 여기서 일괄 삭제하지 않는다.
4. 본체3b4/3b8/3bc/3c0/3c4의5DWORD와 최대 비용290만0으로 만든다. builder count28c, 원소목록 count3b0, 선택 워크샾394, 우선 원소390, 최대 요구에너지길이ad8은 **이 함수에서0으로 초기화하지 않는다**. 최초 값은 실제 생성자에 의해 준비되며 재호출에서는 누적/유지된다.
5. 현재 기술 부모를 다시 조회해 실제 contained cursor를 시작한다. 이 커서는 반환 전 next를 저장한다. 각 현재 후보는 stored type WORD18의 TYPE를 먼저 조회하고 그 뒤 flags WORD20의 bit4를 검사한다. 후보의 free/dead 상태를 임의로 추가 필터하지 않는다.

**SP지정이나 지갑2d8 재설정은 이 함수에 없다.** 새 StartMoney22c는600바이트 복사에 포함되지만 생성자에서 준비한 지갑2d8은 그대로 유지한다. Start의 직접 SP 지정0040edb0, 후속 지급/피해의 지갑00413bb0과 구분한다.

## 실제 builder·최대 길이/비용·원소 분류

flags4 후보의 TYPE.flags2가30000 중 하나이며 signed builderCount<signed capacity이면 실제284 포인터가 가리키는 소유 배열에 type DWORD를 추가하고28c를 증가시킨다. capacity10의 배열 전체를 다시 만들지 않는다. 가득 찬 목록은 그대로 둔다.

TYPE+a0의 실제 요구에너지 문자열 길이를 ad8과 비교한다. 원본은 `ad8<=첫길이`이면 TYPE/문자열을 **다시 조회**해 두 번째 길이를 저장한다. 새 모듈도 같은 경우 getter를 두 번 호출한다. 이 필드는 현재 RiftTypeRecord에 없으므로 생성자와 같은 실제 `requiredEnergy(type)` 공급자가 필요하다. 타입 이름/assetName/문자 수를 임의로 대입하지 않는다.

290은 단순 float 합이나 최저가가 아니다. 현재 DWORD를 **signed int로 FILD**한 값과 현재 TYPE의 float 비용을 비교하고 큰 쪽을 원본 ftol(signed64 절삭의 하위DWORD)로 저장한다. 같음/NaN의 경우에는 float 비용 쪽을 고른다. ftol 무효64비트 결과의 하위DWORD는0이다. 따라서 분류 순서, int32 감김, 분수 비용, NaN을 보존해야 한다.

현재0054126c(기본133) 타입은 여기까지 builder/길이/최대비용에 포함하고 **그 뒤의 원소 분류만** 제외한다. 나머지 후보는 theme별 개수를 증가시킨다. flags2의800이 있으면 theme별 배터리 개수도 증가시키고 현재390==3일 때390을 그 theme으로 바꾼다. 같은 원소의 개수가1/6/11/…일 때만 원소 번호를398 목록에 추가한다.

이 추가 위치에서 실제 `004709d0`은 theme0/1/2/3을 현재005411b0/b4/b8/ac(워크샾86/87/88/85)로 변환하고4/5는0을 반환한다.394가0이면 그 타입을 선택한다. 이미 값이 있으면 **새 워크샾의 ftol 비용이 기존보다 클 때만** 바꾼다. 선택은 가장 비용이 큰 워크샾이며 최저가가 아니다. 같은 비용은 기존 선택을 유지한다.

마지막으로 theme0..4를 방문해 해당 원소의 후보가 정확히1개이며 그 하나가 배터리인 경우를 처리한다. 기존 원소 목록을 순회하며 중립3의 현 개수가5의 배수이면 해당 위치를3으로 치환하고3b4+theme×4에2를 기록한다. 그렇지 않으면 첫 일치 하나를 제거/압축하고1을 기록한다. 두 경우 모두 중립 개수를 증가시킨다. 기존 목록 count를 사용하며 삭제가 없던 나머지 원소도 읽고 복사하는 순서를 유지한다.

원본의398 목록이 다른 필드/현재3b0 count와 겹치는 재호출 입력에서는 쓰기 뒤의 **현재 count를 다시 읽는 순서**도 보존한다. 호스트0xb08 밖의 메모리 손상과 theme의 로컬10정수 배열 밖 접근은 명시 범위 오류다. 원본의 잘못된 메모리 접근까지 host stack에 그대로 실행하는 범위는 아니다.

## 원본 대조와 검사

[선택 export2함수](../../tools/ghidra/ai-initial-update-functions.json), [격리 관찰 도구](../../tools/decomp_ai_initial_update.py), [SHA 보고서](../../cpppj/ai-initial-update.json), [fixture](../../cpppj/tests/fixtures/ai-initial-update-x86.tsv)를 추가했다. 같은 입력 재생을 합산하지 않는 **133고유입력**은128개 정상 조건 교차와5개 기존count/목록/최대길이 재호출 상태다. 원본 함수들은 모두 정상 반환했다.

전체0xb08본체, 실제 builder40바이트, 전체 Player9의1620바이트, 실제 새 Form의50바이트와 언어/타입이름/요구에너지/생성·부착 효과 순서를 비교한다. 언어 이름 유무, mission on/off, 기술 텍스트 유무, 기존 기술부모 유무, 실제 기술 추가, 배터리 중립 치환/제거, flags0/비contained, builder 포화, TYPE133 분류제외, NaN 비용, 기존 count/배열 겹침을 포함한다. 모든 입력에서 새 StartMoney777을 복사해도 기존 지갑999가 유지됨을 별도로 확인한다.

원본00414710/004709d0 및 기존 PlayerTechnology5함수/contained 생성/실제 finder/ftol은 기계어로 실행한다. 원본 쪽 언어 이름/PlayerWorld getter·Create의 현재 기록 응답은 명시 외부 경계다. C++ 쪽은 기존 실제 PlayerWorld::Technology/Create와 PlayerTechnology/RawContainedCreate/Factory/Form6를 사용하며 상위 가상 Pop/부착의 raw 응답은 명시 경계다. 전체 Player152 생성/공간/부모알림까지 이133개로 재검증했다고 주장하지 않는다. 임시 목록 heap 주소/OOM과 전체 Pool free list도 기존 경계다. 검사가 제품처럼 가짜 getter/allocator 요청 trace를 합성하지 않는다.

`AiInitialUpdate_Native133`, `AiInitialUpdate_MissingSelectedMetadataAndWalletPreserved` 집중2개를 추가했다. 두 번째 검사는 선택되지 않은 언어/에너지 경계의 생략과 선택된 실제 metadata 누락의 명시오류/앞선 실제 부분 효과/지갑 보존을 확인한다. 현재133입력 고유성/SHA/UTF-8/새파일 포함 행끝공백 감사가 통과했다. 루트가 CMake 등록과 Release 게임/검사 빌드를 수행했고 **경고0·오류0·집중2개 실패0**이다(`extracted/pause-final-build.log`, `extracted/pause-final-focused.log`). 이후 페이지 소스의 강제 재컴파일도 경고·오류0이다(`extracted/pause-metadata-final-build.log`). 실제 Session 활성화/전략·완주 검사는 아니다.

같은 소스의 최종 전체 CTest는 **1/1·내부1,233개·실패0, 112.41초**로 통과했다(`extracted/pause-final-ctest.log`). 이 단위로 RawAiCreate의 생성자/초기update를 실제 몸체에 연결할 자료 전제는 갖추어지지만, 아직 실제 TYPE+a0 metadata와 언어/Player/세계 수명 공급을 세션에서 조립해야 한다. **전략00417d30/00417de0/00414af0/00416eb0/004139f0/00417c70 및 실제 AI handler 전체는 미복원이다.** 생존 참여만 표시하는 가짜 brain/빈 전략으로 Outcome 자동 검사를 켜지 않는다.
