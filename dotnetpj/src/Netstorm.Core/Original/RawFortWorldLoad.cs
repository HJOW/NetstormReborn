// 실제 저장 Fort 부모의 현재 Template 재조회·Territory 120BYTE·생성/후처리 순서를 공유한다 (cpppj o/RawFortWorldLoad.h/.cpp, 2026-11-07 F2c-12 이식).
using System;
using System.Collections.Generic;

namespace Netstorm.Core.Original;

/// <summary>
/// 실제 보고 뒤 계속하는 Template·stream 진단이다(cpppj <c>o/RawFortWorldLoad.h</c>의 <c>FortWorldLoadAssertion</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public enum FortWorldLoadAssertion
{
    /// <summary>Territory를 읽기 전 현재 Template의 첫 DWORD가 0이다 (2026-11-07 F2c-12 이식).</summary>
    MissingTerritoryTemplate,

    /// <summary>120BYTE 읽은 뒤 signed position &gt; end다 (2026-11-07 F2c-12 이식).</summary>
    TerritoryPastEnd,
}

/// <summary>
/// 모든 child는 실제 완료 몸체 또는 명시 미복원 경계이며 같은 raw 자료를 공급받는 콜백 묶음이다(cpppj <c>o/RawFortWorldLoad.h</c>의 <c>FortWorldLoadHooks</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public sealed class FortWorldLoadHooks
{
    /// <summary>전달된 현재 Template 칸의 첫 DWORD를 조회한다 (2026-11-07 F2c-12 이식).</summary>
    public Func<uint, bool>? TemplatePresent { get; set; }

    /// <summary>Territory lookup(reset pos 0)의 현재 stream을 돌려준다 (2026-11-07 F2c-12 이식).</summary>
    public Func<uint, FortTerritoryStream>? Territory { get; set; }

    /// <summary>각 후처리 직전 현재 localOwner의 저장 Template를 다시 조회한다 (2026-11-07 F2c-12 이식).</summary>
    public Func<uint, FortTemplate>? CurrentTemplate { get; set; }

    /// <summary>원본 보고 뒤 현재 상태로 계속하는 진단 경계다 (2026-11-07 F2c-12 이식).</summary>
    public Action<FortWorldLoadAssertion>? Assertion { get; set; }

    /// <summary>CD Territory의 진입 즉시 증감값을 같은 소비자에 전달한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<uint>? LoadingDepthChanged { get; set; }

    /// <summary>기존 실제 Place·PlaceAll의 필수 진단이다 (2026-11-07 F2c-12 이식).</summary>
    public Action<RawIslandBuilderAssertion>? BuilderAssertion { get; set; }

    /// <summary>같은 builder owner·bounds와 현재 noRegion으로 기존 정상 지형을 생성한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<RawIslandBuilderState, int>? Generate { get; set; }

    /// <summary>현재 Template를 Objects child에 같은 풀·깊이로 전달한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<FortTemplate, uint>? Objects { get; set; }

    /// <summary>현재 Template를 Money child에 같은 풀·깊이로 전달한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<FortTemplate, uint>? Money { get; set; }

    /// <summary>현재 Template를 Badges child에 같은 풀·깊이로 전달한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<FortTemplate, uint>? Badges { get; set; }

    /// <summary>현재 Template를 Technology child에 같은 풀·깊이로 전달한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<FortTemplate, uint>? Technology { get; set; }

    /// <summary>param nonzero와 현재 island 0 Exists일 때만 0046e630 / CD 004906c0를 요청한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<RawIslandBuilderState, int>? PopulateIsland { get; set; }

    /// <summary>마지막 00463110 / CD 0045bd60의 인자 0(false)를 그대로 전달한다 (2026-11-07 F2c-12 이식).</summary>
    public Action<bool>? RebuildGraph { get; set; }

    /// <summary>현재 Canon 진단 owner를 같은 섬·지형·설치 수명 동안 빌린다 (2026-11-07 F2c-12 이식).</summary>
    public CanonDecoderDiagnostics? DecoderDiagnostics { get; set; }

    /// <summary>현재 훅 묶음의 얕은 사본을 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>복사된 훅 인스턴스.</returns>
    public FortWorldLoadHooks Clone() => new()
    {
        TemplatePresent = TemplatePresent,
        Territory = Territory,
        CurrentTemplate = CurrentTemplate,
        Assertion = Assertion,
        LoadingDepthChanged = LoadingDepthChanged,
        BuilderAssertion = BuilderAssertion,
        Generate = Generate,
        Objects = Objects,
        Money = Money,
        Badges = Badges,
        Technology = Technology,
        PopulateIsland = PopulateIsland,
        RebuildGraph = RebuildGraph,
        DecoderDiagnostics = DecoderDiagnostics,
    };
}

/// <summary>
/// 기존 <see cref="RawIslandBuilder"/>·<see cref="RawIslandRuntime"/>을 재사용하고 미복원 전체 child를 기본 성공값으로 대체하지 않는 Fort 월드 적재 부모다(cpppj <c>o/RawFortWorldLoad</c>, 2026-11-07 F2c-12 이식).
/// </summary>
public sealed class RawFortWorldLoad
{
    /// <summary>Territory 섹션 읽기 크기(120바이트)다 (2026-11-07 F2c-12 이식).</summary>
    public const int TerritoryBytes = 120;

    /// <summary>같은 판본·Factory·섬 목록의 raw 풀이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly RawSidPool _pool;

    /// <summary>최초 builder attach가 가리키는 실제 지도다 (2026-11-07 F2c-12 이식).</summary>
    private readonly ChunkMap _world;

    /// <summary>현재 3204BYTE count/Exists와 Place의 Add/Bounds를 공유하는 섬 런타임이다 (2026-11-07 F2c-12 이식).</summary>
    private readonly RawIslandRuntime _islands;

    /// <summary>기존 실제 CanonDecoder의 불변 타입별 코드다 (2026-11-07 F2c-12 이식).</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>실제 builder가 현재 읽는 puzzlePiece 번호 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Func<uint> _puzzlePiece;

    /// <summary>child 직전마다 다시 읽는 원본 현재 플레이어 DWORD 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Func<uint> _localOwner;

    /// <summary>Generate 직전의 현재 지역 없음 전역 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Func<int> _noRegion;

    /// <summary>Territory/CD 및 Money/Meta가 공유하는 현재 로딩 깊이 읽기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Func<uint> _getLoadingDepth;

    /// <summary>Territory/CD 및 Money/Meta가 공유하는 현재 로딩 깊이 쓰기 대리자다 (2026-11-07 F2c-12 이식).</summary>
    private readonly Action<uint> _setLoadingDepth;

    /// <summary>미복원 child·출력·진단의 필수 명시 경계다 (2026-11-07 F2c-12 이식).</summary>
    private readonly FortWorldLoadHooks _hooks;

    /// <summary>동일 World·Runtime·원본 전역과 필수 child 경계를 부모 효과 전에 확인해 Fort 월드 로더를 만든다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="pool">SID 풀.</param>
    /// <param name="world">월드 청크 지도.</param>
    /// <param name="islands">섬 런타임.</param>
    /// <param name="frames">판본별 타입 프레임 표.</param>
    /// <param name="puzzlePiece">현재 puzzlePiece 타입 번호 읽기 대리자.</param>
    /// <param name="localOwner">현재 로컬 플레이어 번호 읽기 대리자.</param>
    /// <param name="noRegion">현재 지역 없음 값 읽기 대리자.</param>
    /// <param name="getLoadingDepth">현재 로딩 깊이 읽기 대리자.</param>
    /// <param name="setLoadingDepth">현재 로딩 깊이 쓰기 대리자.</param>
    /// <param name="hooks">하위 몸체·진단 콜백 묶음.</param>
    public RawFortWorldLoad(
        RawSidPool pool,
        ChunkMap world,
        RawIslandRuntime islands,
        IReadOnlyList<RiftTypeFrames> frames,
        Func<uint> puzzlePiece,
        Func<uint> localOwner,
        Func<int> noRegion,
        Func<uint> getLoadingDepth,
        Action<uint> setLoadingDepth,
        FortWorldLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(world);
        ArgumentNullException.ThrowIfNull(islands);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(puzzlePiece);
        ArgumentNullException.ThrowIfNull(localOwner);
        ArgumentNullException.ThrowIfNull(noRegion);
        ArgumentNullException.ThrowIfNull(getLoadingDepth);
        ArgumentNullException.ThrowIfNull(setLoadingDepth);
        ArgumentNullException.ThrowIfNull(hooks);

        int expectedFrames = pool.Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (!ReferenceEquals(islands.Pool(), pool) ||
            frames.Count != expectedFrames ||
            hooks.TemplatePresent is null ||
            hooks.Territory is null ||
            hooks.CurrentTemplate is null ||
            hooks.Assertion is null ||
            hooks.LoadingDepthChanged is null ||
            hooks.BuilderAssertion is null ||
            hooks.Generate is null ||
            hooks.Objects is null ||
            hooks.Money is null ||
            hooks.Badges is null ||
            hooks.Technology is null ||
            hooks.PopulateIsland is null ||
            hooks.RebuildGraph is null)
        {
            throw new ArgumentException("Fort World 실제 자료/필수 child/진단 연결 누락");
        }

        _pool = pool;
        _world = world;
        _islands = islands;
        _frames = frames;
        _puzzlePiece = puzzlePiece;
        _localOwner = localOwner;
        _noRegion = noRegion;
        _getLoadingDepth = getLoadingDepth;
        _setLoadingDepth = setLoadingDepth;
        _hooks = hooks.Clone();
    }

    /// <summary>004bd660 / CD 00427c00: 현재 pos의 120BYTE를 복사하고 현재 pos에 120을 더한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="templateSlot">Template 슬롯 번호.</param>
    /// <param name="output">120바이트 출력 버퍼.</param>
    public void ReadTerritory(uint templateSlot, Span<byte> output)
    {
        if (output.Length != TerritoryBytes)
        {
            throw new ArgumentException("output은 120바이트여야 합니다.", nameof(output));
        }
        if (_pool.Edition == OriginalEdition.Cd1072)
        {
            uint inc = unchecked(_getLoadingDepth() + 1u);
            _setLoadingDepth(inc);
            _hooks.LoadingDepthChanged!(inc);
            uint dec = unchecked(_getLoadingDepth() - 1u);
            _setLoadingDepth(dec);
            _hooks.LoadingDepthChanged!(dec);
        }
        if (!_hooks.TemplatePresent!(templateSlot))
        {
            _hooks.Assertion!(FortWorldLoadAssertion.MissingTerritoryTemplate);
        }

        FortTerritoryStream stream = _hooks.Territory!(templateSlot);
        uint position = stream.Position;
        ReadOnlySpan<byte> bytes = stream.Bytes.Span;
        if (position > (uint)bytes.Length || (uint)bytes.Length - position < (uint)output.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(templateSlot), "Fort Territory 현재 물리 120BYTE 누락");
        }

        // 실제 DWORD 30개 복사와 동일한 120BYTE만 읽으며 사용하지 않는 꼬리는 보존한다
        for (int index = 0; index < output.Length; ++index)
        {
            output[index] = bytes[(int)position + index];
        }
        stream.Position = unchecked(position + 120u);
        if (unchecked((int)stream.End) < unchecked((int)stream.Position))
        {
            _hooks.Assertion!(FortWorldLoadAssertion.TerritoryPastEnd);
        }
    }

    /// <summary>004b5f70 / CD 004330c0: ctor/attach → zero 120 → Territory → PlaceAll → Generate → 모드별 후처리 → Graph 0 순서로 실행한다 (2026-11-07 F2c-12 이식).</summary>
    /// <param name="alternate">0이면 일반 Objects/Money/Badges/Technology 경로, 0이 아니면 섬 0 Populate 경로.</param>
    public void Load(int alternate)
    {
        var state = new RawIslandBuilderState
        {
            DecoderDiagnostics = _hooks.DecoderDiagnostics,
        };
        var builder = new RawIslandBuilder(
            _world,
            _world,
            _islands,
            _frames,
            _puzzlePiece,
            state,
            _hooks.BuilderAssertion);

        Span<byte> records = stackalloc byte[TerritoryBytes];
        ReadTerritory(_localOwner(), records);
        builder.PlaceAll(records, 0, 20);
        _hooks.Generate!(state, _noRegion());
        if (alternate == 0)
        {
            uint owner = _localOwner();
            _hooks.Objects!(_hooks.CurrentTemplate!(owner), owner);
            owner = _localOwner();
            _hooks.Money!(_hooks.CurrentTemplate!(owner), owner);
            owner = _localOwner();
            _hooks.Badges!(_hooks.CurrentTemplate!(owner), owner);
            owner = _localOwner();
            _hooks.Technology!(_hooks.CurrentTemplate!(owner), owner);
        }
        else if (_islands.Exists(0) != 0u)
        {
            _hooks.PopulateIsland!(state, 0);
        }
        _hooks.RebuildGraph!(false);
    }

    /// <summary>현재 모듈의 풀 참조만 공개하며 새 Pool 사본을 만들지 않는다 (2026-11-07 F2c-12 이식).</summary>
    /// <returns>연결된 <see cref="RawSidPool"/>.</returns>
    public RawSidPool Pool() => _pool;

    /// <summary>
    /// 기존 <see cref="RawFortObjectLoad"/>와 <see cref="RawFortMetaLoad"/>의 완료 몸체만 연결하며 Money와 섹션 선택·TypeNames·UI는 호출자 경계를 유지한다(cpppj <c>o/RawFortWorldLoad.h</c>의 <c>MakeFortWorldChildrenHooks</c>, 2026-11-07 F2c-12 이식).
    /// </summary>
    /// <param name="pool">공유 SID 풀.</param>
    /// <param name="objects">객체 적재 몸체.</param>
    /// <param name="meta">배지·기술 적재 몸체.</param>
    /// <param name="selectMeta">각 child의 현재 Template 칸·포인터·owner를 MetaState에 동기화하는 콜백.</param>
    /// <param name="metaStream">실제 Badges·Technology 섹션을 선택해 현재 cursor를 반환하는 콜백.</param>
    /// <param name="hooks">기반 훅 묶음.</param>
    /// <returns>Objects·Badges·Technology가 연결된 새 훅 묶음.</returns>
    public static FortWorldLoadHooks MakeFortWorldChildrenHooks(
        RawSidPool pool,
        RawFortObjectLoad objects,
        RawFortMetaLoad meta,
        Action<FortTemplate, uint, bool>? selectMeta,
        Func<FortTemplate, bool, FortMetaStream>? metaStream,
        FortWorldLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(objects);
        ArgumentNullException.ThrowIfNull(meta);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(objects.Pool(), pool) ||
            !ReferenceEquals(meta.Pool(), pool) ||
            selectMeta is null ||
            metaStream is null)
        {
            throw new ArgumentException("Fort World child 실제 풀/현재 전역/섹션 조회 불일치");
        }

        FortWorldLoadHooks result = hooks.Clone();
        result.Objects = (fort, owner) => objects.Load(fort, unchecked((int)owner), 0, 20);
        // 실제 섹션 getter는 각 wrapper의 Template 진단·기술 부모 뒤에 실행하고 현재 Template 입력을 유지한다
        result.Badges = (fort, owner) =>
        {
            selectMeta(fort, owner, false);
            meta.LoadBadgesSelected(() => metaStream(fort, false));
        };
        result.Technology = (fort, owner) =>
        {
            selectMeta(fort, owner, true);
            meta.LoadTechnologySelected(() => metaStream(fort, true));
        };
        return result;
    }
}
