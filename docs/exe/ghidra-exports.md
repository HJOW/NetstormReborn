# 기계어 대조 입력(Ghidra 내보내기) 준비

2026-10-11 최신(`HJOW-Athlon`): **[커서 미리보기 표시 수집 복원](cpp-preview-collection-reconstruction.md)** 완료. 커서 조각의 실제 raw SID 목록/선택 타입/부모 깊이/spot을 읽는 `PreviewCollection`을 복원하고 공통 수집/정렬/kind 4 분배에 연결했다. 실패 후보의 14바이트 부분 기록·kind/tail 보존·+0.9999/CRT·두 레이어/원래 보조 깊이·soft 쌍 제한·현재 목록·판본별 잠금/피드백을 원본과 대조했다. 준비한 실제 SP 저장소의 읽기도 연결했다. 새 독립 원본 **1,560개**·누적 **510,303개**, 두 x87 정상 반환 **3,120회**. 새 C++ **8개**, Release 경고/오류 **0**·CTest 내부 **738개·실패 0**(151.98초, 전체 152.03초), 근거 감사 **112종**·새 변이 **11개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **미리보기/일반 sprite 실제 Draw·커서 생성/입력·SP lazy 상태 연결·월드 투영·실제 Gump/구름/배경/dirty/장치·Client/GameWorld 자동 프레임·클릭 선택·건설 SHP/팔레트 출력은 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`previewcollection` 3/3/3개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[일반 자산·그림자 표시 수집 복원](cpp-object-collection-reconstruction.md)** 완료. 일반 자산/그림자의 실제 1~3단계 해시·raw SID 체인·타입/SHP 경계를 읽는 `ObjectCollection`을 복원하고 공통 수집/정렬/분배에 연결했다. 본체와 그림자 별도 교차·판본별 float 저장/NaN·abstract tail·walker/그림자 y 보정·signed 깊이·현재 next·strict 누적 제한·정상 해시 2단계를 원본과 대조했다. 실제 SHP 월드 hotspot을 로더/프레임 캐시에 연결했다. 새 독립 원본 **1,431개**·누적 **508,743개**, 두 x87 정상 반환 **2,862회**. 새 C++ **7개**, Release 경고/오류 **0**·CTest 내부 **730개·실패 0**(199.98초, 전체 200.05초), 근거 감사 **111종**·새 변이 **10개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **미리보기 수집 내부·월드 투영·실제 Gump/각 종류 Draw·구름/배경/dirty/장치·Client/GameWorld 자동 프레임·클릭 선택·건설 SHP/팔레트 출력은 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`objectcollection` 7/4/4개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[지면·다리 행별 표시 묶음 복원](cpp-tile-group-collection-reconstruction.md)** 완료. 지면/다리의 0단계 해시 머리 SID를 raw 슬롯에서 행별로 수집하는 `TileGroupCollection`을 복원하고 공통 묶음 수집/분배에 연결했다. +0.5/CRT 절삭·한 칸 확장·독립 클램프·행/열 순서·첫 SID·signed BYTE 깊이·kind 1/tail 0·빈 칸 배경 판정·strict 행별/판본별 헤더 제한·정상 반환의 해시 2단계를 원본과 대조했다. 묶음 인수 이름을 시각에서 `groupRowLimit`으로 바로잡았다. 새 독립 원본 **1,059개**·누적 **507,312개**, 두 x87 정상 루트 반환 **2,118회**. 새 C++ **6개**, Release 경고/오류 **0**·CTest 내부 **723개·실패 0**(153.36초, 전체 153.41초), 근거 감사 **110종**·새 변이 **10개 모두 통과**(`--focused`, 변이 전 사본 전체 검사 통과). **일반 자산·효과/미리보기 수집 내부·월드 투영·실제 Gump/각 종류 Draw·구름/배경/dirty/장치·Client/GameWorld 자동 프레임·클릭 선택·건설 SHP/팔레트 출력은 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`tilegroup` 5/4/4개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[UI/Gump 화면 좌표·트리 표시 수집 복원](cpp-gump-collection-reconstruction.md)** 완료. UI/Gump의 부모별 float 위치 누적·판본별 월드 부착 반올림·화면 경계·트리 수집을 `GumpCollection`으로 복원하고 실제 공통 UI 슬롯에 연결했다. 로컬 정렬 키·signed 깊이·tail 0·화면 밖 자식 통과·최상위/자식 필터와 현재 링크·루트별 limit·캐시 보존을 원본과 대조했다. 새 독립 원본 **1,167개**·누적 **506,253개**, 두 x87 루트 정상 반환 **2,334회**. 새 C++ **6개**, Release 경고/오류 **0**·CTest 내부 **717개·실패 0**(219.13초, 전체 219.17초), 근거 감사 **109종**·새 변이 **9개 모두 통과**(`--focused`, 변이 전 사본 전체 검사 통과). **묶음/일반 자산·효과/미리보기 수집 내부·월드 투영·실제 Gump 생성/Draw·구름/배경/dirty/장치·Client/GameWorld 자동 프레임·클릭 선택·건설 SHP/팔레트 출력은 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`gumpcollection` 6/7/7개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[공통 표시 목록 병합·종류 분배 복원](cpp-draw-submission-reconstruction.md)** 완료. `drawsubmission` **6/9/9개** 읽기 전용 내보내기. 세 PE의 상위 Renderer 루트와 실제 CRT/클립/좌표/CD 묶음 순회를 두 x87 정밀도에서 정상 반환까지 실행했다. 수집 응답/그림 장치 내부만 명시 경계이며 선택한 dirty 한 개/모드 0·128 분기의 관찰이다. 새 **966개**·누적 **505,086개**, 상위 루트 반환 **1,932회**. Release 경고/오류 **0**·CTest 내부 **711개·실패 0**, 근거 감사 **108종**·새 변이 **8개 모두 통과**. 원본 게임/OS/assert 0. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 수집기 내부/전체 프레임·실제 건설 SHP/장치는 후속이다. 아래 기록은 당시 상태로 보존한다.

2026-10-11 앞 단계(`HJOW-Athlon`): **[원본 CRT 표시 정렬·동률 순서 복원](cpp-draw-sort-reconstruction.md)** 완료. `drawsort` **3/5/5개**를 읽기 전용으로 내보냈다. 실제 qsort/shortsort/swap/표시 비교/CD 옵션 wrapper를 두 x87 정밀도에서 전체 정상 반환까지 실행했다. 응답/계산/효과 대체·원본 게임/OS/assert 0. 새 **1,155개**·누적 **504,120개**, 정상 루트 반환 **2,310회**. Release 경고/오류 **0**·CTest 내부 **703개·실패 0**, 감사 **107종 통과**·실제 클론 창 스모크 통과. 새 변이 8개와 갱신한 앞 단계 1개, 총 **9개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 전체 수집/분배/프레임·건설 SHP/출력은 후속이다. 아래 기록은 당시 상태로 보존한다.

2026-10-11 앞 단계(`HJOW-Athlon`): **[표시 목록 수집·가시 영역·정렬 제출 복원](cpp-render-collection-reconstruction.md)** 완료. `rendercollection` **5/4/4개**를 읽기 전용으로 내보냈다. 목록 수집/환산/부모 getter/교차/공유 정렬 비교를 실제 명령으로 실행했고 계산/효과 응답 대체·원본 assert/게임/OS 호출은 0이다. 새 독립 원본 **4,305개**·누적 **502,965개**, 두 x87 정상 반환 **8,610회**. Release 경고/오류 **0**·CTest 내부 **696개·실패 0**, 감사 **106종 모두 통과**. 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). 실제 클라이언트 공통 목록/qsort/프레임·SHP/팔레트/장치 출력은 후속이다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 아래 기록은 당시 상태로 보존한다.

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 표시 등록·삭제 수명 복원](cpp-construction-lifetime-reconstruction.md)** 완료. `constructionlifetime` **7/13/13개**를 읽기 전용으로 내보냈다. RenderProcess 목록 생성/확장/제거·건설 공통/파생 소멸·CD 내부 helper·BaseProcess 소멸의 전체 몸체를 실행했다. 부착/new/free만 명시 경계이며 원본 assert/게임/OS 호출은 0이다. 새 독립 원본 **1,377개**·누적 **498,660개**, 정상 루트 반환 **2,766회**. Release 경고/오류 **0**·CTest 내부 **680개·실패 0**, 감사 **105종**·새 변이 **8개** 모두 통과. 실제 Kernel/form 부착·클라이언트 자동 Run/Draw·SHP/팔레트/장치 출력·건설 GUI는 후속이다. **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-11.** 아래 같은 날의 앞 단계 기록도 해당 시점 상태로 보존한다.

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 표시 프레임·입자 확장·Draw 복원](cpp-construction-frame-reconstruction.md)** 완료. HP별 활성화·인접 입자 확장/추가·수명/팔레트 갱신·원래 픽셀 복원·시각/표시 사각형·Draw 요청을 `RawConstructionFrame`으로 복원했다. 세 PE의 전체 몸체/연속 중간 상태와 실제 Create→Run/Draw 연결을 대조했다. 새 독립 원본 **1,317개**·누적 **497,283개**, 두 x87 정상 반환 **2,994회**. Release 경고/오류 **0**·CTest 내부 **674개·실패 0**(145.58초, 전체 145.61초), 근거 감사 **104종 모두 통과**, 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **표시 삭제/RenderProcess 등록·클라이언트 자동 Run/Draw·실제 SHP 캡처/팔레트 생성/장치 출력·타입 준비 소리 자료·부모 공간/사제 contained·건설 GUI는 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructionframe` 11/11/11개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 표시 생성·초기 입자 복원](cpp-construction-visual-reconstruction.md)** 완료. 즉시/일반 건설 표시의 생성·공유 팔레트 요청·캡처/출력 버퍼·좌표 동기화·초기 입자 선택과 실제 난수 소비를 `RawConstructionVisual`으로 복원했다. `MakeConstructionVisualHooks`로 실제 Start에 초기 상태 소유 저장소를 연결하고 표시 생성 이후의 건설 시각과 팔레트 재사용을 검사했다. 새 독립 원본 **1,776개**·누적 **495,966개**, 두 x87 정상 반환 **3,552회**. Release 경고/오류 **0**·CTest 내부 **668개·실패 0**(143.97초, 전체 144.01초), 근거 감사 **103종 모두 통과**, 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **표시 RunFrame/팔레트 갱신·Draw/삭제·실제 SHP 캡처/렌더러 등록·타입 준비 소리 자료·부모 공간/사제 contained·건설 GUI는 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructionvisual` 13/13/13개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[종속 객체 조회·삭제와 건설 진행음 정지 복원](cpp-contained-lookup-reconstruction.md)** 완료. 종속 체인의 타입 DWORD·dead 제외·일치 순번 감김·번호별 권한·삭제 뒤 next 읽기를 `RawContainedLookup`으로 복원했다. `MakeContainedConstructionHooks`를 통해 건설 완료의 진행음 삭제를 실제 host/Kernel·소리 프로세스·form Unpop/반납에 연결했고 다른 Regular 프로세스 보존도 검사했다. 새 독립 원본 **2,415개**·누적 **494,190개**, 두 x87 정상 반환 **4,830회**. Release 경고/오류 **0**·CTest 내부 **662개·실패 0**(146.26초, 전체 146.30초), 근거 감사 **102종 모두 통과**, 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **건설 표시 효과·타입 준비 소리 자료·가상 표시/부모 Unpop/재등록·사제 contained/하위 경로·공간·건설 GUI는 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`contained` 1/1/1개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[지정 ProcessForm 부착·단일 건설 패킷 분배 복원](cpp-process-form-reconstruction.md)** 완료. 지정 free 서버 form의 Take·부모 부착을 실제 host/Kernel에 연결하고, 새 건설 수신 객체를 소유하는 `RawConstructionData`와 `RawPacketDispatch`의 메시지 **0x37** 단일 즉시 분배를 복원했다. raw·풀 메타데이터 보존/길이/버전/시각/깊이와 서버→client 전체/부분 상태·HP 진행·권한 대기·삭제 통지 하위 몸체를 검사했다. 새 독립 원본 **1,836개**·누적 **491,775개**, 주 루트 정상 반환 **3,672회**(등록 루트 별도 **5,832회**). Release 경고/오류 **0**·CTest 내부 **656개·실패 0**(143.82초, 전체 143.85초), 근거 감사 **101종 모두 통과**, 새 변이 **8개 모두 검출**(`--focused`, 변이 전 사본 전체 검사 통과). **집합/지연 분배·사용 중인 form 재부착·삭제/부모 상태 전파·표시/contained·하위 경로/공간·건설 GUI는 후속이며 게임 내 건설/미션 완주는 아직 불가능하다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`processform` 10/11/11개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 상태 전송·수신 복원](cpp-construction-data-reconstruction.md)** 완료. 건설 프로세스 메시지 **0x37**의 상태 쓰기/읽기/수신과 가상 쓰기 래퍼를 `RawConstructionData`로 복원했다. 변경 비트별 부모 DWORD·시작/끝 double 비트·추가 BYTE, force=0xff·전송 뒤 dirty=0, 수신의 기존/free form 분기와 할당 뒤 현재 타입·캡처한 form을 보존했다. 실제 host/Kernel의 기존 건설 프로세스 수신과 Start→전체 전송→ResetEnd→끝 부분 전송→수신→지연 완료를 검증했다. 새 독립 원본 **5,340개**·누적 **489,939개**, 두 x87 정상 반환 **10,680회**. Release 경고/오류 **0**·CTest 내부 **648개·실패 0**(124.34초, 전체 124.43초), 근거 감사 **100종 모두 통과**. 새 상태 전송 변이 **8개 모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **지정 form 부착·0x37 디스패처·건설 표시 효과·contained 삭제·하위 경로/자원 정책/공간 Pop·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructiondata` 4/4/4개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 끝 시각 재설정 복원](cpp-construction-time-reconstruction.md)** 완료. 끝 시각 재설정 `00443dc0`/CD `004d0930`·`004d0970`과 form 조회 `004ae2d0`/CD `004af4e0`을 복원했다. 최대 HP 경계 뒤 현재 시각+10초를 끝 시각에 쓰고 변경 비트 4만 켠다. 시작/소리·부모 HP는 유지한다. CD는 진행 HP에서 손상량을 뺀 값을 기본 시간 helper에 넘기지만 helper가 그 인자를 읽지 않아 끝은 동일하다. 실제 `SquidProcessHost::FromForm`의 DWORD Kernel 조회와 건설 프로세스에 연결하고 원래/새 완료 시각의 실제 프레임 동작을 검증했다. 새 독립 원본 **1,269개**·누적 **484,599개**, 두 x87 정상 반환 **2,538회**. Release 경고/오류 **0**·CTest 내부 **641개·실패 0**(137.29초, 전체 137.32초), 근거 감사 **99종 모두 통과**. 새 끝 시각 변이 6개와 기존 건설 프로세스 변이 16개, 총 **22개 모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **전송용 상태 쓰기/읽기/수신·건설 표시 효과·contained 삭제·하위 경로/접근점/자원 정책/공간 Pop·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructiontime` 3/9/9개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 프로세스 시작·진행·완료 복원](cpp-construction-process-reconstruction.md)** 완료. 건설 프로세스의 시작 `00443fe0`/CD `004d06a0`·생성자 `00443c40`/CD `004d0720`·진행 `004428c0`/CD `004d0ba0`·완료 `00443e20`/CD `004d09e0`·form 삭제 통지 `00444030`/CD `004d08e0`을 `RawConstructionProcess`로 복원했다. 표시 효과 뒤 현재 시각의 시작/끝(지속 0은 10초)·부착·1초 초과 진행음 → 프레임별 `절삭((현재-시작)/(끝-시작)×최대 HP)`의 0~최대 제한·quickbuild/quickvortex·abstract/권한 Kill → 진행음 삭제·완료음/준비 소리·Unpop/abstract 해제/재등록(9)/밀어내기 → HP 채움 순서를 보존했다. CD의 진행음 비트 비교·HP WORD 폭/부호·예측 경계 6000·밀어내기 세 인자를 대조했다. 실제 `SquidProcessHost`/Kernel에 붙는 `ConstructionProcess`와 보상 모듈 최대 HP·밀어내기·통지의 건설 시작·소리 계층 어댑터를 연결했다. 새 독립 원본 **2,868개**·누적 **483,330개**, 두 x87 정상 반환 **5,736회**. Release 경고/오류 **0**·CTest 내부 **635개·실패 0**(157.98초, 전체 158.07초), 근거 감사 **98종 모두 통과**. 새 변이 16개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **끝 시각 재설정·전송용 상태 쓰기/읽기/수신·건설 표시 효과·contained 삭제·하위 경로/접근점/자원 정책/공간 Pop·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructionprocess` 12/14/14개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설용 객체 생성·이동과 공급원 소비 복원](cpp-construction-builder-reconstruction.md)** 완료. 건설용 객체 생성·이동 `00443000`/CD `004d1120`와 공유 공급원 소비 `00473570`/CD `0048aaf0`을 `RawConstructionBuilder`로 복원했다. 작업장 기준점→최초 객체 생성/소유자/접근/Pop/경로→시각 조정→공급원 선택/정렬→운반 객체 생성/현재 타입 프레임/중심/Regular/경로→최초 경로 현재 시각 반환 순서를 보존했다. 5.0 조기 실패와 앞선 효과 유지·signed 개수/구멍/중복·개수 불변·CD 프레임 BYTE·할당 실패 후 이동·경계 뒤 현재 자료 재조회를 대조했다. 로컬/통지 이동 어댑터와 실제 작업장 조회/factory/일반 owner/Regular Form·Kernel 통합 검사를 연결했다. 새 독립 원본 **1,110개**·누적 **480,462개**, 두 x87 정상 반환 **2,220회**. Release 경고/오류 **0**·CTest 내부 **622개·실패 0**(139.41초, 전체 139.53초), 근거 감사 **97종 모두 통과**. 새 변이 10개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **건설 시작/진행/완료·하위 경로/접근점/자원 정책/공간 Pop·소유 사제 contained·타입별 파생 효과·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructionbuilder` 23/20/20개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 기준점 연결·소유 사제 조회 복원](cpp-construction-selection-reconstruction.md)** 완료. 확정의 `0048fdb0`/CD `00406f80`은 작업장/그래프 기준점 조회여서 기존 `RawPlayerPlacementAnchor`를 연결했다. 배치 통지의 실제 소유 사제 조회 `004921f0`/CD `0040d940`을 `RawPriestLookup`으로 복원하고 공유 등록/삭제 목록에 연결했다. 현재 소유자 BYTE와 요청 DWORD 전체·dead bit 2·signed 개수·목록 순서/조기 종료를 보존했다. 실제 사제 factory/등록·소유자 지정/삭제 접두와 확정/factory→통지 통합 검사, 패치의 두 번째 조회와 첫 사제 보관을 대조했다. 새 독립 원본 **4,851개**·누적 **479,352개**, 두 x87 정상 반환 **9,702회**. Release 경고/오류 **0**·CTest 내부 **614개·실패 0**(153.60초, 전체 153.64초), 근거 감사 **96종 모두 통과**. 새 변이 8개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **건설 사제 생성/이동·시작/진행/완료·접근점/contained·타입별 공간/파생 효과·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`priestlookup` 1/1/1개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[건설 밀어내기/섬 정리 복원](cpp-construction-effects-reconstruction.md)** 완료. 건설 유닛 밀어내기(`00443800`/CD `004d2450`)와 섬 실패 정리(`0046e6c0`/CD `0048f960`)를 `RawConstructionEffects`로 복원했다. 판본별 세/두 칸 탐색·현재 genus·자리 선택 뒤 적대 거부·16.16 전파 좌표·추락/소리 억제·삭제 순서/플래그·9/1/1 조각 수 보고를 대조했다. 같은 풀의 서버 요청 어댑터와 실제 factory·SquidUnpop/공통 삭제·SID 반납 통합 검사를 연결했다. 새 독립 원본 **3,534개**·누적 **474,501개**, 두 x87 정상 반환 **7,068회**. Release 경고/오류 **0**·CTest 내부 **606개·실패 0**(158.44초, 전체 158.52초), 근거 감사 **95종 모두 통과**. 새 변이 12개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **경로 contained/타입별 공간 이동·파생 삭제/추락/알림·사제 선택/이동/건설 진행·Form/입력/건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructioneffects` 30/26/26개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[생산 칸 조각 반환 복원](cpp-cursor-return-piece-reconstruction.md)** 완료. 생산 칸 조각 반환(`004468b0`/CD `0043e930`)과 contained Form 생성(`004afa70`/CD `004ac4d0`)을 `RawCursorReturnPiece`·`RawContainedCreate`로 복원했다. 일반 품질 감소/플래그·다리 난수/추첨·유지 옵션 재조회·타이머 할당 실패/시간 선택을 원본과 대조했다. 타입 6 전용 factory 생성과 실제 Regular/Kernel, 로컬 배치/GUI 반환·전체 건설 통합 검사를 연결했다. 새 독립 원본 **1,899개**·누적 **470,967개**, 두 x87 정상 반환 **3,798회**. Release 경고/오류 **0**·CTest 내부 **600개·실패 0**(147.79초, 전체 147.88초), 근거 감사 **94종 모두 통과**. 새 변이 12개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **일반 Form 부착/이벤트 0/최종 소멸·표시·유닛 밀어내기/실패 정리·사제 선택/이동/건설 진행·공간 Pop/삭제·실제 위치 판정/건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`returnpiece` 8/8/8개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[커서 내려놓기 복원](cpp-cursor-drop-reconstruction.md)** 완료. 커서 내려놓기(`004473e0`/CD `0043d660`)의 잠금/법적 위치·반올림·월드/GUI·새 건설/이동·두 연관 조회·품질 캡처·비용·정리를 `RawCursorDrop`으로 복원했다. 실제 로컬 건설 배치·SP 보상·소리 어댑터와 커서→로컬 예측→전체 MayPlace→서버 확정/통지→예측 교체 통합 검사도 연결했다. 새 독립 원본 **2,058개**·누적 **469,068개**, 두 x87 정상 반환 **4,116회**. Release 경고/오류 **0**·CTest 내부 **594개·실패 0**(136.22초, 전체 136.25초), 근거 감사 **93종 모두 통과**. 새 변이 12개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **실제 위치 판정/미리보기·Form/Gump·생산 칸 반환/유닛 밀어내기/실패 정리·사제 이동/건설 진행·공간 Pop/삭제·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`cursordrop` 7/8/8개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[로컬 건설 배치 복원](cpp-construction-local-reconstruction.md)** 완료. 로컬 건설 배치(`004433b0`/CD `004d14a0`)의 미리보기 캡처/삭제·생산 칸 반환 호출 → 본/받침 클라이언트 SID 확보 → 받침/본 abstract 배치 → 사제 이동 호출 → 서버 요청을 `RawConstructionLocal`로 복원했다. 실제 factory/배치 어댑터와 로컬 예측→전체 MayPlace→서버 확정/통지→예측 교체 통합 검사도 연결했다. 새 독립 원본 **1,450개**·누적 **467,010개**, 두 x87 정상 반환 **2,900회**. Release 경고/오류 **0**·CTest 내부 **589개·실패 0**(142.96초), 근거 감사 **92종 모두 통과**. 새 변이 11개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **커서 내려놓기·생산 칸 반환/유닛 밀어내기/실패 정리 하위 효과·실제 사제 선택/이동·건설 진행·공간 Pop/삭제·건설 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructionlocal` 11/12/12개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`): **[서버 건설 요청 복원](cpp-construction-request-reconstruction.md)** 완료. 서버 건설 요청(`00444760`/CD `004d17d0`)의 편집 소유자 보정 → MayPlace → 모든 칸의 abstract 충돌 검사 → 판본별 유닛 밀어내기 → 받침/본 확정 → 실패 정리를 `RawConstructionRequest`로 복원했다. 실제 전체 MayPlace·확정·통지·정리·배치/소유자 지정을 잇는 통합 검사도 연결했다. 세 실제 PE 새 독립 원본 **2,192개**·누적 **465,560개**, 두 x87 정상 반환 **4,384회**. Release 경고/오류 **0**·CTest 내부 **583개·실패 0**(143.80초, 전체 143.84초), 근거 감사 **91종 모두 통과**. 새 변이 11개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **유닛 밀어내기/실패 정리·실제 사제 선택/이동·건설 진행·Pop/삭제 공간 효과·로컬 배치·건설 메뉴/커서 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-11**(`constructionrequest` 26/22/22개 읽기 전용 내보내기).

2026-10-11 앞 단계(`HJOW-Athlon`, 10월 10일 착수): **[건설 배치 통지 처리기 복원](cpp-construction-notice-reconstruction.md)** 완료. 건설 배치 통지 처리기(`004441b0`/CD `004d1d00`)의 정리 → abstract/사제 판정·로컬 배치 이력 → 배치 → 사제 이동 또는 건설 시작/실패 시 환불·삭제 → 갱신 순서를 `RawConstructionNotice`로 복원했다. 실제 확정·정리·배치·환불 어댑터와 예측 아홉 조각 교체 통합 검사를 연결했다. 세 실제 PE 새 독립 원본 **1,611개**·누적 **463,368개**, 두 x87 정상 반환 **3,222회**. Release 경고/오류 **0**·CTest 내부 **577개·실패 0**(147.68초), 근거 감사 **90종 모두 통과**. 새 변이 10개 **모두 검출**(`--focused`, 변이 전 사본은 전체 검사 통과). **실제 사제 이동/건설 진행·삭제 공간 효과·서버 요청·로컬 배치·건설 메뉴/배치 커서 GUI는 후속이며 게임에서 건물을 지을 수는 아직 없다.** 마지막 디컴파일 수행 PC: **HJOW-Athlon, 2026-10-10**(`constructionnotice` 6/5/5개 읽기 전용 내보내기).

2026-10-10 앞 단계(`HJOW-Athlon`): **`constructionclear` 목록 23/19/19개**를 읽기 전용으로 내보냈다. 로컬 예측 조각 정리 전체 몸체와 실제 CanonDecoder·칸 범위·자르기/유효성·절삭/올림·일반 탐색기·방향 글자 조회를 제한 x86으로 실행했다. 세 실제 PE 새 독립 원본 **1,020개**·누적 **461,757개**, 두 x87 정상 반환 **2,040회**, 근거 감사 **89종 통과**. Release 경고/오류 **0**·CTest 내부 **572개·실패 0**(136.53초). 새 변이 9개 **모두 검출**(기대 검사만 돌리는 `--focused` 실행. 변이 전 사본은 전체 검사 통과). 후보의 가상 삭제만 명시 경계다. [주소·순서·관찰 범위](cpp-construction-clear-reconstruction.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`constructionconfirm` 목록 11/11/11개**를 읽기 전용으로 내보냈다. 서버 건설 확정 전체 몸체와 실제 CanonDecoder·스택 확보·메시지 항목 주소 계산을 제한 x86으로 실행했다. 세 실제 PE 새 독립 원본 **1,300개**·누적 **460,737개**, 두 x87 정상 반환 **2,600회**, 근거 감사 **88종 통과**. Release 경고/오류 **0**·CTest 내부 **566개·실패 0**(144.12초). 새 변이 7개 **모두 검출**(기대 검사만 돌리는 `--focused` 실행. 변이 전 사본은 전체 검사 통과). 지을 사제 조회·메시지 생성/전송·SID 생성·통지 처리기·기록기는 명시 경계다. [주소·순서·관찰 범위](cpp-construction-confirm-reconstruction.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`constructionplace` 목록 23/17/17개**를 읽기 전용으로 내보냈다. 건설 배치 실행 전체 몸체와 패치판 비용 부족 처리·환불/취소 통지·SP 조회, 실제 CanonDecoder/HP/단어/프레임 helper를 제한 x86으로 실행했다. 세 실제 PE 새 독립 원본 **3,682개**·누적 **459,437개**, 두 x87 정상 반환 **7,364회**, 근거 감사 **87종 통과**. Release 경고/오류 **0**·CTest 내부 **561개·실패 0**(145.73초). 새 변이 18개 **모두 검출**(첫 실행의 미검출 1개는 검사 빈틈이어서 수명 상수 17·33 원본 입력을 더해 메웠다). SID 수신·SP 저장소·표면 알림·가상 소유자/Pop·문구/메시지는 명시 경계다. [주소·순서·관찰 범위](cpp-construction-place-reconstruction.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`gumpvisual` 목록 2/2/2개**를 읽기 전용으로 내보냈다. 전체 정상 버튼 생성/그리기의 문맥·플래그·선·글자 위치를 제한 x86으로 실행했다. 세 실제 PE 새 독립 원본 **192개**·누적 **455,578개**, 두 x87 생성/그리기 정상 반환 **768회**, 근거 감사 **85종 통과**. Release 경고/오류 **0**·CTest 내부 **550개·실패 0**(148.27초). GUI 기반/자식/부착·사각형 공급·선/글자 래스터는 명시 경계다. [주소·계약·관찰 범위](cpp-gump-visual-reconstruction.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`palettecolors` 목록 2/3/3개**를 읽기 전용으로 내보냈다. 전체 정상 파일 로더·58/50색 검색·기본 9색·날씨 별칭의 새 독립 입력 **24개**, 두 x87 로더 진입/정상 반환 **48회**·색 검색 **2,528회**, 누적 **455,386개**, 근거 감사 **84종 통과**. 파일/할당과 SetPalette 장치 적용은 명시 경계다. [주소·파일 형식·관찰 범위](cpp-palette-colors-reconstruction.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`paletteflash` 목록 6/6/6개**를 읽기 전용으로 내보냈다. 새 독립 원본 **768개**, 두 x87 정밀도의 직접 전체 진입/정상 반환 **3,072회**, 누적 **455,362개**, 근거 감사 **83종 통과**. 생성/초기화/현재 색 읽기/진행/중단은 실제 실행하며 메모리·부모/등록·OS 팔레트·해제는 명시 경계다. CD/10.37의 초기화되지 않은 스택 진행 버그를 기록하고 C++는 목표 10.78 진행을 사용한다. [계약·판본 차이·검증 범위](cpp-palette-flash-reconstruction.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`palettecolor` 목록 2/2/2개**를 읽기 전용으로 내보냈다. 색 검색 전체 몸체의 새 독립 입력 **786개**(두 x87 정밀도 1,572회 정상 반환), 누적 **454,594개**, 최신 감사 **82종 모두 통과**. 팔레트 로더는 정적 판독이다. [복원/연결/검증 범위](cpp-weather-palette-integration.md). **마지막 디컴파일 수행 PC: HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`scenemusic` 목록 10/9/9개**를 읽기 전용으로 내보냈다. 장면별 음악 감독(결과 곡 잠금·곡 요청·다음 곡·프레임 확인·시작·날씨 효과)과 곡 길이 조회·시계 가산·난수·ASCII 비교의 독립 입력 **9,339개**, 최신 감사 **81종 모두 통과**. [복원/검증 범위](cpp-scene-music-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`musicruntime` 목록 3/3/3개**를 읽기 전용으로 내보냈다. 두 채널 초기화/종료·500ms worker의 독립 입력 **4,800개**, 감사 **80종 모두 통과**. 정책/worker/Stop/갱신/Read는 실제 명령이며 커널/COM/파일/잠금만 명시 대체한다. 두 x87 관찰 일치, raw192/출력128·준비/핸들/ID·파일/사건을 비교한다. 직접 11,328회(root 30,528회) 정상 반환, 예상 생성 실패 보고 288회/그 외 assert 0. 실제 Windows 중단은 협조 종료/합류로 바꾸고 실제 worker/임시 파일 검사도 수행했다. 원본 게임/창/실제 오디오 장치 실행 없음. [주소/계약/범위](cpp-music-runtime-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`musicselection` 목록 4/4/4개**를 읽기 전용으로 내보냈다. 상위 음악 옵션/현재 곡·존재 검사·실제 strncpy·특수 곡 loop·demo 재선택의 독립 입력 **2,064개**, 감사 **79종 모두 통과**. 상위 fallback 대체를 제거하고 기존 공개 Play/Stop·실제 헤더까지 실행했다. 두 x87 관찰 일치, raw96/raw256·사건·파일 위치/남은 파일 수를 비교한다. 직접 선택/래퍼 4,128회(root 12,384회) 정상 반환. 실제 게임/장치/스레드 실행 없음. [주소/계약/범위](cpp-music-selection-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`musicopen` 목록 3/2/2개**를 읽기 전용으로 내보냈다. 공개 음악 시작/경로/실제 헤더/곡 길이·생성/되감기의 독립 입력 **1,872개**, 감사 **78종 모두 통과**. 두 x87 제어값의 공개 시작 3,744회·헤더 3,234회(root 11,232회) 정상 반환. C++는 53비트 raw96와 정확히 비교하고 10.78 72개의 64비트 차이(1 ULP)도 보존한다. CD 생성/시작도 실제 공개 몸체에서 검증했다. find/상위 옵션 callback·COM·Winmm만 명시 대체, 실제 장치/게임/스레드 실행 없음. [주소/계약/범위](cpp-music-open-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`musicstream` 목록 2/4/4개**를 읽기 전용으로 내보냈다. 음악 버퍼 생성/채우기와 실제 Read/Stop/helper의 독립 입력 **5,088개**, 최신 감사 **77종 모두 통과**. 두 x87 제어값에서 직접 생성/갱신 10,176회(root 총 30,528회) 정상 반환. CD 생성은 공개 시작 안의 정적 대조이며 생성 입력은 10.78에만 있다. 실제 장치/게임/스레드 실행 없음. [주소/계약/검증 범위](cpp-music-stream-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`musiccontrol` 목록 11/13/13개**를 읽기 전용으로 내보냈다. 음악 채널 읽기/되감기/상태 시작/닫기/정지·음량·공유 음소거와 기존 효과음 제어 몸체의 독립 입력 **5,292개**, 최신 감사 **76종 모두 통과**. 두 x87 제어값에서 스크립트 10,584회·음악 직접 호출 44,244회(root 총 66,132회) 정상 반환. CD 시작은 정적 대조이며 직접 시작 입력은 10.78에만 있다. 실제 장치/게임/스레드 실행 없음. [주소/계약/검증 범위](cpp-music-control-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`soundcontrol` 목록 6/6/6개**를 읽기 전용으로 내보냈다. 소리 재계산·이름 정지/조회/켜기·반복 정지·전체 음량 여섯 몸체와 기존 `soundplay` 호출 몸체의 독립 입력 **3,189개**, 최신 감사 **75종 모두 통과**. 두 x87 제어값에서 스크립트 6,378회·직접 함수 호출 76,458회 정상 반환. 실제 장치/게임 실행 없음. [주소/계약/검증 범위](cpp-sound-control-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`sounddevice` 목록 7/7/7개**를 읽기 전용으로 내보냈다. 장치 초기화/종료·파일 선택/버퍼 적재와 WAVE 열기/읽기/닫기를 확인했다. **실제 WAVE 세 몸체의 독립 입력 1,305개**, 최신 감사 **74종 모두 통과**. DirectSound/COM/파일 검색은 독립 기대값의 범위 밖이며 실제 장치는 실행하지 않았다. [복원/검증 범위](cpp-sound-device-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`soundplay` 목록 19/16/16개**를 읽기 전용으로 내보냈다. 전역/위치 재생·정지·재생 여부·이름 기반 재생과 월드→화면·화면 안 판정·좌우/음량 계산, 이름 표 조회·CRT 정수 변환의 독립 입력 **14,472개**, 최신 감사 **73종 모두 통과**. [복원/검증 범위](cpp-sound-play-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`soundprocess` 목록 10/7/7개**를 읽기 전용으로 내보냈다. 소리 프로세스 생성자/실행/form 삭제 통지·부모 타입 조회·패치 SID 범위 검사와 소리 이름 표 조회/이름 복사/ASCII 비교 본문의 독립 입력 **18,381개**, 최신 감사 **72종 모두 통과**. [복원/검증 범위](cpp-sound-process-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`forcefieldpostpop` 목록 7/6/6개**를 읽기 전용으로 내보냈다. 실제 보호막 전용 후처리/Regular/좌표 타입 helper·일반 finder의 독립 입력 **3,624개**, 최신 감사 **71종 모두 통과**. [공간/프로세스/삭제 연결 범위](cpp-forcefield-lifecycle-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`priestpop` 목록 24/18/18개를 읽기 전용으로 내보냈다.** 실제 사제 가상 표의 공통 Pop/firstPop/Activate→사제 전체 후처리 정상 반환 독립 입력 **3,840개**, 감사 **70종 모두 통과**. [복원/검증/글꼴 범위](cpp-priest-pop-reconstruction.md). 마지막 디컴파일 수행 PC **HJOW-Athlon, 2026-10-10**.

2026-10-10 앞 단계(`HJOW-Athlon`): **`frameadvance` 목록 2/4/4개를 읽기 전용으로 내보냈다.** 실제 진행→지정/단계/글자 표/구판 방향의 독립 입력 **23,040개**, 감사 **69종 모두 통과**. [복원/검증 범위](cpp-frame-advance-reconstruction.md). 기존 아래 목록/날짜는 앞 단계 이력이다.

2026-10-10 후속: **마지막 디컴파일 수행 PC HJOW-Athlon**에서 `carriercheck` 목록 **9/10/10개**를 읽기 전용으로 내보냈다. 실제 +0xcc/+0xc8·낙하 요청/공유 생성자의 독립 입력 **2,028개**, 두 x87 정밀도 총 4,056회 정상 반환을 대조했다. 최신 감사 **68종 모두 통과**. [조건/효과 경계](cpp-carrier-check-reconstruction.md).

2026-10-10 후속: **마지막 디컴파일 수행 PC HJOW-Athlon**에서 `priestfall` 목록 **15/15/15개**를 읽기 전용으로 내보냈다. 낙하 요청·실제 SharedRegular 생성자·0x25b 분배/실제 상태·방향·지도/CRT를 정상 반환까지 실행한 독립 입력 **1,368개**를 생성했다. 두 x87 정밀도 총 2,736회 실행이며 최신 감사 **67종 모두 통과**. [범위/대체 경계](cpp-priest-fall-reconstruction.md).

2026-10-10 후속: **마지막 디컴파일 수행 PC HJOW-Athlon**에서 `priestpostpoptail` 목록 **18/20/20개**를 읽기 전용으로 내보냈다. 전체 사제 postPop wrapper와 실제 접두/상태/지면/프레임의 정상 반환 독립 입력 **6,144개**를 생성했으며 최신 감사 **66종 모두 통과**. 아래 기존 내보내기 날짜는 당시 이력이다.

> **마지막 디컴파일 수행 PC: `HJOW-Athlon`, 2026-10-10** (`VM-W11-CODEX`의 변경을 pull한 뒤 `export_functions.ps1 -All`로 다시 만들었고, 이어 새 `prieststate` 목록 11/12/12개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 `priestregen` 목록 10/11/11개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `priestdestroy` 목록 4/4/4개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `priestforcefield` 목록 12/10/10개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `carrierpredestroy` 목록 7/7/7개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `damageablepredestroy` 목록 9/9/9개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `damageablerelease` 목록 17/15/15개도 2026-10-08 23:58~59에 같은 PC에서 읽기 전용으로 내보냈다. 2026-10-09 자정 뒤 감사 42개 모두 통과했다. 이어 새 `priestspawn` 목록 8/7/7개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `priestplacement` 목록 3/4/4개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `priestpreview` 목록 9/7/7개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `priestcollision` 목록 5/4/4개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `priestgeometry` 목록 10/9/9개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `priestshape` 목록 6/4/4개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `priestterrain` 목록 7/8/8개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `typehotspot` 목록 3/3/3개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canontype` 목록 10/9/9개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonshape` 목록 8/6/6개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canongeometry` 목록 12/11/11개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonplacement` 목록 3/4/4개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonpreview` 목록 9/7/7개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canoncollision` 목록 5/4/4개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonterrain` 목록 7/8/8개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonpermission` 목록 3/2/2개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `playeranchor` 목록 14/15/15개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonsurrounding` 목록 25/21/21개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 새 `canonrelations` 목록 6/2/2개도 같은 PC에서 읽기 전용으로 내보냈다. 이어 `canonmayplace` 56/51/51개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 `outpostlifecycle` 목록 4/2/2개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 `regionownership` 목록 10/8/8개를 같은 PC에서 읽기 전용으로 내보냈다. 이어 KST 2026-10-09 자정 전에 `priestshield` 목록 15/13/13개를 같은 PC에서 읽기 전용으로 내보냈고 2026-10-10 구현/대조를 마쳤다. 직전 감사 **65종 모두 통과**. 일본 유통판 추가로 바뀐 `run_script.ps1` SHA는 기존 destroygraph 1,728개를 재생하고 fixture/관찰/호출 동일성을 확인하여 해당 근거 SHA 하나만 갱신했다. 정밀 디컴파일은 이 PC에서 다시 하지 않았다. 그 앞은 `VM-W11-CODEX`, 2026-10-08.) (AGENTS.md 규칙: 디컴파일 소스를 바꾸면 여기와 [LEFT_JOBS.md](../../LEFT_JOBS.md) 머리말의 호스트명을 갱신한다. 현재 PC가 이 호스트가 아니면 아래 절차로 디컴파일/내보내기를 다시 만든 뒤 작업한다). 그 앞은 `HJOW-Athlon`(2026-10-07~08)이다.

2026-10-07~08 정리. `tools/decomp_*_oracle.py`는 원본 PE의 함수를 Unicorn에서 실행할 때 **Ghidra가 내보낸 함수 몸체 범위**(`functions.tsv`)만 실행을 허용하고, 감사(`--verify`)에서 그 파일과 디컴파일 C(`creation.c`)의 SHA를 확인한다. 이 파일들은 `extracted/`(Git 제외)에 있어 **PC마다 한 번 만들어야 한다.**

## 만드는 방법

```powershell
# 목록이 있는 모든 묶음(약 10분, 읽기 전용 — Ghidra 프로젝트를 바꾸지 않는다)
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -All
# 일부만
powershell -ExecutionPolicy Bypass -File tools/ghidra/export_functions.ps1 -Name bridgedecay,finder
# 모든 감사 실행
Get-ChildItem tools/decomp_*_oracle.py | Where-Object { Select-String -LiteralPath $_.FullName -SimpleMatch "add_argument('--verify'", 'add_argument("--verify"' -Quiet } | ForEach-Object { python -X utf8 $_.FullName --verify }
```

[export_functions.ps1](../../tools/ghidra/export_functions.ps1)은 `tools/ghidra/<이름>-functions.json`의 주소 목록을 판본마다 `extracted/<이름>/<판본>/`에 내보낸다. **목록의 순서가 파일 안의 순서이고 SHA가 그 순서에 의존한다.** 선행 조건은 세 판본의 Ghidra 프로젝트(`tools/ghidra/run_decomp.ps1`)다. 내보내기는 결정적이다 — 같은 목록을 두 번 내보낸 86개 파일이 바이트 단위로 같았고, 다른 PC에서 기록한 SHA와도 일치한다.

| 이름 | 쓰는 도구 | 함수 수(패치 / CD / 10.37) | 목록의 출처 |
|---|---|---|---|
| `sid` | sid·creation 계열·destroy 계열 | 7 / 6 / — | 문서의 명령 (`ExportSid.java`, `sid.c`를 만든다) |
| `creation` | creation·derived 등 | 8 / 13 / — | 문서의 명령 |
| `graphremove` | graphremove·graphlookup·destroy 계열 | 95+2 / 90+2+1 / 90+2+1 | 처음부터 있던 목록(base·geometry·lookup 묶음) |
| `graphrebuild` | graphrebuild·graphrecovery | 5 / 7 / 7 | 문서의 명령 |
| `graphrecovery` | graphrecovery | 6 / 6 / 6 | 문서의 명령 |
| `bridgedecay` | bridgedecay·bridgeeffects·rawfinder | 48 / 34 / 34 | **실행 추적으로 다시 정함**(아래) |
| `bridgeeffects` | bridgeeffects·rawfinder·destroy 계열 | 12 / 7 / 7 | **기록 SHA에 맞춘 순서 복원**(아래) |
| `finder` | rawfinder·destroy 계열 | 8 / 6 / 6 | **기록 SHA에 맞춘 순서 복원**(아래) |
| `destroy`·`destroylifecycle`·`destroygraph` | 같은 이름의 도구 | 9/6/6 · 14/20/20 · 15/17/17 | 처음부터 있던 목록 |
| `reward`·`process` | 같은 이름의 도구 | 27/24/24 · 73/71/71 | 처음부터 있던 목록 |
| `bridgeevent`·`owner` | 같은 이름의 도구 | 8/6/6 · 5/4/4 | 처음부터 있던 목록 |
| `bridgepostpop` | bridgepostpop | 1 / 1 / 1 | 다리 vtable +0x20의 실제 함수 |
| `bridgeconnect` | bridgeconnect·islandlifecycle | 10 / 10 / 10 | 2026-10-08 추가([다리/섬 연결](cpp-bridgeconnect-reconstruction.md)) |
| `setframe` | setframe | 3 / 3 / 3 | 2026-10-08 추가([프레임 지정](cpp-setframe-reconstruction.md)) |
| `pathanimation` | pathanimation | 3 / 3 / 3 | 2026-10-08 추가([방향 조회·도착 프레임 접두](cpp-path-animation-reconstruction.md)), `VM-W11-CODEX`에서 내보냄 |
| `priestowner` | priestowner·owner | 2 / 2 / 2 | 2026-10-08 추가([사제 소유자 재정의](cpp-priest-owner-reconstruction.md)), 공통 owner 내보내기 재사용·`VM-W11-CODEX`에서 내보냄 |
| `priestpostpop` | priestpostpop·owner | 5 / 6 / 6 | 2026-10-08 추가([사제 목록·회복 예약 prefix](cpp-priest-postpop-reconstruction.md)), carrier 진입에서 중단·`VM-W11-CODEX`에서 내보냄 |
| `carrierpostpop` | carrierpostpop·owner | 5 / 5 / 5 | 2026-10-08 추가([Carrier/Damageable 직접 호출](cpp-carrier-postpop-reconstruction.md)), 외부 세 효과 대체·전체 몸체 반환·`VM-W11-CODEX`에서 내보냄 |
| `carriercheck` | carriercheck | 9 / 10 / 10 | Carrier +0xcc/사제 +0xc8·실제 낙하 요청의 최신 읽기 전용 내보내기 |
| `frameadvance` | frameadvance | 2 / 4 / 4 | 공통 프레임 진행·타입/구판 방향 helper의 최신 읽기 전용 내보내기 |
| `priestpop` | priestpop·priestpostpoptail·priestpostpop·owner | 24 / 18 / 18 | 일반 사제 Pop/firstPop/Activate의 실제 가상 분배·전체 wrapper 정상 반환, HJOW-Athlon 2026-10-10 |
| `forcefieldpostpop` | forcefieldpostpop·priestforcefield·owner | 7 / 6 / 6 | 보호막 최초 예약/Regular/실제 좌표 타입 finder·생성자/가상 대응, HJOW-Athlon 2026-10-10 |
| `soundprocess` | soundprocess·owner(실행기만) | 10 / 7 / 7 | 소리 프로세스 생성자/실행/통지·소리 이름 표 조회, 장치·시계·부착 경계 대체, HJOW-Athlon 2026-10-10 |
| `soundplay` | soundplay·owner(실행기만) | 19 / 16 / 16 | 소리 재생 계층(전역/위치 재생·정지·재생 여부·화면 위치 계산), 장치 버퍼 COM 호출·적재·기록 대체, HJOW-Athlon 2026-10-10 |
| `sounddevice` | sounddevice·owner(실행기만) | 7 / 7 / 7 | 장치/파일/WAVE 읽기 코드, WAVE 세 몸체의 native 관찰, HJOW-Athlon 2026-10-10 |
| `musicruntime` | musicruntime·musicstream·musiccontrol·soundcontrol·soundplay·owner(실행기만) | 3 / 3 / 3 | 두 채널 초기화/종료/worker·강제 중단 정책 관찰, HJOW-Athlon 2026-10-10 |
| `scenemusic` | scenemusic·owner(실행기만) | 10 / 9 / 9 | 장면별 음악 감독·곡 길이·시계 가산·난수·ASCII 비교, 선택/시계/월드/화면/설정/효과음 경계 대체, HJOW-Athlon 2026-10-10 |
| `musicselection` | musicselection·musicopen·musicstream·musiccontrol·soundcontrol·soundplay·sounddevice·owner(실행기만) | 4 / 4 / 4 | 상위 옵션/이름/존재/strncpy·실제 demo 재선택, HJOW-Athlon 2026-10-10 |
| `musicopen` | musicopen·musicstream·musiccontrol·soundcontrol·soundplay·sounddevice·owner(실행기만) | 3 / 2 / 2 | 공개 음악 시작/경로/헤더/곡 길이·CD 생성/시작, x87 두 raw 보존, HJOW-Athlon 2026-10-10 |
| `musicstream` | musicstream·musiccontrol·soundcontrol·soundplay·owner(실행기만) | 2 / 4 / 4 | 음악 버퍼 생성/갱신, CD 생성 인라인은 정적 대조, HJOW-Athlon 2026-10-10 |
| `musiccontrol` | musiccontrol·soundcontrol·soundplay·owner(실행기만) | 11 / 13 / 13 | 음악 채널 제어/공유 음소거, CD 시작 인라인은 정적 대조, HJOW-Athlon 2026-10-10 |
| `soundcontrol` | soundcontrol·soundplay·owner(실행기만) | 6 / 6 / 6 | 효과음 제어 여섯 몸체, 호출 몸체는 기존 soundplay, HJOW-Athlon 2026-10-10 |
| `prieststate` | prieststate·owner | 11 / 12 / 12 | 2026-10-08 추가([사제 HP/지면 상태·HP setter](cpp-priest-state-reconstruction.md)), 조회 대체 없음·setter 공간 두 효과 대체·전체 몸체 반환·`HJOW-Athlon`에서 내보냄 |
| `priestregen` | priestregen·prieststate·owner | 10 / 11 / 11 | 2026-10-08 추가([사제 회복 0x25a](cpp-priest-regen-reconstruction.md)), 회복/HP/중립 조건/난수 실제 실행·외부 공간/표시/추적 효과 대체·`HJOW-Athlon`에서 내보냄 |
| `priestdestroy` | priestdestroy·owner | 4 / 4 / 4 | 2026-10-08 추가([사제 삭제 준비](cpp-priest-destroy-reconstruction.md)), 목록 압축/범위 검사 실제 실행·조회/가상 삭제/Carrier 대체·전체 본문 반환·`HJOW-Athlon`에서 내보냄 |
| `priestforcefield` | priestforcefield·owner | 12 / 10 / 10 | 2026-10-08 추가([사제 보호막 실제 조회](cpp-priest-forcefield-reconstruction.md)), 좌표/일반 finder/필터 대체 없이 실행·Pre의 가상 삭제/Carrier만 대체·`HJOW-Athlon`에서 내보냄 |
| `constructionlocal` | constructionlocal·constructionplace·priestgeometry·damageablepredestroy·owner(실행기만) | 11 / 12 / 12 | 2026-10-11 내보내기([로컬 건설 배치](cpp-construction-local-reconstruction.md)), 전체 몸체·decoder/프레임·spot 절삭·메시지 초기화/길이 쓰기·CD 시각 생성 정상 반환·Form/반환/SID/배치/사제/전송 명시 경계·`HJOW-Athlon` |
| `cursordrop` | cursordrop·constructionplace·priestgeometry·damageablepredestroy·owner(실행기만) | 7 / 8 / 8 | 2026-10-11 내보내기([커서 내려놓기](cpp-cursor-drop-reconstruction.md)), 전체 몸체·반올림/스냅·CRT floor/ftol·CD 잠금 helper 정상 반환·Form/Gump/판정/준비/소리/기록/로컬/SP/화면 명시 경계·`HJOW-Athlon` |
| `returnpiece` | returnpiece·priestgeometry·damageablepredestroy·owner(실행기만) | 8 / 8 / 8 | 2026-10-11 내보내기([생산 칸 반환/contained 생성](cpp-cursor-return-piece-reconstruction.md)), 전체 반환/helper·난수/추첨·CD 단어 쓰기·타입 6 factory/생성자/빈 postCreate 실제 명령, 가상 부착/할당/Regular·직접 factory의 SID 할당 명시 경계·`HJOW-Athlon` |
| `constructioneffects` | constructioneffects·constructionclear·constructionplace·priestgeometry·damageablepredestroy·owner(실행기만) | 30 / 26 / 26 | 2026-10-11 내보내기([밀어내기/섬 정리](cpp-construction-effects-reconstruction.md)), 전체 루트·발자국/폭·절삭/거의 올림·일반 finder·spot·현재 genus 실제 명령, 경로/가상 공간/삭제/표시/알림·메시지·보고는 명시 경계·`HJOW-Athlon` |
| `priestlookup` | priestlookup·owner(PE 로더만), 기존 playeranchor 재사용 | 1 / 1 / 1 | 2026-10-11 내보내기([건설 조회 연결](cpp-construction-selection-reconstruction.md)), 소유 사제 조회 전체 실제 명령·대체 함수 없음·기준점/확정/통지 조합은 C++ 통합 검사·`HJOW-Athlon` |
| `constructionbuilder` | constructionbuilder·owner/priestgeometry/returnpiece(공통 로더) | 23 / 20 / 20 | 2026-10-11 내보내기([건설용 생성/이동](cpp-construction-builder-reconstruction.md)), 전체 생성/이동·발자국/중심·반올림/CRT·공급원 소비 실제 명령, 경로/자원 정책/가상 공간/할당 명시 경계·`HJOW-Athlon` |
| `constructionprocess` | constructionprocess·owner/priestgeometry/returnpiece(공통 로더) | 12 / 14 / 14 | 2026-10-11 내보내기([건설 프로세스](cpp-construction-process-reconstruction.md)), 시작/생성자·진행·완료·form 삭제 통지·부모 조회/HP 쓰기/genus·타입 조회/abstract 지정/CRT·CD 기본 시간 helper 실제 명령, 할당/효과/부착/소리/최대 HP/가상 표시·공간/Kill/contained 삭제/밀어내기 명시 경계·`HJOW-Athlon` |
| `constructiontime` | constructiontime·constructionprocess/owner/priestgeometry/returnpiece(공통 로더) | 3 / 9 / 9 | 2026-10-11 내보내기([건설 끝 시각](cpp-construction-time-reconstruction.md)), 재설정 전체·form/부모 getter·CD 진행 계산/CRT/기본 시간/시각 getter·setter 실제 명령, Kernel 조회/최대 HP만 명시 응답 경계·`HJOW-Athlon` |
| `constructionlifetime` | constructionlifetime·process(공통 제한 PE/ABI 로더) | 7 / 13 / 13 | 2026-10-11 읽기 전용 내보내기([표시 등록/삭제 수명](cpp-construction-lifetime-reconstruction.md)), RenderProcess 등록/삭제·건설 공통/파생 소멸·base 소멸 전체 명령, 부착/new/free만 명시 경계·`HJOW-Athlon` |
| `constructionframe` | constructionframe·constructionvisual·process(공통 제한 PE/ABI 로더) | 11 / 11 / 11 | 2026-10-11 읽기 전용 내보내기([건설 표시 프레임](cpp-construction-frame-reconstruction.md)), RunFrame/확장/추가/시각/팔레트/사각형/Draw·실제 난수/CRT 전체 명령, 최대 HP/dirty 큐/blit/Kill만 명시 경계·`HJOW-Athlon` |
| `constructionvisual` | constructionvisual·process(공통 읽기 전용 PE/ABI 로더) | 13 / 13 / 13 | 2026-10-11 읽기 전용 내보내기([건설 표시 초기화](cpp-construction-visual-reconstruction.md)), 생성/버퍼/동기화/초기 입자·실제 난수/CRT 전체 명령, RenderProcess/SHP/팔레트/확보만 명시 경계·`HJOW-Athlon` |
| `contained` | contained·process(공통 읽기 전용 PE/ABI 로더) | 1 / 1 / 1 | 2026-10-11 읽기 전용 내보내기([종속 조회·건설 진행음](cpp-contained-lookup-reconstruction.md)), 타입/순번/권한·삭제 뒤 next 읽기 전체 실제 명령, 가상 삭제 내부만 명시 경계·`HJOW-Athlon` |
| `processform` | processform·process(공통 실제 종속 로더) | 10 / 11 / 11 | 2026-10-11 읽기 전용 내보내기([지정 form/단일 분배](cpp-process-form-reconstruction.md)), Take/생성자/종속 부착·등록/검증/단일 즉시 분배/CRT 시각 변환 실제 명령, Kernel 번호/가상 통지·외부 기록/수명만 경계·`HJOW-Athlon` |
| `constructiondata` | constructiondata·constructionprocess/owner/priestgeometry/returnpiece(공통 로더) | 4 / 4 / 4 | 2026-10-11 내보내기([건설 상태](cpp-construction-data-reconstruction.md)), 쓰기/읽기/수신 생성자·분기/가상 래퍼 전체 실제 명령, 할당/Kernel/지정 form 부착만 응답 경계·`HJOW-Athlon` |
| `constructionrequest` | constructionrequest·constructionclear·constructionplace·priestgeometry·damageablepredestroy·owner(실행기만) | 26 / 22 / 22 | 2026-10-11 내보내기([서버 건설 요청](cpp-construction-request-reconstruction.md)), 요청 전체·decoder·타입 발자국·좌표/spot/표면·일반 finder 정상 반환·MayPlace/밀어내기/확정/실패 정리 명시 경계·`HJOW-Athlon` |
| `constructionnotice` | constructionnotice·priestgeometry·damageablepredestroy·owner(실행기만) | 6 / 5 / 5 | 2026-10-10 내보내기([건설 배치 통지 처리기](cpp-construction-notice-reconstruction.md)), 전체 통지 분기·이력·SID/genus 조회·스냅/CRT 정상 반환·외부 효과 명시 경계·`HJOW-Athlon` |
| `constructionclear` | constructionclear·constructionplace(실행기만) | 23 / 19 / 19 | 2026-10-10 추가([로컬 예측 조각 정리](cpp-construction-clear-reconstruction.md)), 정리 전체 몸체·실제 decoder/칸 범위/탐색기/방향 글자 정상 반환·후보의 가상 삭제만 명시 대체·`HJOW-Athlon` |
| `constructionconfirm` | constructionconfirm·constructionplace(실행기만) | 11 / 11 / 11 | 2026-10-10 추가([서버의 건설 확정](cpp-construction-confirm-reconstruction.md)), 확정 전체 몸체·실제 decoder/스택 확보/항목 주소 정상 반환·사제 조회/메시지 생성·전송/SID 생성/통지 처리기/기록기만 명시 대체·`HJOW-Athlon` |
| `constructionplace` | constructionplace·constructionconfirm·constructionclear·priestgeometry·damageablepredestroy·owner(실행기만) | 23 / 17 / 17 | 2026-10-10 추가([건설 배치 실행](cpp-construction-place-reconstruction.md)), 배치 전체 몸체·패치판 비용 처리·실제 decoder/HP/단어/프레임 helper 정상 반환·SID 수신/SP 저장소/표면 알림/가상 소유자·Pop/문구·메시지만 명시 대체·`HJOW-Athlon` |
| `priestshield` | priestshield | 15 / 13 / 13 | 2026-10-09 내보내기/10-10 완료([사제 보호막 생성](cpp-priest-shield-reconstruction.md)), 전체 wrapper/lookup/finder/공통 owner 정상 반환·자산 생성/Pop·소리/문구만 명시 대체·`HJOW-Athlon` |
| `priestpostpoptail` | priestpostpoptail·priestpostpop·owner | 18 / 20 / 20 | 2026-10-10 추가([사제 postPop 전체 연결](cpp-priest-postpop-tail-reconstruction.md)), 전체 wrapper/접두·이동 불가/지면/프레임 정상 반환·Carrier/낙하/보호막·Regular만 명시 대체·`HJOW-Athlon` |
| `priestfall` | priestfall | 15 / 15 / 15 | 사제 낙하 요청·SharedRegular 생성·0x25b 처리의 최신 읽기 전용 내보내기 |
| `carrierpredestroy` | carrierpredestroy·owner | 7 / 7 / 7 | 2026-10-08 추가([Carrier 삭제 준비·실제 contained 조회](cpp-carrier-predestroy-reconstruction.md)), 직접 조회 대체 0·Carrier의 Damageable/전역 후처리만 대체·전체 본문 반환·`HJOW-Athlon`에서 내보냄 |
| `damageablepredestroy` | damageablepredestroy·owner | 9 / 9 / 9 | 2026-10-08 추가([Damageable 효과/소리 접두·실제 종속 순회](cpp-damageable-predestroy-reconstruction.md)), spot/x87/contained 실제 실행·하위 함수/권한 해방 중간 구간을 명시 대체·`HJOW-Athlon`에서 내보냄 |
| `damageablerelease` | damageablerelease | 17 / 15 / 15 | 2026-10-08 추가([종속 해방 좌표·타입·WORD·생성 요청](cpp-damageable-release-reconstruction.md)), `HJOW-Athlon`에서 내보냄 |
| `priestspawn` | priestspawn | 8 / 7 / 7 | 2026-10-09 추가([사제 나선 생성·Carrier 검사](cpp-priest-spawn-reconstruction.md)), `HJOW-Athlon`에서 내보냄 |
| `priestplacement` | priestplacement | 3 / 4 / 4 | 2026-10-09 추가([사제 배치 초기화·여백/지도 접두](cpp-priest-placement-reconstruction.md)), 모양 진입/후반 중간 구간 명시 대체·`HJOW-Athlon`에서 내보냄 |
| `priestpreview` | priestpreview | 9 / 7 / 7 | 2026-10-09 추가([사제 배치 미리보기·고정 표면/관계](cpp-priest-preview-reconstruction.md)), 미리보기 실제 실행·모양 진입/이후 충돌 구간 대체·`HJOW-Athlon`에서 내보냄 |
| `canonpreview` | canonpreview | 9 / 7 / 7 | 2026-10-09 추가([일반 타입/패턴 로컬 미리보기](cpp-canon-preview-reconstruction.md)), 실제 표면/관계·별도 argument 관찰; 모양/decoder 진입·후반 충돌 명시 대체·`HJOW-Athlon`에서 내보냄 |
| `priestcollision` | priestcollision | 5 / 4 / 4 | 2026-10-09 추가([사제 충돌 후보](cpp-priest-collision-reconstruction.md)), 무시 helper/한 후보 구간 실제 실행·모양/finder/지형/전체 MayPlace 미실행·`HJOW-Athlon`에서 내보냄 |
| `canoncollision` | canoncollision | 5 / 4 / 4 | 2026-10-09 추가([일반 타입 후보 충돌/실제 패턴 연결](cpp-canon-collision-reconstruction.md)), 후보/타입/무시/CRT 실제 실행·중간 구간 종료·전체 MayPlace 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonterrain` | canonterrain | 7 / 8 / 8 | 2026-10-09 추가([일반 타입 지형·지역 누적](cpp-canon-terrain-reconstruction.md)), 초기 권한/후보/모양 종료/지면 누적의 중간 구간·실제 지역 getter·전체 MayPlace/권한 helper/최종 관계 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonpermission` | canonpermission | 3 / 2 / 2 | 2026-10-09 추가([후보 권한·방향 소유 관계](cpp-canon-permission-reconstruction.md)), 실제 helper 전체 정상 반환·Player 조회 진입만 명시 대체·Player 몸체/전체 MayPlace/주변·최종 관계 미실행·`HJOW-Athlon`에서 내보냄 |
| `playeranchor` | playeranchor | 14 / 15 / 15 | 2026-10-09 추가([Player 작업장·그래프 기준점](cpp-player-anchor-reconstruction.md)), 실제 후보 수집/거리/그래프/contained 전체 반환·임시 배열 메모리만 대체·전체 MayPlace/주변·최종 관계 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonsurrounding` | canonsurrounding | 25 / 21 / 21 | 2026-10-09 추가([모양 주변 표면/권한](cpp-canon-surrounding-reconstruction.md)), 실제 flag 8 finder 전체/주변 구간·Player 진입만 명시 대체·전체 MayPlace/최종 관계 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonrelations` | canonrelations | 6 / 2 / 2 | 2026-10-09 추가([최종 표면 관계/특수 지역/거부](cpp-canon-relations-reconstruction.md)), 최종 구간/실제 helper/성공·실패 에필로그 정상 반환·함수 대체 없음·전체 MayPlace 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonmayplace` | canonmayplace | 56 / 51 / 51 | 2026-10-09 추가([전체 MayPlace 연결](cpp-canon-mayplace-reconstruction.md)), 진입부터 정상 반환·실제 helper·임시 160바이트 확보/반납만 대체·`HJOW-Athlon` 내보내기 |
| `outpostlifecycle` | outpostlifecycle | 4 / 2 / 2 | 2026-10-09 추가([outpost 두 목록 접두](cpp-outpost-lifecycle-reconstruction.md)), wrapper/Array 정상 반환·지역/부모 몸체 명시 대체·`HJOW-Athlon` 내보내기 |
| `regionownership` | regionownership | 10 / 8 / 8 | 2026-10-09 추가([지역 소유 투표](cpp-region-ownership-reconstruction.md)), 전체 wrapper/재귀/getter/clear/생성자 정상 반환·지형 flood/도장 및 옵션 조회만 대체·`HJOW-Athlon` 내보내기 |
| `priestgeometry` | priestgeometry | 10 / 9 / 9 | 2026-10-09 추가([사제 비패턴 CanonDecoder/범위](cpp-priest-geometry-reconstruction.md)), 생성/진행/정수 범위/한 모양 finder 인자 실제 계산·finder/전체 MayPlace/지형 미실행·`HJOW-Athlon`에서 내보냄 |
| `priestshape` | priestshape | 6 / 4 / 4 | 2026-10-09 추가([사제 비패턴 픽셀/SHP 조회](cpp-priest-shape-reconstruction.md)), 전체 getter·decoder·패치 frameCheck/SHP helper 정상 반환·실제 주소 표 조회·자산 로더/전체 MayPlace/지형/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `priestterrain` | priestterrain | 7 / 8 / 8 | 2026-10-09 추가([일반 사제 다리·섬·지역 효과](cpp-priest-terrain-reconstruction.md)), 실제 후보/모양 종료/최종 구간·지역 getter 정상 반환·finder/전체 MayPlace/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `typehotspot` | typehotspot·cpp_priestassets_smoke | 3 / 3 / 3 | 2026-10-09 추가([타입 기준점/실제 자산 연결](cpp-priest-assets-reconstruction.md)), 숫자 속성 구간/실제 CRT ftol·타입 초기화 근거·전체 로더/게임/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `canontype` | canontype·cpp_canontype_smoke | 10 / 9 / 9 | 2026-10-09 추가([타입별 패턴 선택/전체 순회](cpp-canon-type-reconstruction.md)), 생성/Advance/셀/검색/방향/범위 helper 대체 없이 정상 반환·전체 MayPlace/게임/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonshape` | canonshape·cpp_canonshape_smoke | 8 / 6 / 6 | 2026-10-09 추가([일반 자산/패턴 전체 픽셀 범위](cpp-canon-shape-reconstruction.md)), 전체 getter·ctor/Advance·셀/검색·패치 SHP helper 대체 없이 정상 반환·실제 자산 전체 대조·전체 MayPlace/게임/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `canonplacement` | canonplacement·cpp_canonplacement_smoke | 3 / 4 / 4 | 2026-10-09 추가([일반 타입/패턴 배치 접두](cpp-canon-placement-reconstruction.md)), 별도 argument·진입 초기화/허용/여백/지도/반환 실제 실행·모양 진입/후반 정책 명시 대체·실제 두 판본 자산 접두 대조·전체 MayPlace/게임/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `canongeometry` | canongeometry·cpp_canongeometry_smoke | 12 / 11 / 11 | 2026-10-09 추가([일반/패턴 배치 모양/finder 인자](cpp-canon-geometry-reconstruction.md)), 전체 decoder/Bounds·모양별 snap/CRT/사각형 실제 실행·finder 진입에서 관찰 종료·전체 MayPlace/후보/지형/게임/OS 미실행·`HJOW-Athlon`에서 내보냄 |
| `islandlifecycle` | islandlifecycle | 4 / 2 / 2 | 2026-10-08 추가([섬 삭제 훅](cpp-islandlifecycle-reconstruction.md)) |
| `islandpostpop` | islandpostpop | 5 / 4 / 4 | 2026-10-08 추가([noIsland 최초 등록·받침 소유자](cpp-islandpostpop-reconstruction.md)), `VM-W11-CODEX`에서 내보냄 |
| `regiongraph` | regiongraph | 13+3 / 14+1 / 14+1 | **증거 JSON의 `function_ranges` 순서에서 복원**(아래 "남은 문제") |
| `pop` | pop·display·postpop·rawgraph·regiongraph | 22(+보조 2) / 18 / — | 문서의 명령. 패치판 보조 둘은 `extracted/pop/helpers-originals`(목록의 `aliases`) |
| `display` | display 계열 | 10 / 7 / — | 증거 JSON의 누적 `function_ranges`에서 앞 단계 몫을 뺀 나머지 |
| `postpop` | postpop 계열 | 10 / 13 / — | 같은 방법 |
| `graph` | graph·rawgraph 계열 | 13 / 16 / — | 증거 JSON의 `function_ranges` 그대로 |
| `lifecycle` | unpop | 22 / 17 / — | 문서의 명령(`decomp_unpop_oracle.py`가 `extracted/lifecycle`에서 읽는다) |

2026-10-08 후속의 `neighbor`는 새 내보내기 없이 `bridgeevent`·`graphremove`/`geometry`를 재사용한다. `bridgepostpop`의 접두 대조 240개와 함께 [첫 연결 이웃/끝 칸 통합 근거](cpp-neighbor-reconstruction.md)에 정리했다. 두 새 도구를 포함해 **감사 27개가 모두 통과**했다. `--verify` 옵션이 없는 옛 `config`·`graphics`·`options` 생성기는 위 감사 명령에서 제외한다.

`derived`만 아직 목록 파일이 없다(주소를 스크립트로 계산하는 명령이 [그 문서](cpp-derived-reconstruction.md)에 있다). `pop`·`display`·`postpop`·`graph`·`lifecycle`의 `--verify`는 저장소 안의 파일만 확인하므로 내보내기가 없는 PC에서도 통과하지만, **기대값을 다시 생성하려면** 그 내보내기가 있어야 한다. 2026-10-08에 이 다섯 묶음의 목록을 저장소에 넣었다 — 새로 내보낸 `functions.tsv`의 (진입 주소, 범위)가 **두 판본 모두 각 증거 JSON에 기록된 것과 순서까지 같음**을 확인했다(`rawgraph`는 여기에 더해 정밀 디컴파일의 `extracted/refined/<판본>/functions.tsv`를 읽는다 — `tools/ghidra/refine_all.ps1`, 이 PC에서 약 20분).

## 2026-10-08 `VM-W11-CODEX`에서 처음 준비한 결과

새로 받은 PC에서 위 절차를 그대로 따랐다: `run_decomp.ps1`로 만든 세 판본의 기본 Ghidra 프로젝트(10.78 4,506개·CD/10.37 각 3,711개 함수) → `export_functions.ps1 -All` → CRLF로 남아 있던 도구/증거/스크립트 사본 21개를 지우고 `git checkout`으로 다시 받음 → 감사. 처음에 **27개 가운데 26개가 통과**했고, 이 날 더한 세 도구(`bridgeconnect`·`setframe`·`islandlifecycle`)까지 **30개 가운데 29개가 통과**한다. 내보내기는 이 PC에서도 다른 PC가 기록한 SHA와 일치했다.

알아 둘 것:

- Python 의존성은 `extracted/oracle-python`에 격리 설치한다. 요구사항 파일에 한글 주석이 있어 한국어 Windows에서는 `PYTHONUTF8=1`(또는 `python -X utf8 -m pip ...`)이 필요하다.
- **Ghidra 헤드리스는 같은 설정 폴더(`extracted/ghidra-settings`)로 동시에 두 개를 돌릴 수 없다.** 둘째 실행이 번들 캐시 잠금(`Unable to create bundle cache lock file`)으로 스크립트를 싣지 못하고 멈춘다. `refine_all.ps1`이 도는 동안에는 내보내기를 하지 않는다.
- `powershell -File export_functions.ps1 -Name a,b`처럼 부르면 쉼표 목록이 한 문자열로 넘어와 실패했다. 스크립트가 쉼표를 직접 나누도록 고쳤다.

### 해결한 문제: `regiongraph` (2026-10-08, VM-W11-CODEX)

후속 세션에서 위 준비된 내보내기로 `decomp_regiongraph_oracle.py`를 실제로 재실행했다. **1,536개 fixture는 파일 전체가 바이트 단위로 그대로**이며 `--verify`도 통과한다. 증거 JSON의 디컴파일 C SHA와 누적 함수 경계 메타데이터(CD/10.37 helper 구분)를 현재 PC의 판독에 맞췄다. 새 noIsland 도구를 포함해 감사 **31개 모두 통과**한다. 아래 문장은 재생성 전의 원인과 절차 기록이다.

`regiongraph`는 목록이 없어 이 PC에서 감사가 실패했다. 증거 JSON의 `function_ranges`가 내보낸 순서 그대로 남아 있어 [목록 파일](../../tools/ghidra/regiongraph-functions.json)로 복원했다(CD/10.37의 InsertCD 비교 함수 `00433bf0`의 위치는 `functions.tsv` SHA로 확정 — 맨 끝). 결과:

- `functions.tsv`(실행 허용 범위)는 **세 판본·두 묶음 모두 기록 SHA와 일치**한다.
- `creation.c`(디컴파일 텍스트)는 **10.37만 일치**하고 10.78·CD는 다르다. 기록은 다른 PC의 예전 Ghidra 프로젝트 상태에서 만든 것으로 보인다(앞서 `sid`·`creation`에서 같은 현상이 있었다). 텍스트만 다르고 실행 범위는 같다.

당시 이 PC의 `decomp_regiongraph_oracle.py --verify`는 `creation.c` SHA에서 멈췄다. 앞 단계의 `pop`·`display`·`postpop`·`graph` 내보내기(위 목록)와 `extracted/refined/`가 준비돼 있었고 후속 세션에서 아래 절차로 재생성해 해결했다.

```powershell
python -X utf8 tools/decomp_regiongraph_oracle.py            # 기대값 재생성(소요 시간 미측정)
git status --short cpppj/tests/fixtures/regiongraph-x86.tsv   # 바뀌지 않아야 한다(바뀌면 증거를 되돌리고 원인을 먼저 본다)
python -X utf8 tools/decomp_regiongraph_oracle.py --verify
```

fixture가 바이트 단위로 그대로이고 증거 JSON의 SHA 항목만 바뀌면 30개 감사가 모두 통과한다. 같은 방법으로 `pop`·`display`·`postpop`·`graph`·`rawgraph` 도구의 재생성도 이 PC에서 확인할 수 있다(하지 않았다).

## 2026-10-07~08에 고친 것 (`HJOW-Athlon`)

처음에는 이 PC에서 24개 감사 가운데 11개가 실패했다. 원인은 세 가지였다.

1. **목록이 저장소에 없던 묶음.** `bridgedecay`·`bridgeeffects`·`finder`·`graphrebuild`·`graphrecovery`·`sid`·`creation`의 주소 목록이 문서의 명령이나 다른 PC의 로그에만 있었다. 모두 목록 파일로 만들었다.
   - `bridgeeffects`·`finder`: 진입점에서 직접 호출을 따라간 폐포(`finder`는 여기에 간접 호출되는 기본 true 필터 하나를 더함)가 문서의 함수 수와 같았다. **저장 순서는 기록된 `functions.tsv`의 SHA와 맞는 순열을 찾아 확정했다**(CD 7개·6개는 전수, 패치 12개는 앞 5개를 도구의 진입점 순서로 두고 나머지 7개 전수). 다시 내보낸 `creation.c`의 SHA까지 기록과 같다.
   - `bridgedecay`: 처음 기록(패치 51·CD 35개)의 목록은 어디에도 남아 있지 않았고 SHA에 맞는 구성을 찾지 못했다. 폐포(68/49개)는 실행되지 않는 함수까지 포함한다. 그래서 **기대값 생성 전체를 실제로 돌려 실행된 함수만 추렸다**(대체 진입점 함수 제외, 진입점 먼저 + 첫 실행 순서): 패치 48·CD/10.37 각 34개. 이 목록만으로 다시 생성한 `bridgedecay-x86.tsv`는 저장소의 것과 **바이트 단위로 같다**(입력 14,524개). 처음 기록과 3개·1개가 다른 이유는 확인하지 못했다(실행되지 않는 함수가 들어 있었을 가능성).
2. **줄바꿈에 좌우되던 SHA.** `decomp_bridgeeffects_oracle.py`·`decomp_rawfinder_oracle.py`가 줄바꿈을 지정하지 않고 fixture/증거를 써서 Windows에서 CRLF 파일의 SHA를 기록했고(저장소에는 LF로 들어간다), `decomp_rawgraph_oracle.py`는 `eol` 고정이 없어 CRLF 작업 사본의 SHA가 다른 증거들에 기록돼 있었다. 두 도구가 LF로 쓰도록 고치고, `.gitattributes`에 `tools/decomp_*.py`·`tools/ghidra/*.java`·`tools/ghidra/*-functions.json`·`cpppj/recovery-*.json`을 `eol=lf`로, `*.ps1`을 `eol=crlf`로 고정했다.
3. **이 PC의 오래된 사본.** `extracted/sid/originalCD`·`extracted/creation/originalCD`가 예전 Ghidra 프로젝트 상태에서 만든 것이었다(잘못된 주소 한 줄, 함수 하나 더). 새로 내보낸 파일은 다른 PC가 기록한 SHA와 일치한다.

그 뒤 영향을 받는 11개 도구(`bridgeevent`·`bridgedecay`·`bridgeeffects`·`rawfinder`·`graphremove`·`graphlookup`·`graphrebuild`·`graphrecovery`·`destroy`·`destroylifecycle`·`destroygraph`)를 이 PC에서 **다시 실행해 증거를 다시 기록했다. 모든 fixture는 바이트 단위로 그대로였다**(증거 JSON의 SHA 항목만 바뀜). 지금은 새로 더한 `owner`까지 25개 감사가 모두 통과한다.

## 다른 PC에서 할 일

- 이 변경을 받은 뒤 `export_functions.ps1 -All`로 내보내기를 다시 만든다. 특히 `bridgedecay`는 목록이 바뀌었으므로(51/35 → 48/34) 예전 내보내기로는 `--verify`가 실패한다.
- `.gitattributes`가 바뀌었으므로 CRLF로 체크아웃돼 있던 도구(`tools/decomp_rawgraph_oracle.py` 등)는 작업 사본을 지운 뒤 `git checkout -- <파일>`로 다시 받아야 LF가 되어 SHA가 맞는다.
- `extracted/creation/<판본>/sid.c`처럼 목록에 없는 파일이 남아 있어도 감사에는 영향이 없다.
