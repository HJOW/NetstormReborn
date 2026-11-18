// 신전 소유자의 실제 가상 재정의(+0x74) 몸체를 복원한다.
namespace Netstorm.Core.Original;

// 신전 가상 소유자 몸체를 담는 네임스페이스다. 2026-11-19 0.620에서 R4-2 지역 소유자 확정용으로 추가했다.

// 신전 Owner 몸체가 호출하는 실제 하위 경계다. 2026-11-19 0.620에서 추가했다.
// 근거: cpppj o/RawVortexOwner.h 의 VortexOwnerHooks 다.
public sealed class VortexOwnerHooks
{
    /// <summary>현재 좌표의 지면 지역 번호를 조회한다. 2026-11-19 0.620에서 추가했다.</summary>
    public Func<float, float, int>? IslandNumber { get; set; }

    /// <summary>같은 섬 목록의 현재 존재 DWORD를 검사한다. 2026-11-19 0.620에서 추가했다.</summary>
    public Func<int, bool>? IslandExists { get; set; }

    /// <summary>실제 섬 Owner의 목록/Player 전파를 실행한다. 2026-11-19 0.620에서 추가했다.</summary>
    public Action<int, uint>? SetIslandOwner { get; set; }

    /// <summary>실제 지역 투표·지형 도장을 mode 0으로 요청한다. 2026-11-19 0.620에서 추가했다.</summary>
    public Action<float, float>? RegionOwnership { get; set; }

    /// <summary>마지막 실제 생산 갱신을 호출한다. 2026-11-19 0.620에서 추가했다.</summary>
    public Action? RefreshProduction { get; set; }

    /// <summary>현재 SID/지역/새 소유자의 진단을 보고한 뒤 계속한다. 2026-11-19 0.620에서 추가했다.</summary>
    public Action<int, int, uint>? MissingIsland { get; set; }
}

// PATCH 004dc4e0/CD 0044f2a0의 실제 신전 Owner 순서와 판본 차이를 복원한다. 2026-11-19 0.620에서 추가했다.
// 근거: cpppj o/RawVortexOwner.h/.cpp, 원본 가상 표 +0x74 분배다.
public sealed class RawVortexOwner
{
    /// <summary>같은 원본 Owner 몸체를 가리키는 패치의 네 신전 가상 표다. 2026-11-19 0.620에서 추가했다.</summary>
    private static readonly uint[] PatchTables = [0x005196f0u, 0x00519860u, 0x005197a8u, 0x00519918u];

    /// <summary>세 원소의 CD 신전 가상 표다. 2026-11-19 0.620에서 추가했다.</summary>
    private static readonly uint[] CdTables = [0x00501d28u, 0x00501de0u, 0x00501e98u];

    /// <summary>같은 풀의 실제 슬롯 저장소다. 이 몸체보다 오래 살아야 한다. 2026-11-19 0.620에서 추가했다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>기존 독립 검증된 비가상 공통 Owner 몸체다. 2026-11-19 0.620에서 추가했다.</summary>
    private readonly SquidOwner _base;

    /// <summary>매 선택 위치에서 현재 실제 9 Player 기록을 읽는다. 2026-11-19 0.620에서 추가했다.</summary>
    private readonly Func<byte[]> _players;

    /// <summary>패치 추가 지역 갱신이 매 반복 읽는 별도 현재 목록이다. 2026-11-19 0.620에서 추가했다.</summary>
    private readonly RawPostPopList _outposts;

    /// <summary>패치 생산 갱신 뒤 증가하는 현재 DWORD다. CD는 쓰지 않는다. 2026-11-19 0.620에서 추가했다.</summary>
    private readonly UIntBox _revision;

    /// <summary>원본 필수 하위의 실제 공유 저장소와 소비자다. 2026-11-19 0.620에서 추가했다.</summary>
    private readonly VortexOwnerHooks _hooks;

    /// <summary>같은 풀/실제 9 기록/필수 효과를 최초 쓰기 전에 확인한다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="pool">같은 풀의 슬롯 저장소다.</param>
    /// <param name="base">기존 공통 Owner 몸체다. 같은 풀을 빌려야 한다.</param>
    /// <param name="players">원본 9개 Player 기록이다. 판본별 1620/1548바이트여야 한다.</param>
    /// <param name="outposts">패치가 매 반복 읽는 별도 목록이다.</param>
    /// <param name="revision">패치 생산 갱신 뒤 증가하는 DWORD 상자다.</param>
    /// <param name="hooks">섬·지역·생산·진단의 실제 소비자다.</param>
    public RawVortexOwner(RawSidPool pool, SquidOwner baseOwner, byte[] players, RawPostPopList outposts, UIntBox revision, VortexOwnerHooks hooks)
        : this(pool, baseOwner, () => players, outposts, revision, hooks)
    {
        // 생성 시점에 같은 Player 저장소 크기를 확정한다.
        _ = Records();
    }

    /// <summary>현재 getter 정체성만 결합하고 실제 Player 필드를 읽기 전에는 배열을 요구하지 않는다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="pool">같은 풀의 슬롯 저장소다.</param>
    /// <param name="baseOwner">기존 공통 Owner 몸체다. 같은 풀을 빌려야 한다.</param>
    /// <param name="players">매 선택 위치에서 현재 실제 9 기록을 읽는다.</param>
    /// <param name="outposts">패치가 매 반복 읽는 별도 목록이다.</param>
    /// <param name="revision">패치 생산 갱신 뒤 증가하는 DWORD 상자다.</param>
    /// <param name="hooks">섬·지역·생산·진단의 실제 소비자다.</param>
    public RawVortexOwner(RawSidPool pool, SquidOwner baseOwner, Func<byte[]> players, RawPostPopList outposts, UIntBox revision, VortexOwnerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(baseOwner);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(outposts);
        ArgumentNullException.ThrowIfNull(revision);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(baseOwner.Pool, pool) || players is null || hooks.IslandNumber is null || hooks.IslandExists is null
            || hooks.SetIslandOwner is null || hooks.RegionOwnership is null || hooks.RefreshProduction is null || hooks.MissingIsland is null)
        {
            throw new ArgumentException("신전 Owner의 실제 Pool/9기록/필수 효과 누락", nameof(hooks));
        }

        _pool = pool;
        _base = baseOwner;
        _players = players;
        _outposts = outposts;
        _revision = revision;
        _hooks = hooks;
    }

    /// <summary>같은 월드의 실제 지역 조회인지 확인할 풀이다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <returns>이 몸체가 쓰는 SID 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>같은 9 기록을 참조하며 별도 Player 복사본을 돌려주지 않는다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <returns>현재 실제 9 Player 기록이다.</returns>
    public byte[] Records()
    {
        byte[] current = _players();
        uint stride = _pool.Edition == OriginalEdition.Patch1078 ? 0xb4u : 0xacu;
        if ((uint)current.Length != 9 * stride)
        {
            throw new ArgumentException("신전 Owner의 실제 현재 9 Player 크기 오류", nameof(_players));
        }

        return current;
    }

    /// <summary>두 판본의 실제 신전 가상 표 +0x74인지 확인한다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>가상 재정의 대상이면 참이다.</returns>
    public bool Handles(int sid) => HandlesRawVortexOwner(_pool, sid);

    /// <summary>실제 Owner 몸체와 가상 분배가 같은 보호 PE 표를 읽고 TYPE 이름으로 대체하지 않는다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="pool">같은 풀의 슬롯 저장소다.</param>
    /// <param name="sid">확인할 슬롯 번호다.</param>
    /// <returns>가상 재정의 대상이면 참이다.</returns>
    public static bool HandlesRawVortexOwner(RawSidPool pool, int sid)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        uint table = (uint)(raw[0] | (raw[1] << 8) | (raw[2] << 16) | (raw[3] << 24));
        uint[] candidates = pool.Edition == OriginalEdition.Patch1078 ? PatchTables : CdTables;
        // 네 후보 표와 같은 보호 PE 주소를 찾는다.
        foreach (uint candidate in candidates)
        {
            if (table == candidate)
            {
                return true;
            }
        }

        return false;
    }

    /// <summary>이전 신전 해제→섬/지역/패치 목록→공통 Owner→현재 신전 기록→생산→패치 revision 순서다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="sid">소유자를 정할 신전 번호다.</param>
    /// <param name="owner">새 소유자 번호다. 0~8이어야 한다.</param>
    public void Set(int sid, uint owner)
    {
        if (!Handles(sid))
        {
            throw new ArgumentException("미복원 신전 가상 Owner 표", nameof(sid));
        }

        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        int ownerOffset = patch ? 34 : 32;
        int extraOffset = patch ? 40 : 35;
        byte previous = _pool.Slot(sid)[ownerOffset];
        byte extra = _pool.Slot(sid)[extraOffset];
        if ((extra & 8) == 0 && previous != 0 && Read(Records(), TempleField(previous)) == (uint)sid)
        {
            Write(Records(), TempleField(previous), 0);
            Write(Records(), TempleField(previous, true), 0);
        }

        if ((_pool.Slot(sid)[11] & 4) == 0 && (extra & 9) == 0)
        {
            float x = Position(_pool.Slot(sid), 14);
            float y = Position(_pool.Slot(sid), 18);
            int island = Need(_hooks.IslandNumber, nameof(VortexOwnerHooks.IslandNumber))(x, y);
            if (!Need(_hooks.IslandExists, nameof(VortexOwnerHooks.IslandExists))(island))
            {
                Need(_hooks.MissingIsland, nameof(VortexOwnerHooks.MissingIsland))(sid, island, owner);
            }

            Need(_hooks.SetIslandOwner, nameof(VortexOwnerHooks.SetIslandOwner))(island, owner);
            Need(_hooks.RegionOwnership, nameof(VortexOwnerHooks.RegionOwnership))(Position(_pool.Slot(sid), 14), Position(_pool.Slot(sid), 18));
            if (patch)
            {
                // 현재 count와 각 SID·소유자·좌표를 효과 뒤에 다시 읽는다. free/dead 필터는 추가하지 않는다.
                for (uint index = 0; unchecked((int)index) < unchecked((int)_outposts.Count); ++index)
                {
                    if (index >= (uint)_outposts.Entries.Length)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "신전 Owner 현재 추가 목록 범위 오류");
                    }

                    if (_outposts.Entries[(int)index] >= (uint)_pool.Capacity)
                    {
                        throw new ArgumentOutOfRangeException(nameof(sid), "신전 Owner 현재 추가 목록 범위 오류");
                    }

                    int other = (int)(ushort)_outposts.Entries[(int)index];
                    if (_pool.Slot(other)[ownerOffset] == _pool.Slot(sid)[ownerOffset])
                    {
                        Need(_hooks.RegionOwnership, nameof(VortexOwnerHooks.RegionOwnership))(Position(_pool.Slot(other), 14), Position(_pool.Slot(other), 18));
                    }
                }
            }
        }

        _base.Set(sid, owner);
        byte current = _pool.Slot(sid)[ownerOffset];
        byte extraAfter = _pool.Slot(sid)[extraOffset];
        if ((extraAfter & 9) == 0 && current != 0 && Read(Records(), TempleField(current)) == 0)
        {
            Write(Records(), TempleField(current), (uint)sid);
            Write(Records(), TempleField(current, true), (uint)sid);
        }

        Need(_hooks.RefreshProduction, nameof(VortexOwnerHooks.RefreshProduction))();
        if (patch)
        {
            _revision.Value = unchecked(_revision.Value + 1u);
        }
    }

    /// <summary>PATCH +7c/+80 및 CD +74/+78의 같은 초기 신전 두 DWORD다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="owner">조회할 소유자 번호다. 0~8이어야 한다.</param>
    /// <param name="copy">참이면 표시 복사 DWORD를 쓴다.</param>
    /// <returns>Player 기록 안의 바이트 위치다.</returns>
    private int TempleField(uint owner, bool copy = false)
    {
        if (owner > 8u)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "신전 Owner 실제 9 Player 범위 오류");
        }

        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        return (int)(owner * (patch ? 0xb4u : 0xacu) + (patch ? 0x7cu : 0x74u) + (copy ? 4u : 0u));
    }

    /// <summary>정렬되지 않은 원본 little endian DWORD를 읽는다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>결합한 DWORD다.</returns>
    private static uint Read(byte[] raw, int offset)
    {
        uint value = 0;
        // 이웃 필드를 섞지 않고 정확한 DWORD만 조합한다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }

        return value;
    }

    /// <summary>원본 Player DWORD만 저장하며 주변 바이트를 유지한다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="raw">쓸 바이트다.</param>
    /// <param name="offset">쓰기 시작 위치다.</param>
    /// <param name="value">쓸 DWORD다.</param>
    private static void Write(byte[] raw, int offset, uint value)
    {
        // 지정한 네 바이트만 낮은 비트부터 기록한다.
        for (int index = 0; index < 4; ++index)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>외부 효과 뒤의 현재 단정도 좌표를 읽는다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽기 시작 위치다.</param>
    /// <returns>비트 그대로의 float 값이다.</returns>
    private static float Position(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 낮은 바이트부터 조립하는 반복문이다.
        for (int index = 0; index < 4; ++index)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }

        return BitConverter.UInt32BitsToSingle(value);
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다. 2026-11-19 0.620에서 추가했다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식)이다.</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <param name="name">장치 이름이다.</param>
    /// <returns>연결된 장치다.</returns>
    private static T Need<T>(T? function, string name) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("신전 Owner 실제 장치 누락: " + name);
        }

        return function;
    }
}
