using Netstorm.Core.Original;

// 실제 사제/소리와 동일한 Host·풀·장기전역에 공통표시/연소를 연결한다.
namespace Netstorm.Client.Host;

/// <summary>실제 미션의 공통 표시 목록과 저HP 연소를 한 수명으로 조립한다.</summary>
public sealed partial class HostSession
{
    /// <summary>이 미션의 모든 효과가 공유할 실제 등록/수집 owner다.</summary>
    private RenderProcessSystem? _worldRendering;
    /// <summary>실제 같은 Host/Form/Kernel이 소유하는 연소14 조회기다.</summary>
    private BurningVisualSystem? _worldBurning;
    /// <summary>같은 현재 보상HP와 실제 연소의 생성/제거 부모다.</summary>
    private RawDamageableBurning? _worldDamageableBurning;

    /// <summary>다른 파생 표시도 같은 현재 목록에 등록할 수 있게 빌린다.</summary>
    public RenderProcessSystem WorldRendering => _worldRendering ?? throw new InvalidOperationException("호스트 공통 표시 미연결");
    /// <summary>실제 연소 프로세스 조회/생성 owner다. 프레임/장치는 구성자보다 오래 살아야 한다.</summary>
    public BurningVisualSystem WorldBurning => _worldBurning ?? throw new InvalidOperationException("호스트 연소 시스템 미연결");
    /// <summary>피해/회복이 같은 원문 저HP 갱신 몸체를 빌린다.</summary>
    public RawDamageableBurning WorldDamageableBurning => _worldDamageableBurning ?? throw new InvalidOperationException("호스트 저HP 연소 미연결");

    /// <summary>원문 식/파일/줄을 잃지 않는 기본 진단이다. 후속UI 장치의 정상복귀와 구별한다.</summary>
    private static void BurningAssertion(string expression, string file, uint line) =>
        throw new InvalidOperationException($"호스트 연소 원본 진단: {expression} {file}:{line}");

    /// <summary>관리 풀의 현재 상대주소에서 원문 상세SID 몸체를 실행한다. 세대/void 사전필터를 추가하지 않는다.</summary>
    /// <remarks>고정 네이티브주소로 위장하지 않는다. 원문의 base+sid*50을 관리배열의 byte offset으로 대응하며,
    /// 각 선택필드 접근에서 현재 저장소를 읽는다. TYPE의 논리범위 선행조회와 진단 복귀 순서는 같은 몸체다.</remarks>
    public bool CurrentWorldSidRobust(int sid, bool live) => RawSidRobust.IsSidRobust(sid, live ? 1 : 0, Gameplay.SidRobustDiagnostics, new()
    {
        AddressMode = SidRobustAddressMode.HostPointer, PoolBase = static () => 0,
        Capacity = () => (uint)Pool.Capacity, Checking = () => AiChecking.Read(),
        Memory = (offset, width) =>
        {
            ReadOnlySpan<byte> bytes = Pool.Bytes;
            if (offset > (ulong)bytes.Length || width < 0 || (ulong)width > (ulong)bytes.Length - offset)
                throw new ArgumentOutOfRangeException(nameof(offset), "SID 진단의 현재 관리 Pool 물리 범위");
            return bytes.Slice((int)offset, width).ToArray();
        },
        Assertion = (expression, line) => BurningAssertion(expression, "SID", line),
    });

    /// <summary>실제 같은 TYPE/HP·공장·프로세스에 현재 연소장치를 대여한다. 중복 연결과 다른 풀 장치는 거부된다.</summary>
    public BurningVisualSystem BuildBurningWorldDevices(BurningFrameState frame, BurningDrawPalette palette,
        Func<uint> frameNumber, BurningSystemHooks hooks, Action<int, float, float>? bulfSound = null)
    {
        ArgumentNullException.ThrowIfNull(frame); ArgumentNullException.ThrowIfNull(palette);
        ArgumentNullException.ThrowIfNull(frameNumber); ArgumentNullException.ThrowIfNull(hooks);
        if (_worldBurning is not null) throw new InvalidOperationException("호스트 실제 연소 중복 연결");
        _ = BindLivePlayerWorld();
        IReadOnlyList<OriginalTypeRecord> types = _terrainPopTypes!;
        var rendering = new RenderProcessSystem(Pool, PlayerProcesses, new(), new()
        {
            // 배열은 공통 등록기의 실제 관리배열이 확보/반납한다.
            Reserve = static _ => true, ReleaseArray = static () => { },
        });
        frame.PixelScaleX = rendering.State.ScaleX; frame.PixelScaleY = rendering.State.ScaleY;
        var sources = new BurningSourcesState
        {
            SharedWeakenedMana = () => GameAux.PriestQuarterHp,
            SharedManaType = GameAux.PriestTypeBox, SharedDaisType = GameAux.DaisTypeBox,
        };
        // 세 연소 소비자가 같은 현재checking/상세진단 owner를 읽는다.
        foreach (BurningVictimState victim in new[] { sources.Victim, frame.Victim })
        {
            victim.Checking = new RawCheckingFlag(() => AiChecking.Read());
            victim.Robust = CurrentWorldSidRobust; victim.Assertion = BurningAssertion;
        }
        var burning = new BurningVisualSystem(rendering, Game, types, new(), sources, frame, palette, frameNumber, hooks);
        var lookup = new RawContainedLookup(Pool, DestroyBurningForm);
        var damageable = new RawDamageableBurning(Pool, types, _fortHp!, new()
        { Checking = new RawCheckingFlag(() => AiChecking.Read()) }, new()
        {
            Find = (parent, type, ordinal) => lookup.Find(parent, type, ordinal),
            Create = victim =>
            {
                // 원문 wrapper의 실제 확보 실패만 자연0으로 복귀한다. 다른 미연결/계산 실패는 숨기지 않는다.
                try { return burning.Add(victim, 1).Fields.Registration.Form; }
                catch (OutOfMemoryException) { return 0; }
            },
            Destroy = DestroyBurningForm, BulfSound = bulfSound, Assertion = BurningAssertion,
        });
        _worldRendering = rendering; _worldBurning = burning; _worldDamageableBurning = damageable;
        return burning;
    }

    /// <summary>같은 실제 Form의 가상 삭제/등록반납/Kernel 소멸을 실행한다.</summary>
    private void DestroyBurningForm(int form, uint flags) => PlayerProcesses.Destroy().Destroy(form, flags, PlayerProcesses.Hooks());

    /// <summary>같은 Host가 살아 있고 외부 캡처/소리/팔레트 장치가 남아 있을 때 연소만 먼저 반납한다.</summary>
    public void ReleaseBurningWorld()
    {
        EnsureRuntimeCanRelease();
        ReleaseConstructionWorld();
        _worldBurning?.Dispose(); _worldBurning = null; _worldDamageableBurning = null; _worldRendering = null;
    }
}
