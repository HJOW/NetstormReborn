# cpppj 실제 Fort 객체·내용물·Terr 순회 로더

2026-10-13, **현재/마지막 정밀 디컴파일 PC VM-W11-CODEX**. AGENTS.md와 두 최신 LEFT_JOBS를 읽고 앞 단계 전체904개·새5318개 근거를 보존했다. 새 `RawFortObjectLoad.h/.cpp`, `FortObjectLoadTests.cpp`, 두 고유 Ghidra 목록·oracle·fixture만 작성했다. 보호 원본/복사본 게임·GUI 실행, 보호 파일/AGENTS.md/dotnetpj 수정, 커밋/푸시는 없다.

## 저장 객체와 현재 월드 효과

`Object(FortObject,originX,originY,defaultOwner)`는 실제 `004bdc60`/CD `00428320`의 객체 효과를 적용한다. 값형 parser의 데이터는 입력이며 결과는 같은 실제 SID 풀에 쓴다. 저장 타입0/현재 isle는 생성하지 않는다. 다리 약식은 현재 bridge 타입과 `255-storedType` 후보를 사용한다. 원본 x/y는 각 nibble에 청크 시작 칸을 더한 signed DWORD를 float로 저장한다. CD 디컴파일의 `quantityWord`가 Y 지역변수를 덮는 듯한 표기는 실제 명령의 Y 저장 위치와 다르므로 적용하지 않았다.

실제 순서는 factory `(type,0)`→현재 saveFrame BYTE의 raw frame 쓰기→현재 saveQA 낮은 BYTE/기존 높은 BYTE를 합친 WORD→saveQB signed WORD→다리 frame→version0 legacy state/2 이상 factory state→가상 owner→HasHitPoints/현재 MaxHitPoints→공통 HP 저장→지형/랜덤 frame→**가상 Pop9→contained 재귀**→Fort 편집 삭제 정책이다. callback 뒤 현재 타입·전역을 다시 읽는다. 필수 저장 필드가 없으면 임의0을 넣지 않고 거부한다.

초기 frame은 factory 결과를 유지하며 `.type defaultFrame`으로 교체하지 않는다. 저장/특수 frame은 `SquidFrame::Set`의 표시 갱신을 부르지 않는 원본 직접 쓰기다. WORD는 현재 bridge 타입일 때 표면 알림을 요청한다. HP body는 **004adca0 DWORD/CD004aed20 WORD 직접 쓰기**이며 사제 가상 HP 전환004942b0을 호출하지 않는다. HP 보유·최대값은 실제 `SquidReward` 어댑터로 연결해 사제1/4·dais/abstract를 유지한다.

owner는 version>0과 ownerSaved mask가 맞을 때만 저장 BYTE를 읽고0/9 이상을1로 정규화한다. special/mission 둘 다 꺼져 있으면 기본 owner로 덮으며 version0도 기본 owner다. ownerSaved 대상이 아닌 version>0 다리 약식은 약식 owner를 유지한다. ownerCleared mask·현재 isle/noIsland는0이며 isleBig은 원본 assert 경로를 예외로 거부한다. 소유자 BYTE 직접 쓰기 대신 실제 가상 분배 경계를 사용하므로 사제의 원래 owner/로컬 표식도 외부 실제 모듈로 연결할 수 있다.

지형 frame은 같은 pool/hash/types의 실제 일반 finder에서 genus2의 첫 통과 대상을 사용한다. 패치 DWORD `-1`은0으로 바꾸며 CD의 raw BYTE255는 zero-extend되어 `-1`이 아니므로255를 보존한다. 랜덤 frame은 이 Fort body의 고정 **P/P·flags wildcard·quartile 없음** 요청을 복원했다. P/P→방향 변환P/P→P/A→P/아무 변형에서 최대256 후보를 저장하고 현재 `GameRandom::Next(count)`를 한 번 소비한다. 실패에서는0 frame과 난수 미소비를 보존한다. 일반적인 임의 방향/flags/quartile 검색 API 전체를 구현한 것은 아니다.

## 내용물과 상위 순회

`Contents`는 `004bd130`/CD `00426dd0`의 현재 내용물 type.contentListFlags→현재 실제 부모 type.containerListFlags→저장 BYTE 순서를 사용한다. listFlags0x10이면 attach flags의8을 끄고16을 켜며 **후행 형제에도 유지**한다. `listFlags & ~0x100`, quantity DWORD, 품질0으로 실제 `RawContainedCreate`에 요청한다. 중첩에서는 생성된 SID를 부모로 재귀한다. type0/불완전 목록의0 반환을 보존하고 이미 생성된 앞선 내용물을 되돌리지 않는다.

중첩 parser의 현재 한계: 기존 `Template.cpp`는 중첩 목록을 읽을 때 내용물 원타입의 containerListFlags를 부모로 사용한다. 실제 재귀 부모는 생성된 **ContentForm(type6)** SID다. 두 값이 다르고 파일 BYTE가 필요한 합성/다른 맵은 값형 모델만으로 원래 BYTE를 복원할 수 없어 새 로더가 거부한다. parser나 보호 파일을 바꾸지는 않았다. 1-1의 빈 사제 목록·단일 sunWalker 목록에는 이 중첩 차이가 없다.

`Chunk`는 현재 청크 x/y를16배한 시작 칸으로 저장 객체 순서를 적용한다. `Load(FortTemplate,templateSlot,start,end)`는 실제 `004be610`/CD `00428d50` 순서로 깊이를 증가시켜 동일 월드에 전달하고 **Terr[start,end)→허용 시 Chaff**를 처리한다. 각 섹션은 y 바깥/x 안쪽이며 현재 signed 청크 지역과 실제 owner/Terr 정책의 반환값으로 선택한다. 지도 크기/버전/scope의 현재 값을 유지한다. templateSlot1..8은 해당 기본 owner이며 패치의 다른 칸은0이다. CD의 다른 칸은 원본 assert이므로 예외로 거부한다. type conversion 자료는 기존 parser가 소유한 변환된 레코드를 이 호출 내내 사용하며 raw 포인터/힙 allocator 자체는 재현하지 않는다.

정상 종료의 `finishOwner` 다음에 깊이를 감소시킨다. **finishOwner는 noop가 아니다.** 실제0040e1a0/CD0042ca70은 모드/Player 기본 지역과 genus0x40000000 자산 유무에 따라 지역 경계/추가 생성 배치를 반복한다. 이번 loader에서는 필수 외부 경계로 남았으며 새 독립 몸체 복원이 후속이다. host 예외에서는 깊이를 복구하되 이미 실행한 월드 효과를 취소하지 않는다. 손상 파일·원본 assert 이후 실행의 재현 범위는 아니다.

## 독립 관찰과 검증 범위

새 `fortobjectload` **원본31/CD32**, CD 추가 helper **4개**를 읽기 전용으로 내보냈다. 최초 helper3개 성공 기록과 초기 경계 규약/누락 helper 실패 로그를 따로 보존했고 성공한 본체 export는 재사용했다. `decomp_fortobjectload_oracle.py`는 실제 객체/내용물/청크/Terr0..19→Chaff/타입 getter·HP·일반 finder·랜덤 프레임·전역 RNG 및 정상 깊이 반납을 격리 x86으로 실행한다. factory/가상 owner/state/count/Pop/Destroy, contained 생성, 지역 정책, 데이터 관리/type conversion 준비·해제/owner 후처리, malloc/free만 명시 경계다.

두 PE **609개씩·새1,218개** 입력이 정상 반환했고 두 x87 정밀도 관찰이 같았다. 저장 필드 조합, owner/버전/편집/미션/다리 약식, HP/사제1/4/dais, 지형 frame -1·CD255, P/P/fallback/256 후보, 현재 타입/후처리 플래그/Pop 이후 container 변경, 내용물 형제/재귀/중단, 타입 변환/빈·isle skip, 실제 Terr0..19/빈 섹션→Chaff와 Template owner1/2/8을 포함한다. **중간1,212개·반복 실행·두 FPU·하위 함수 호출 수는 신규 수에 더하지 않는다.** 최종 source/export/보호 PE/fixture SHA·입력/실제 진입 감사도 통과했다.

자료: `cpppj/fortobjectload.json`, `cpppj/tests/fixtures/fortobjectload-x86.tsv`, `extracted/fortobjectload-native-final-complete.log`, `extracted/fortobjectload-verify-final.log`. 새 C++ 검사4개는 같은 입력의 전체 raw/효과/현재 난수·version/depth/scope를 대조하고 실제 factory/owner/HP/Pop9→정렬 ContentForm 부착 및 부모 알림/공유 해시를 연결한다. 최초 공유 빌드에서 검사 전용 Scene/TypeInput이 공통 rawscene 이름과 모호했던 로그를 보존하고 FortLoadScene/FortTypeInput으로 교정했다. **공유 빌드 뒤 집중4개·실패0**을 root가 확인했다. 같은 새1,218개 입력의 원본/C++ 대조가 포함되며 전체 회귀는 상위 공유 작업의 최종 기록을 따른다.

실제 미션 월드 이관은 root의 `RawSurfaceWorld::Core/BeginLoad/FinishLoad/Create/Pop`과 동일한 pool/hash/spot/장부·사제 소유자/상태·ContentForm 알림·지역 선택·finishOwner 실제 몸체를 연결해야 한다. 실제 GUI/건설/전투/사제 운반·의식/승패/캠페인1-1 완주 검증은 이 독립 로더 관찰로 완료 처리하지 않는다.
