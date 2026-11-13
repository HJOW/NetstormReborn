# webpj 생산 칸 조각 반환 (WB04-E399, 2026-11-14)

6단계 커서 선행 목록의 `client/RawCursorReturnPiece.cpp/.h`(기준선 `1f00a25a`, 111줄)를 `core/raw-cursor-return-piece.js`로 옮겼다. 기준선 뒤 소스와 `ReturnPieceTests.cpp` 변경은 없다. E398에서 미룬 `CursorDropTests.ReturnPiece_ConnectsGuiDrop`도 이번에 실제 생성기와 연결해 마쳤다.

## 구현

- `CursorReturnPieceState`는 현재 TYPE·다리 TYPE82·생산 칸 유지 옵션·요새 모드·시간 옵션 BYTE를 공유한다.
- `returnPiece(flags, source, quality)`는 부모 검사 후 `0x100` 비트를 해제한다. 다리는 게임 난수 한 번과 기존 `selectBridgePattern`으로 패턴을 골라 품질0으로 contained를 생성하고 타이머 옵션은 읽지 않는다.
- 일반 조각은 현재 유지 옵션이 0이면 `0x100`을 더한다. 품질은 DWORD 감소 → signed 음수면0으로 제한 → 기존 `RawContainedCreate`의 BYTE 쓰기다. 생성 뒤 유지 옵션을 다시 읽어 0일 때만 40바이트 타이머 할당을 요청한다. 성공 뒤 현재 요새/시간 옵션을 읽어 10·5·1초 또는 원문 float 비트 `0x38d1b717`을 Regular payload로 준다.
- 실제 설정 getter `productTimers`·`fortMode`·`unitRate`를 선택하면 같은 위치에서 호출한다. 요새 경로는 일반 속도 BYTE를 읽지 않는다. 독립 시험은 기존 상태 값을 쓴다.
- `makeCursorReturnPieceHooks`는 같은 풀의 `SquidProcessHost.addRegular`에 연결한다. `makeReturnPieceHooks`는 로컬 건설과 GUI 내려놓기가 공통 반환 몸체를 쓰도록 새 훅 묶음을 만든다. 호출자 훅·할당 실패 경계는 보존한다.

## 검증

`tests/cursor-return-piece.test.mjs` **5건, 첫 실행 전부 통과**:

1. `returnpiece-x86.tsv` originals **633행**(Return528·Create101·Factory4): 저장 관측의 호출/인수·전체50바이트·최종 전역8개·게임 난수와 일치했다. Create/Factory 행은 이미 이식된 실제 `RawContainedCreate`·`SquidFactory`를 재대조한다. CD/10.37은 제외한다.
2. 타이머 경계·부모·시간 번호·다른 풀 거부와 실패 시 앞선 생성/할당 효과 보존.
3. 유지 옵션의 생성 전/후 재조회, 할당 뒤 모드·속도 재조회, 요새의 속도 읽기 생략.
4. cpppj `ReturnPiece_ConnectsFactoryAndRegular`: 실제 Factory·contained·ProcessForm·Host·Kernel을 같은 풀에 구성했다. 네 payload·시각12.5·count0·부모를 확인하고, 1초 예약을 실행해 시각13.5·count1과 시각13.0에서의 재실행 생략을 확인했다. 가상 Form 부착/이벤트는 cpppj 검사처럼 명시 장치다.
5. cpppj `ReturnPiece_ConnectsGuiDrop`: 실제 `RawCursorDrop`→반환 어댑터→`RawContainedCreate`→Factory 사슬이 TYPE6의 새 생산 칸을 만들고, 기존 TYPE100·플래그8·품질3→2를 보존했다. detach 뒤 미리보기의 현재 플래그 `0x100`·품질 `0xfe`와 분리되며 난수0은 소비하지 않았다. Gump 화면 효과는 명시 장치다.

원본/클론 게임·브라우저·실제 청취는 실행하지 않았다. fixture는 수정하지 않았고 정규화 SHA-256은 `675c8e23013dd8d09b5b9943b58aa3f278d9da6d18f7c29eebd13ff6688defcd`(파일1900행)이며 manifest `returnpiece`에도 기록했다. 대응표 오류0, port646·partial60·todo190이다. Node v26.13.0·v22.23.3의 전체 검사도 각각 **3855/3855 통과·실패/건너뜀0**(342.2초·377.3초, 종료 코드0)이다. Node v24는 미설치다.

## 다음

`RawCursorLifecycle`의 취득/취소/미리보기 정리와 자산/글자 측정 바인딩을 옮긴다. 커서 전역·선택·실제 입력·음향·미리보기 표시·Session 연결은 아직 완료하지 않았다. 이번 부품 통합을 실제 미션 플레이나 L2/L4a 판정으로 세지 않는다.
