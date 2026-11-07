// 실제 미션 설치 부모의 prologue→Player→Builder/Territory/PlaceAll→Generate→Objects→Graph→mission finish→Camera→현재이름 순서를 공유한다.
namespace Netstorm.Core.Original;

// 캠페인 월드 적재 훅 묶음을 담는 네임스페이스다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.

// 이미 완료한 동일 월드 하위 몸체와 명시 번역/config/spec/UI 경계를 공급하는 훅 묶음이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
public sealed class CampaignWorldLoadHooks
{
    /// <summary>원본 번역키 "You"의 NUL 포함 실제 결과 바이트다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Func<byte[]>? TranslateYou { get; set; }

    /// <summary>fort.mySubHandle과 NUL 포함 현재 handle을 원본 config 경계로 전달한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action<string, byte[]>? SetConfig { get; set; }

    /// <summary>현재 창 제목 갱신 경계다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action? RefreshWindowTitle { get; set; }

    /// <summary>실제 PlaceAll/Add/Bounds/CanonDecoder 진단을 보존한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action<RawIslandBuilderAssertion>? BuilderAssertion { get; set; }

    /// <summary>같은 지도/3204바이트/두난수의 현재 정상 지형 생성이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action<RawIslandBuilderState, int>? Generate { get; set; }

    /// <summary>Objects 직전의 현재 Template owner를 다시 선택한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Func<uint, FortTemplate>? CurrentTemplate { get; set; }

    /// <summary>기존 실제 ObjectLoad(현재 owner, 0, 20)를 같은 Factory/Pool로 실행한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action<FortTemplate, uint>? Objects { get; set; }

    /// <summary>실제 Graph.Rebuild(false)를 정확히 한 번 요청한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action<bool>? RebuildGraph { get; set; }

    /// <summary>미복원 카메라/UI 경계다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action? ResetCamera { get; set; }

    /// <summary>현재 Canon 진단 owner를 같은 섬/지형/설치 수명 동안 빌린다. null이면 빌리지 않는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public CanonDecoderDiagnostics? DecoderDiagnostics { get; set; }

    /// <summary>missionPost가 0일 때 실제 부모를 선택한다. missionPost가 참이면 호출하지 않는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public Action? PreparePriests { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    /// <returns>복사된 훅 묶음이다.</returns>
    public CampaignWorldLoadHooks Clone() => new()
    {
        TranslateYou = TranslateYou,
        SetConfig = SetConfig,
        RefreshWindowTitle = RefreshWindowTitle,
        BuilderAssertion = BuilderAssertion,
        Generate = Generate,
        CurrentTemplate = CurrentTemplate,
        Objects = Objects,
        RebuildGraph = RebuildGraph,
        ResetCamera = ResetCamera,
        DecoderDiagnostics = DecoderDiagnostics,
        PreparePriests = PreparePriests,
    };
}

// Money/Badges/Technology를 호출하지 않는 실제 미션 설치 부모다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
public sealed class RawCampaignWorldLoad
{
    /// <summary>실제 Template 수다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int TemplateCount = 9;

    /// <summary>실제 Template header 한 칸 크기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int TemplateStride = 20;

    /// <summary>실제 Template header 안에서 1로 저장하는 +8 DWORD 위치다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int TemplateHeaderField = 8;

    /// <summary>실제 현재 9×20바이트 Template header 크기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int TemplatesBytes = 180;

    /// <summary>실제 Player 수다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int PlayerCount = 9;

    /// <summary>실제 Player 한 기록 크기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int PatchPlayerStride = 0xb4;

    /// <summary>CD판 Player 한 기록 크기다. 첫 구현은 PATCH 전용이라 관측되지 않으며 후속 단위에서 잇는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int CdPlayerStride = 0xac;

    /// <summary>실제 Player 기록 안에서 현재 handle을 복사하는 이름 위치다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int NameField = 0xc;

    /// <summary>editor가 0이 아닐 때 0으로 쓰는 실제 Player residence/island DWORD 위치다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int PatchEditorField = 0x78;

    /// <summary>CD판 editor DWORD 위치다. 첫 구현은 PATCH 전용이라 관측되지 않으며 후속 단위에서 잇는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private const int CdEditorField = 0x70;

    /// <summary>실제 builder attach의 원본 ChunkMap이며 사본을 만들지 않는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly ChunkMap _world;

    /// <summary>실제 current count/Exists/Owner/Bounds의 3204바이트 저장소다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly RawIslandRuntime _islands;

    /// <summary>기존 PlaceAll decoder가 읽는 같은 TYPE 프레임이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>실제 decoder가 현재 읽는 puzzlePiece 타입 전역 읽기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly Func<uint> _puzzlePiece;

    /// <summary>Generate 직전 재조회하는 현재 지역없음 전역 읽기다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly Func<int> _noRegion;

    /// <summary>기존 실제 120바이트 getter 몸체를 읽기 재사용하는 Territory 부모다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly RawFortWorldLoad _territory;

    /// <summary>실제 152 wrapper/Post와 현재 9기록/두 owner를 직접 공유하는 Player 부모다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly RawPlayerWorld _player;

    /// <summary>매 단계의 현재 subscriber/editor/missionPost를 참조한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly CampaignWorldGlobals _globals;

    /// <summary>현재 9×20바이트 Template header이며 +8 DWORD를 직접 쓴다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly byte[] _templates;

    /// <summary>현재 mySubHandle의 실제 NUL 저장소다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly byte[] _handle;

    /// <summary>미복원 번역/config/spec/UI를 기본성공으로 덮지 않는 훅 묶음이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    private readonly CampaignWorldLoadHooks _hooks;

    /// <summary>
    /// 같은 완료 자료와 명시 필수 하위 경계를 실제 최초쓰기 전에 확인한다. 이 첫 구현은 PATCH 전용이며 CD 판본·전수 장면 재생은 후속 단위에서 잇는다.
    /// </summary>
    /// <param name="world">실제 builder attach의 원본 ChunkMap이다.</param>
    /// <param name="islands">실제 3204바이트 섬 런타임이다.</param>
    /// <param name="frames">같은 TYPE 프레임 표다.</param>
    /// <param name="puzzlePiece">현재 puzzlePiece 타입 전역 읽기다.</param>
    /// <param name="noRegion">현재 지역없음 전역 읽기다.</param>
    /// <param name="territory">실제 Territory getter 부모다.</param>
    /// <param name="player">실제 Player 부모다.</param>
    /// <param name="globals">현재 subscriber/editor/missionPost 전역이다.</param>
    /// <param name="templates">현재 9×20바이트 Template header다.</param>
    /// <param name="handle">현재 mySubHandle의 실제 NUL 저장소다.</param>
    /// <param name="hooks">하위 몸체·진단 훅 묶음이다.</param>
    /// <exception cref="ArgumentException">실제 캠페인 부모의 동일 Pool/현재 저장소/필수 하위 경계 누락이다.</exception>
    public RawCampaignWorldLoad(
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> puzzlePiece,
        Func<int> noRegion,
        RawFortWorldLoad territory,
        RawPlayerWorld player,
        CampaignWorldGlobals globals,
        byte[] templates,
        byte[] handle,
        CampaignWorldLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(puzzlePiece);
        ArgumentNullException.ThrowIfNull(noRegion);
        ArgumentNullException.ThrowIfNull(territory);
        ArgumentNullException.ThrowIfNull(player);
        ArgumentNullException.ThrowIfNull(globals);
        ArgumentNullException.ThrowIfNull(templates);
        ArgumentNullException.ThrowIfNull(handle);
        ArgumentNullException.ThrowIfNull(hooks);
        int expectedFrames = player.Pool().Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (!ReferenceEquals(islands.Pool(), player.Pool()) ||
            !ReferenceEquals(territory.Pool(), player.Pool()) ||
            frames.Count != expectedFrames ||
            templates.Length != TemplatesBytes ||
            handle.Length == 0 ||
            hooks.TranslateYou is null ||
            hooks.SetConfig is null ||
            hooks.RefreshWindowTitle is null ||
            hooks.BuilderAssertion is null ||
            hooks.Generate is null ||
            hooks.CurrentTemplate is null ||
            hooks.Objects is null ||
            hooks.RebuildGraph is null ||
            hooks.ResetCamera is null)
        {
            throw new ArgumentException("실제 캠페인 부모의 동일 Pool/현재 저장소/필수 하위 경계 누락");
        }

        _world = world;
        _islands = islands;
        _frames = frames;
        _puzzlePiece = puzzlePiece;
        _noRegion = noRegion;
        _territory = territory;
        _player = player;
        _globals = globals;
        _templates = templates;
        _handle = handle;
        _hooks = hooks.Clone();
    }

    /// <summary>
    /// 원본 두 strcpy 형태는 같은 NUL 바이트까지 쓰며 나머지 stale 메모리를 보존한다.
    /// </summary>
    /// <param name="target">쓸 대상이다.</param>
    /// <param name="source">NUL 포함 원본이다.</param>
    /// <exception cref="ArgumentOutOfRangeException">캠페인 실제 NUL 문자열 물리바이트 누락 또는 이름 대상 물리바이트 범위 초과다.</exception>
    public static void CopyName(Span<byte> target, ReadOnlySpan<byte> source)
    {
        // 원본 좁은문자열의 첫 NUL까지만 읽는 탐색이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        int nul = source.IndexOf((byte)0);
        if (nul < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(source), "캠페인 실제 NUL 문자열 물리바이트 누락");
        }

        // 원본이 마지막에 저장하는 NUL을 포함하는 개수다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        int count = nul + 1;
        if (count > target.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(target), "캠페인 실제 이름 대상 물리바이트 범위 초과");
        }

        source.Slice(0, count).CopyTo(target);
    }

    /// <summary>
    /// 현재 두 owner를 합치지 않고 실제 battleOwner 기록만 선택한다.
    /// </summary>
    /// <returns>현재 battleOwner의 실제 Player 기록 뷰이며 사본을 만들지 않는다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">캠페인 현재 battleOwner의 실제 9기록 범위 오류다.</exception>
    public Span<byte> CurrentPlayer()
    {
        // 부모의 각 현재 Player 접근 직전에 다시 읽는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        uint owner = _player.State().BattleOwner;
        if (owner >= (uint)PlayerCount)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "캠페인 현재 battleOwner의 실제 9기록 범위 오류");
        }

        int stride = _player.Pool().Edition == OriginalEdition.Patch1078 ? PatchPlayerStride : CdPlayerStride;
        return _player.Records().AsSpan(unchecked((int)(owner * (uint)stride)), stride);
    }

    /// <summary>
    /// mission1은 자연 반환하고 그 밖에는 실제 현재 기본 기술/지역 사제 부모를 선택한다.
    /// </summary>
    /// <exception cref="InvalidOperationException">Fort 실제 기술/사제 준비 부모 미연결이다.</exception>
    public void FinishMissionPriests()
    {
        if (!_globals.MissionPost)
        {
            if (_hooks.PreparePriests is null)
            {
                throw new InvalidOperationException("Fort 실제 기술/사제 준비 부모 미연결");
            }

            _hooks.PreparePriests();
        }
    }

    /// <summary>본문순서를 보존하고 기존 하위 body를 다시 구현하지 않는다. 실제 prologue→Player→Builder/Territory/PlaceAll→Generate→Objects→Graph0→mission finish→Camera→현재이름 순서다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    public void Load()
    {
        _globals.MySubscriber = _globals.LoginSubscriber;
        uint owner = _player.State().TemplateOwner;
        if (owner >= (uint)PlayerCount)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "캠페인 현재 Template owner의 실제 9기록 범위 오류");
        }

        // 최초 현재 Template의 +8 DWORD만 1로 저장한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        Span<byte> header = _templates.AsSpan(unchecked((int)(owner * (uint)TemplateStride + (uint)TemplateHeaderField)), 4);
        header[0] = 1;
        header[1] = 0;
        header[2] = 0;
        header[3] = 0;
        CopyName(_handle, Need(_hooks.TranslateYou)());
        // config에 전달하는 현재 NUL 포함 handle이다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.
        int nul = _handle.AsSpan().IndexOf((byte)0);
        Need(_hooks.SetConfig)("fort.mySubHandle", _handle.AsSpan(0, nul + 1).ToArray());
        Need(_hooks.RefreshWindowTitle)();
        _player.Create(unchecked((int)_player.State().BattleOwner));
        var state = new RawIslandBuilderState
        {
            DecoderDiagnostics = _hooks.DecoderDiagnostics,
        };
        var builder = new RawIslandBuilder(_world, _world, _islands, _frames, _puzzlePiece, state, Need(_hooks.BuilderAssertion));
        if (_globals.Editor != 0u)
        {
            Span<byte> raw = CurrentPlayer();
            int field = _player.Pool().Edition == OriginalEdition.Patch1078 ? PatchEditorField : CdEditorField;
            raw.Slice(field, 4).Clear();
        }

        byte[] records = new byte[RawIslandBuilder.RawTerritoryBytes];
        _territory.ReadTerritory(_player.State().TemplateOwner, records);
        builder.PlaceAll(records, 0, 20);
        Need(_hooks.Generate)(state, _noRegion());
        uint current = _player.State().TemplateOwner;
        Need(_hooks.Objects)(Need(_hooks.CurrentTemplate)(current), current);
        Need(_hooks.RebuildGraph)(false);
        FinishMissionPriests();
        Need(_hooks.ResetCamera)();
        CopyName(CurrentPlayer().Slice(NameField), _handle);
    }

    /// <summary>실제 Player와 같은 pool 참조를 공개하며 풀/Player 사본을 만들지 않는다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    /// <returns>쓰는 슬롯 풀이다.</returns>
    public RawSidPool Pool() => _player.Pool();

    /// <summary>실제 도달한 효과만 필수로 요구한다. 2026-11-07 F2d 캠페인 월드 적재 본체 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">캠페인 월드 적재의 선택된 실제 하위 미연결이다.</exception>
    private static T Need<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("캠페인 월드 적재의 선택된 실제 하위 미연결");
        }

        return callback;
    }
}
