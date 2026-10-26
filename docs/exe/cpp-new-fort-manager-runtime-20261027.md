# NewFort 부모와 실제 manager 장치 — 단위470

2026-10-27 / HJOW-Athlon에서 세 NewFort 부모를 제품 모듈에 채택하고 manager 생성에 실제 저장소·작업본 파일 장치를 연결했다. 마지막 전체 정밀 디컴파일은 **HJOW-Athlon / 2026-10-23**으로 유지하며 산출물을 변경하지 않았다. [실행 보고서](../../cpppj/new-fort-manager-runtime-20261027.json)에 최종 소스·실행 파일·원시 로그의 SHA와 검사 범위를 기록했다.

## 구현

`RawNewFort`는 [준비본](prepared/new-fort-469-20261027/README.md)의 `004bfba0` manager 생성, `004b3920` 신규 자료 작성, `004b5ca0` 파일 부재 분기를 채택했다. manager 헤더의 부분 쓰기, callback 뒤 현재 owner 재조회, 캡처한 ID와 물리 이름, Core10BYTE clear, seed의 현재 상위WORD 보존과 lowWORD0 재시도, State 종류·Notify·alternate 확정 순서를 보존한다.

`CreateNewEditorFortManager`는 단위469의 `CampaignTemplateStorage`와 실제 `EditorFiles`에 manager 부모를 연결한다. 존재 검사 → 필요하면 임시276BYTE manager 생성 → 그 manager의 현재 이름으로 작업 파일 삭제 → 소멸/free → 최종 manager 할당·생성 → 같은20BYTE 헤더 공개 → 실제35개 섹션 초기화 순서다. `Install`이나 빈 `.fort` 읽기로 이 경로를 대신하지 않는다. 상대 파일명도 존재 검사와 같은 작업 namespace에서 삭제하며, manager에 저장하는 원문 이름은 보존한다.

이미 열린 manager의 진단을 앞선 `Clear`로 피하지 않는다. 보호 경로가 존재 검사에서 거부되면 그 전에 쓴 header+4와+16은 남고 파일·할당·이름표 효과는 발생하지 않는다. 필요한 진단 장치가 없으면 선택 위치에서 중단한다.

초기 자료와 파일 부재 부모는 아직 실제 Session 설치 경로에 연결하지 않았다. 이 단위의 manager 장치도 NewEditor 설치 부모가 호출하도록 이어야 한다. 새 맵 메뉴·GUI 완료로 처리하지 않는다.

## 원본 근거와 검증

- 준비 도구를 실제 다시 실행하여 최초 Native **1,400행**(manager48·data1248·missing26·bit0 보강78)과 바이트 일치를 확인했다. 네 fixture를 새 제품 경로에 그대로 복사했다. 큰 외부 하위 장치의 한계는 준비본 README와 보고서를 유지한다.
- Release 빌드2회 모두 종료0·경고/오류0이다. 첫 빌드에서 신규8검사가 통과했고, 상대 경로의 실제 파일 삭제 대조를 추가해 두 번째 빌드와 최종 신규9검사를 마쳤다.
- 최종 집중 **88개/실패0**이다. 원문1400행 재생4검사, 실제 manager/작업 파일5검사와 기존 DataManager·Template·EditorFiles·Subscriber 회귀를 포함한다. 파일 부재, 존재 파일의 임시 수명, 상대 이름, 보호 원본 거부의 부분 효과, 기존 manager 진단 시 선행 변경 부재를 확인했다.
- 기존3,219등록을 보존하고 최종3,228등록이다. 기존 검사 본문·fixture와 원본/CD/AGENTS의 최초3,154파일이 바이트동일이다. CMake 등록 외 기존 검사 파일을 바꾸지 않았다.
- 게임 SHA256은 `7414a4157a22090f3765bec6f6db24ccbd47813674cd3ba55c1215f857636f62`다. 실제 게임·GUI·전체CTest는 실행하지 않았다. 전체 통합 검사의 마지막 기준은468의3,211개/실패0이며 등록 수와 구별한다.

원시 로그는 `extracted/cpp-new-fort-470/`, 이번 목표 재개 중 앞서 실행한 Native 재검증은 `extracted/cpp-new-map-469/native-new-fort{,-bits}.*`에 있다. 원시 보고서와 보관 SHA를 바꾸지 않았다. 결과 보고서가 Git 제외 원시 경로·해시를 함께 남긴다.

## 조사 문서 정정과 다음 연결

기존 [후속 조사](cpp-new-fort-next-20261026.md)의 두 해석을 정정했다. `0042b200`은 기술/슬롯 초기화가 아니라 현재 CoreData의10BYTE clear이며, `004beec0`은 Deck 초기 저장이 아니라 Money 섹션 저장이다. 원문 NewData의 Money writer는 현재 SP가 아니라 같은 게임 난수 `Next(1010)+333.0f`를 float32로 저장한다.

다음은 같은 Client의 Subscriber/handle·SP·Core·난수·Territory/Money/State/Notify를 NewData와 파일 부재 부모에 연결하는 것이다. 이후 NewEditor의 alternate1 월드 설치·공통 State 확정·자동 Add Island 큐를 연결한다. 섬 추가 전용 MetaImage/Bitmap/목록/창 수명·Test Battle/시나리오와 전체 미션 자연 완주/원본 동일성은 계속 남는다. CD 10.72 대응 주소는 기존 조사 문서에 유지했으며 이번 채택·실행 기준은10.78이다.
