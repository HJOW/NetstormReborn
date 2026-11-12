# webpj Template9 저장소 — CampaignTemplateStorage (WB04-E345~, 2026-11-12, 4-D B 구획)

3.1절 4-D B 구획의 client **CampaignTemplateStorage**(기준선 `.h` 108 + `.cpp` 587 = 695줄, 기준선 뒤 `.cpp` +41/−8·`.h` +18줄)를 `core/campaign-template-storage.js` 로 옮긴다. Client 와 Session 이 **같은 Template9 manager 이미지와 State cursor 수명을 공유**하는 소유자이며, 큰 클래스라 세 단위로 나눈다: **1부(E345)** 이 문서 1~4절, 2부(E346) 파일 읽기/Load/InstallFile, 3부(E347) 로그인 이미지 복제·저장·덤프. 다른 단계가 이 저장소를 쓰는 곳은 Player State getter(RawPlayerTemplateState)·월드 로더(Territory/Money/Badges/Technology 선택)·Session 로그인(State 확장·Subscriber)이다.

## 1. 모델 (1부)

- **manager 이미지:** 호스트 포인터 대신 DWORD 핸들로 `images` 표가 `Image` 를 소유한다. 한 `Image` = 276BYTE manager(`+0` 가상표·`+4` head·`+8` current·`+12` 파일명 256BYTE·`+0x10c` 서명 판정·`+0x110` FE 판정) + 섹션 노드 표. 9개 20BYTE 헤더(`+0` manager 핸들·`+4` 빌림 표시 등)는 고정 180BYTE 이고, 소유한 칸만 반납하며 **빌린 칸**(`+4`=1)을 비워도 원래 소유자의 이미지는 유지된다(자기 빌림·원래 소유자를 반납하는 같은 manager 빌림은 거부).
- **섹션 노드:** 60BYTE(`+0` 버퍼 토큰·`+4` pos·`+8` end·`+12` 할당·`+16` 이름 40BYTE·`+0x38` next)이고 표준 35개 이름은 `fort.sections` 위치를, 그 밖의 이름(`Temp` 등 동적)은 노드가 소유한 별도 버퍼를 쓴다. 버퍼 토큰 = `0x10000000 + 노드 핸들`, 인라인 쓰기 중 임시 할당 토큰 = `0x20000000 + 핸들`(호스트 주소를 자르지 않는다). 이름이 바뀌어도 노드 핸들·생성 수명·버퍼 할당은 유지되고 표준 위치만 옮겨진다.
- **ByteVector:** `std::vector<uint8_t>` 의 size/capacity/move 의미(용량 안 `resize` 는 같은 저장소·늘린 칸 0, 용량 초과는 새 저장소로 내용 보존 이동, `take` 는 원본을 비움)를 가진 소유 버퍼이다. 표준 위치의 변화는 `fort.sections[i]` 의 현재 논리 크기 뷰로 반영한다. 노드 `+12`(할당)는 이 용량이다.
- **typed cursor:** `state{bytes,position,end,present}`·`territory{bytes,position,end}`·`money{position,end}`·`badges`/`technology{bytes,position,end,present}` 는 **Image 가 소유한 같은 객체**이며 선택·확장·이름 교체 때 필드만 갱신된다(C++ 멤버에 값을 대입하는 의미라 빌린 참조가 계속 살아 있다 — 이 단위의 시험이 처음엔 매번 새 객체를 만들어 낡은 참조가 생기는 것을 드러내 교정했다). 이름을 잃은 노드의 cursor 는 비워 이전 위치를 새 이름의 노드에 쓰지 않는다.

## 2. 이 단위(E345)가 옮긴 것

- 빈 manager 흐름: `allocateEmptyManager(276)`(new276 만) → `constructEmptyManager`(원문 ctor, 판본 전역 005c89c0 에 0 을 씀) → `initializeEmptyManager`(35개 이름 Create). `install`/`borrow`/`clear`/`clearAll`/`ownedImages`/`managerLifetime`/`name`/`fileHeader`.
- 섹션: `createSection`(같은 이름 기존 노드 unlink+버퍼 반납 뒤 새 마지막 노드, 16384 할당)/`createManagerSection`/`findSection`(실패는 current 0 이고 null)/`selectSection`(쓰기 허용 모드)/`renameSection`(대상 제거 뒤 source 노드의 이름만 교체, 노드·할당 보존)/`currentWriteStream`/`writeSection`/`accessSection`(인라인 쓰기 빌림, 중첩·구조 변경 금지, 예외 시 부분 쓰기와 end 보존)/`ensureStateSize`(임시 노드 `T$e$m$p` 로 복사 확장)/`sectionOrder`/`currentSection`.
- cursor/State: `state`/`selectState`/`initializeState`(28BYTE)/`territory`/`selectTerritory`/`money`/`selectMoney`/`badges`/`selectBadges`/`technology`/`selectTechnology`/`readSubscriberName`(ID 4BYTE 건너뜀·WORD 길이·255BYTE 이하)/`growState`·`commitStateGrowth`(분리 확장).
- **옮기지 않은 것(1부):** 파일 읽기 `ReadFile`/`LoadFile`/`InstallFile`/`BindFileStreams`(2부), `CloneSectionFrom`/`ReadLoginImage`/`SaveImage`/`Dump`/`SaveFile`(3부), `BindCurrentName`(Editor 전용 `RawFortCurrentName`, 웹 제외).
- 기준선 뒤 변경(`.cpp` +41/−8, `.h` +18; PORT_MAP base `a328ac1f` 가 3.6①로 판정): 파일 읽기의 이미지 장치 공용화(같은 결과)와 원격 수신 `ReadImage`(MP). 웹은 기준선 판을 옮긴다.

## 3. 검사 (`tests/campaign-template-storage.test.mjs` 20건)

- cpppj 이식: `CampaignTemplates_DynamicTempRenamePreservesNodeAndAllocation`·`DynamicTypedCursorFollowsCurrentName`(`DynamicTwentyTerritoriesAndCurrentSave` 는 SaveFile 이 필요해 3부).
- 웹 계약: ByteVector 의미 직접 확인, 헤더 소유/빌림, 판본 전역 0 쓰기, 빈 manager→35개 이름 순서, 섹션 교체·오래된 참조 거부(생성 수명/없는 manager), 인라인 쓰기 빌림의 구조 변경 금지와 예외 보존, State 28BYTE 초기화·최소 확장 뒤 새 할당에 쓰기, 분리 확장, Subscriber 이름(255 이하·NUL 앞), cursor 공개/선택/위치 동기화/이름 교체 시 비우기, 쓰기 허용 모드, 일부(10개) 표준 섹션 요새의 Temp→표준 이름 교체 중복 방지, 빌린 cursor 참조 생존.
- 이 저장소는 실제 파일 fixture 가 아니라 손으로 쓴 시험이 기준이다(하위 DataManager 부품은 각자 원본 관측 fixture 로 이미 대조됨). 파일 읽기(2부)부터 원본 관측과 만나는 시험을 더한다.

## 4. 변형 확인과 남은 경계

- 한 문자열 치환 변형 57개를 돌려 첫 실행에서 23개가 남았다. 시험을 보강(ByteVector 직접 시험, 이름 교체 뒤 낡은 cursor 위치 무영향, 인라인 쓰기 예외의 end 보존·잘못된 반납 토큰, 일부 섹션 요새의 중복 연결 방지, 최소 확장 뒤 쓰기, 이미 빌린 칸 재빌림, 쓰기 허용 모드, 분리 확장 중 최소 확장 거부)한 뒤 **6개만 남았고 모두 동등**하다.
  1. `compactTail` 의 `end === alloc` 조기 반환: 같은 조건을 `compactMemStream` 이 안에서 다시 검사해 무동작이라 결과가 같다.
  2. `bindRenamedBuffer` 의 같은 표준 위치 조기 반환: 제거해도 벡터를 자기 자신에게 이동했다 되돌리고 cursor 를 다시 공개해 결과가 같다.
  3. Subscriber 길이의 `SIGNED_LENGTH_LIMIT` 비교: 같은 길이는 이미 256BYTE 한도 비교가 거부한다.
  4. Subscriber 의 `position > end` 비교: 앞선 `bytes.length - position < 2` 검사가 end ≤ 버퍼 길이 불변에서 같은 입력을 거부한다.
  5. `createManagerSection` 의 `!manager` 검사: manager 0 은 표에 없어 `!image` 가 같은 오류를 낸다.
  6. 소멸 효과의 `retiredState.release()`: Image 가 곧 버려져 관찰할 수 없다.
- 이 단위 시험이 드러낸 구현 오류 하나: 처음에는 cursor 를 공개할 때마다 새 객체로 바꿔 `storage.state(1)` 등을 미리 잡은 소비자가 낡은 참조를 갖게 되었다. 같은 객체에 값을 대입하도록 고쳤다(1.의 typed cursor 항목).
- 남은 경계: 2부(파일 읽기와 Load/Install), 3부(로그인 이미지 복제/저장/덤프), Session/Client 조립에서의 실제 소비자(Player State getter·월드 로더).

## 5. 2부 — 파일 읽기와 Load 부모 (WB04-E346)

- **ReadFile(`Image.readFile`)·BindFileStreams:** 빈 이미지가 처음 읽을 때는 요새 섹션 35칸과 표준 위치 벡터, typed cursor 를 빈 상태로 되돌리고(이미 노드가 있으면 동기화만), 하위 `readDataManagerFile`(원본 관측 fixture 로 대조된 하위 부품)에 이름표·노드 할당(60BYTE, 최대 35개)·버퍼 할당(새 버퍼 중복 할당 금지)·읽은 파일 할당(`0x30000` 토큰)·서명 판독(`typeCount`/`tooManyTypes`/`rejectFile`)·반납을 연결한다. 읽은 뒤(성공이든 호스트 예외든) `bindFileStreams` 가 원문이 이미 붙인 노드만 같은 순서로 표준 위치로 옮겨 공개하고 논리 end 로 줄인다(미완료 임시 노드를 붙이지 않고 부분 pos 를 숨기거나 롤백하지 않음). 성공했는데 새 노드가 35개가 아니면 이름 표 불일치로 거부한다. Native 경로의 `readWhole`(최소 4BYTE 할당과 논리 길이 분리)과 값형 `read`(빈 값은 0 취급)를 모두 지원한다.
- **LoadFile(`loadFile`, 004bf550 연결):** 원문 `loadTemplateFile` 에 실제 소유자를 연결한다 — 기존 파일이 있으면 같은 이름/다른 이름 모두 할당·읽기·해제 없이 비교(다르면 진단)만, 빈 헤더면 `+4`=0 → new276 핸들 → 생성자(판본 전역 0) → 헤더 공개 → 읽기 전용 사전 설정(`+8` 이 0 일 때만 판정해 1/0) → `+16`=0 → 실제 읽기. 읽기 성공이면 헤더 `+0` 을 **다시 읽어** 비영이고 현재 슬롯일 때만 제목을 갱신, 정상 false 면 현재 manager 만 소멸·반납하고 부가 필드와 callback 이 쓴 값은 보존한다. 파일 장치 예외는 정상 false 가 아니며 공개한 manager 를 롤백하지 않는다. readonly callback 이 헤더를 다른 owner 로 돌리면 그 manager 에 병합 읽기를 수행하고 빈 새 섹션은 기존 섹션(노드·생성 수명·할당)을 유지한다. 장치는 도달한 분기에서만 요구한다.
- **InstallFile(`installFile`):** 파싱된 보조 자료와 같은 파일 원문을 새 manager 에 읽는다. 서명 거부(`rejectFile` 은 예외)·TYPE 개수 256 이상 진단·빈/잘린 이미지는 설치하지 않으며 헤더를 바꾸지 않는다. 기본 TYPE 개수 188 은 10.78 독립 호출용이다.
- **시험:** `tests/campaign-template-load-file.test.mjs` 11건 — cpppj `CampaignTemplateLoadFile_*` 9건 이식(첫 시험의 SaveFile 단언만 3부로 미룸)과 installFile·Native readWhole 웹 계약. **첫 실행에서 cpppj 9건이 모두 일치했다.** 하위 읽기 부품(`datamanager-read`·`datamanager-live-read`·`datamanager-live-body` 등)은 각자 이미 대조되어 이 소유자는 연결 계약을 시험한다.
- **변형 확인:** 한 문자열 치환 변형 26개 중 첫 실행에 7개가 남았고, 시험 보강(빈 섹션의 논리 길이 0, 부분 읽기 뒤 동기화가 읽지 않은 표준 섹션을 되살리지 않음, 4BYTE 미만 할당 거부) 뒤 **6개만 남았으며 모두 동등/도달 불가**하다: ① `bindFileStreams` 의 `end > buffer.size` 보호(정상 읽기에서 도달 불가) ② 빈 이미지 최초 읽기 초기화(`resetForFileRead`)를 건너뜀(표준 위치는 읽기 뒤 바인딩이 같은 상태로 채움) ③ 노드 개수 상한 `>=`→`>` 경계(36번째 노드는 이름 표가 만들 수 없음) ④ 값형 읽기 길이 +1(물리 버퍼가 범위를 제한해 결과 동일) ⑤ 성공 시 35개 이름 표 불일치 검사(도달 불가) ⑥ `readImage` 의 입력 복사(읽기 쪽이 변경하지 않음).
- **남은 경계:** 3부(`CloneSectionFrom`·`ReadLoginImage`·`SaveImage`·`Dump`·`SaveFile`), 실제 Client/Session 에서의 Load 호출자, Fort 저장과의 관계.
