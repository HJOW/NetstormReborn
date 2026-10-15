namespace Netstorm.Core.Simulation;

/// <summary>
/// Early Mission 1(Bridge the Gap!)의 단계 판정이 읽는 현재 전역 값. 호출마다 현재 값을 읽으므로 훅이 단계를 바꾸거나
/// 값이 갱신돼도 같은 인스턴스에서 다음 읽기에 반영된다.
/// </summary>
public sealed class TutorialOneState
{
    /// <summary>현재 단계 글자(전역 005ca8e8, 'A'~'F')를 돌려주는 조회. 미션 게이트와 같은 저장소를 읽는다.</summary>
    public Func<int>? Stage { get; init; }

    /// <summary>현재 로컬 플레이어 번호(00540c70). 신전 조회·SP 조회·간헐천 연결 비교에 쓴다.</summary>
    public uint LocalPlayer { get; set; } = 1;

    /// <summary>SP 조회 허용 전역(00595344). 0이면 단계 E·F 의 SP 조건을 보지 않는다.</summary>
    public uint BattleShown { get; set; }

    /// <summary>F4(섬으로 복귀) 명령 토큰(005429ac). 단계 A 가 현재 사건의 하위 26비트와 비교한다.</summary>
    public uint HomeCommand { get; set; }

    /// <summary>회수 명령 토큰(0054271c). 단계 A 가 같은 입력과 비교해 일치하면 사건을 소비한 것으로 1을 돌려준다.</summary>
    public uint ReclaimCommand { get; set; }
}

/// <summary>
/// 단계 판정에 필요한 실제 게임 저장소와 효과의 연결점. 모든 항목이 필요하며 비어 있으면 생성하지 못한다(빈 성공 금지).
/// </summary>
public sealed class TutorialOneHooks
{
    /// <summary>004c2500(마스크): 플레이어의 누적 제작 수를 플래그2 부류로 합산한다(<see cref="TotalMade.BuiltForGenus"/>).</summary>
    public Func<uint, int>? BuiltForGenus { get; init; }

    /// <summary>00462bc0(SID): 현재 신전/간헐천 객체가 속한 표면 Graph 번호. 0/254 도 그대로 비교한다.</summary>
    public Func<uint, uint>? GraphFor { get; init; }

    /// <summary>0040ec50(플레이어): 현재 플레이어 SP 의 단정도(float32) 값.</summary>
    public Func<uint, float>? Money { get; init; }

    /// <summary>004c33f0: 정지/잠금을 확인한 뒤 공유 단계를 증가시킨다. 같은 판정 안에서 여러 번 불릴 수 있다.</summary>
    public Action? Advance { get; init; }

    /// <summary>00486360: 단계 C 의 조건에서 Advance 의 성공 여부와 무관하게 실행하는 간헐천 생성.</summary>
    public Action? SpawnGeysers { get; init; }

    /// <summary>플레이어의 현재 신전 SID(Player+0x7c)를 돌려준다. 간헐천 순회 전에 한 번만 읽는다.</summary>
    public Func<uint, uint>? TempleOf { get; init; }
}

/// <summary>
/// 00595094/98 의 "최근 생성한 간헐천" 목록(최대 5개). 전체 간헐천 장부가 아니다. 판정이 매 반복마다 현재 개수를 다시 읽으므로
/// 훅이 목록을 줄여도 다음 반복에 반영된다. 개수는 원본처럼 부호 있는 DWORD 로 해석한다.
/// </summary>
public sealed class RecentGeyserList
{
    /// <summary>미리 확보한 항목 배열(개수보다 길 수 있다).</summary>
    public List<uint> Entries { get; } = [];

    /// <summary>현재 유효한 항목 수(부호 있는 DWORD 로 해석한다).</summary>
    public uint Count { get; set; }
}

/// <summary>
/// 원본 10.78 004c3a20 의 Early Mission 1 단계 판정 전체 분기(A~F). 단락 평가·같은 판정 안의 중복 Advance·SP 비교 순서를 유지하며
/// 단계 증가는 훅(공통 미션 게이트의 Advance)에 맡긴다.
/// </summary>
/// <remarks>
/// 단계별 조건: A 는 현재 사건이 F4 명령이거나 다리 부류(4)의 누적 제작 수가 0보다 크면 Advance, B 는 7보다 크면, C 는 18보다 크면
/// Advance 뒤 간헐천 생성(Advance 가 거부돼도 생성은 실행), D 는 신전의 Graph 와 최근 간헐천 각각의 Graph 가 같을 때마다 Advance,
/// E·F 는 SP 조회가 허용되고 float32 SP 가 각각 200·600 이상이면 Advance. 원본 10.78 실제 기계어 266개 관찰과 호출 순서까지 대조한다
/// (cpppj/tests/fixtures/tutorial-one-x86.tsv Stage 행, X86TutorialOneTests). 근거: docs/exe/cpp-tutorial-one-20261015.md.
/// 2026-10-15 감사 D05 에서 cpppj RawTutorialOne 을 옮겨 추가.
/// </remarks>
public sealed class TutorialOne
{
    /// <summary>원본이 사건에서 modifier/release 상위 비트를 지우는 마스크(하위 26비트만 명령).</summary>
    private const uint CommandMask = 0x03ffffff;

    /// <summary>제작 수를 합산하는 다리 부류(플래그2 비트 4).</summary>
    private const uint BridgeGenus = 4;

    /// <summary>단계 B 의 통과 기준: 누적 다리 칸 수가 이 값보다 커야 한다(0x07).</summary>
    private const int FirstBridgeThreshold = 7;

    /// <summary>단계 C 의 통과 기준: 누적 다리 칸 수가 이 값보다 커야 한다(0x12).</summary>
    private const int SecondBridgeThreshold = 18;

    /// <summary>단계 E 의 SP 기준(float32 200).</summary>
    private const float FirstMoneyThreshold = 200.0F;

    /// <summary>단계 F 의 SP 기준(float32 600).</summary>
    private const float SecondMoneyThreshold = 600.0F;

    /// <summary>플레이어 기록 개수(범위 검사용).</summary>
    private const uint PlayerCount = 9;

    /// <summary>현재 전역 값.</summary>
    private readonly TutorialOneState _state;

    /// <summary>최근 간헐천 목록(호출마다 현재 개수와 항목을 다시 읽는다).</summary>
    private readonly RecentGeyserList _geysers;

    /// <summary>실제 집계·Graph·SP·단계·생성 경계.</summary>
    private readonly TutorialOneHooks _hooks;

    /// <summary>
    /// 상태·목록·경계를 묶는다. 경계가 하나라도 비어 있으면 거부한다.
    /// </summary>
    /// <param name="state">현재 전역 값(공유).</param>
    /// <param name="geysers">최근 생성 간헐천 목록(공유).</param>
    /// <param name="hooks">집계·Graph·SP·단계·생성·신전 조회 경계.</param>
    public TutorialOne(TutorialOneState state, RecentGeyserList geysers, TutorialOneHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(geysers);
        ArgumentNullException.ThrowIfNull(hooks);
        if (state.Stage == null || hooks.BuiltForGenus == null || hooks.GraphFor == null || hooks.Money == null
            || hooks.Advance == null || hooks.SpawnGeysers == null || hooks.TempleOf == null)
            throw new ArgumentException("튜토리얼 1 의 실제 단계/집계/Graph/SP/생성 경계가 누락되었습니다.", nameof(hooks));
        _state = state;
        _geysers = geysers;
        _hooks = hooks;
    }

    /// <summary>
    /// 현재 단계의 조건을 판정한다. 같은 프레임에 단계가 바뀌더라도 원본 switch 가 처음 선택한 한 분기만 실행한다.
    /// </summary>
    /// <param name="currentEvent">현재 처리 중인 사건 값(하위 26비트가 명령).</param>
    /// <returns>단계 A 에서 회수 명령 토큰과 일치하면 1(사건 소비), 그 밖에는 0.</returns>
    public uint Handle(uint currentEvent)
    {
        uint command = currentEvent & CommandMask;
        switch (_state.Stage!())
        {
            case 'A':
                if (command == _state.HomeCommand || _hooks.BuiltForGenus!(BridgeGenus) > 0) _hooks.Advance!();
                if (command == _state.ReclaimCommand) return 1;
                break;
            case 'B':
                if (_hooks.BuiltForGenus!(BridgeGenus) > FirstBridgeThreshold) _hooks.Advance!();
                break;
            case 'C':
                if (_hooks.BuiltForGenus!(BridgeGenus) > SecondBridgeThreshold)
                {
                    _hooks.Advance!();
                    _hooks.SpawnGeysers!();
                }
                break;
            case 'D':
            {
                if (_state.LocalPlayer >= PlayerCount) throw new InvalidDataException("튜토리얼 1 의 현재 Player 번호가 범위를 벗어났습니다.");
                // 간헐천을 도는 동안 신전 Graph 는 한 번만 구한다. Graph 0/254 도 원본처럼 그대로 비교한다.
                uint temple = _hooks.TempleOf!(_state.LocalPlayer);
                uint graph = _hooks.GraphFor!(temple);
                // 매 반복마다 현재 부호 있는 개수를 다시 읽고 일치하는 모든 항목에 Advance 를 요청한다.
                for (int index = 0; index < unchecked((int)_geysers.Count); index++)
                {
                    if (index >= _geysers.Entries.Count) throw new InvalidDataException("튜토리얼 1 의 간헐천 목록이 물리 범위를 벗어났습니다.");
                    if (_hooks.GraphFor!(_geysers.Entries[index]) == graph) _hooks.Advance!();
                }
                break;
            }
            case 'E':
                if (_state.BattleShown != 0 && _hooks.Money!(_state.LocalPlayer) >= FirstMoneyThreshold) _hooks.Advance!();
                break;
            case 'F':
                if (_state.BattleShown != 0 && _hooks.Money!(_state.LocalPlayer) >= SecondMoneyThreshold) _hooks.Advance!();
                break;
        }
        return 0;
    }
}
