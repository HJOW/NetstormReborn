namespace Netstorm.Core.Original;

/// <summary>
/// 신전 삭제가 읽고 갱신하는 로딩 깊이(원본 005c89b8). 0 일 때만 생산 표시 갱신을 요구한다.
/// 외부 효과(삭제 하위 몸체)가 값을 바꿀 수 있어 매번 현재 값을 다시 읽는다.
/// </summary>
public sealed class VortexPreDestroyState
{
    /// <summary>현재 로딩 깊이. 0 이면 로딩 중이 아니어서 생산 갱신 대상이다.</summary>
    public uint Loading { get; set; }
}

/// <summary>
/// 신전 삭제 전처리가 바깥에 요구하는 실제 하위·지역·공통 경계 묶음(원본 VortexPreDestroyHooks).
/// 목록 제거 뒤의 섬·지역 효과와 공통 Damageable·생산 갱신·진단 다섯 갈래를 나눈다.
/// </summary>
public sealed class VortexPreDestroyHooks
{
    /// <summary>현재 좌표의 섬 번호(원본 0046f530). 삭제 전 좌표로 조회한다.</summary>
    public Func<float, float, int>? IslandNumber { get; set; }

    /// <summary>섬 존재 판정(원본 0046ee30). 거짓이면 진단 0x2ab 을 요구한다.</summary>
    public Func<int, bool>? IslandExists { get; set; }

    /// <summary>섬의 현재 신전 SID(원본 0046f260). 삭제 번호와 다르면 진단 0x2ac 을 요구한다.</summary>
    public Func<int, uint>? IslandTemple { get; set; }

    /// <summary>섬 신전 지우기와 생산 갱신(원본 004db890 region,0). 존재 검사→신전 0→생산 갱신 전체다.</summary>
    public Action<int>? ClearIslandTemple { get; set; }

    /// <summary>섬 소유자·거주 구역 전파(원본 0046ef10). 신전을 지운 섬을 0번 소유로 돌린다.</summary>
    public Action<int, uint>? IslandOwner { get; set; }

    /// <summary>같은 현재 지도·공장 투표의 지역 소유권 갱신(원본 00455380). 삭제 전후 좌표로 두 번 부른다.</summary>
    public Action<float, float>? RegionOwnership { get; set; }

    /// <summary>실제 해방·공통 사전 파괴(원본 0044b4b0). 목록·지역 효과 뒤에 같은 플래그로 부른다.</summary>
    public Action<int, uint>? DamageablePre { get; set; }

    /// <summary>현재 생산 표시 갱신(원본 0047d890). 로딩 0 이고 가림 비트가 없을 때만 부른다.</summary>
    public Action? RefreshProduction { get; set; }

    /// <summary>진단 줄 번호 통지(원본 Vortex.cpp). 0x2ab·0x2ac 뒤에도 계속 진행한다.</summary>
    public Action<uint>? Assertion { get; set; }
}

/// <summary>
/// 신전의 실제 가상 사전 파괴(원본 004dca50, RawVortexPreDestroy, 10.78).
/// 순서는 원본 그대로 "목록 제거→섬 조회·진단→섬 지우기·소유자 전파→지역 갱신→outpost 지역 갱신→공통 Damageable→생산 갱신→현재 Player 4필드 정리"다.
/// 효과 사이마다 부모·후보의 현재 소유자와 좌표를 다시 읽어 콜백이 바꾼 값을 반영한다.
/// 기준 판본은 10.78 뿐이다.
/// 근거: cpppj/src/o/RawVortexPreDestroy.cpp (W03 신전 삭제 조각, 2026-10-23).
/// </summary>
public sealed class RawVortexPreDestroy
{
    /// <summary>슬롯 안 소유자 바이트(1바이트)의 위치(10.78).</summary>
    private const int OwnerOffset = 34;

    /// <summary>슬롯 안 x 좌표(float 4바이트)의 위치.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 y 좌표(float 4바이트)의 위치.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 추가 비트 바이트의 위치(10.78). 9(0x09)가 켜지면 섬·지역 효과를 건너뛴다.</summary>
    private const int ExtraOffset = 40;

    /// <summary>가림·매림 마스크. 이 비트가 하나라도 있으면 목록 제거 뒤 효과를 건너뛴다.</summary>
    private const byte SkipMask = 9;

    /// <summary>소유자 바이트가 가질 수 있는 최대값 + 1. 9 이상이면 호스트 오류다.</summary>
    private const int OwnerCount = 9;

    /// <summary>한 Player 기록의 바이트 수(원본 0xb4). 소유자 번호에 곱해 필드 주소를 구한다.</summary>
    private const int PlayerStride = 0xb4;

    /// <summary>Player 안에서 지운 신전을 가리키던 첫 번째 필드(원본 +0x7c).</summary>
    private const int PlayerFieldFirst = 0x7c;

    /// <summary>첫 번째 필드를 지운 뒤 복사해 넣는 표시 필드(원본 +0x80).</summary>
    private const int PlayerFieldFirstMirror = 0x80;

    /// <summary>Player 안에서 지운 신전을 가리키던 두 번째 필드(원본 +0x84).</summary>
    private const int PlayerFieldSecond = 0x84;

    /// <summary>두 번째 필드를 지운 뒤 복사해 넣는 표시 필드(원본 +0x88).</summary>
    private const int PlayerFieldSecondMirror = 0x88;

    /// <summary>존재하지 않는 섬을 지울 때 요구하는 진단 줄 번호(원본 0x2ab).</summary>
    private const uint AssertMissingIsland = 0x2ab;

    /// <summary>섬 신전이 삭제 번호와 다를 때 요구하는 진단 줄 번호(원본 0x2ac).</summary>
    private const uint AssertTempleMismatch = 0x2ac;

    /// <summary>outpost SID 가 가질 수 있는 최대 16비트 값. 이를 넘으면 범위 오류다.</summary>
    private const uint MaxSid16 = 65535;

    /// <summary>삭제되는 신전과 outpost 가 공유하는 실제 풀.</summary>
    private readonly RawSidPool _pool;

    /// <summary>원본 9×0xb4 의 같은 Player 기록. 정리 단계에서 현재 값을 직접 고친다.</summary>
    private readonly byte[] _players;

    /// <summary>현재 신전 등록 목록. 앞방향 압축으로 삭제 번호를 모두 뺀다.</summary>
    private readonly RawPostPopList _temples;

    /// <summary>현재 거주 구역 등록 목록. 신전 목록과 같은 방식으로 압축한다.</summary>
    private readonly RawPostPopList _residences;

    /// <summary>효과 뒤 현재 값으로 읽는 outpost 목록. 개체 소유자가 같을 때만 지역 갱신을 부른다.</summary>
    private readonly RawPostPopList _outposts;

    /// <summary>현재 로딩 깊이를 가진 상태. 콜백이 바꾼 값을 효과 뒤에 다시 읽는다.</summary>
    private readonly VortexPreDestroyState _state;

    /// <summary>섬·지역·공통·생산·진단의 실제 하위 소비자.</summary>
    private readonly VortexPreDestroyHooks _hooks;

    /// <summary>
    /// 신전 삭제를 만든다. 풀·세 목록·로딩 상태는 이 객체보다 오래 살아야 한다.
    /// </summary>
    /// <param name="pool">삭제되는 신전과 outpost 가 공유하는 풀.</param>
    /// <param name="players">원본 9×0xb4 Player 기록. 길이가 1620 이어야 한다.</param>
    /// <param name="temples">현재 신전 등록 목록.</param>
    /// <param name="residences">현재 거주 구역 등록 목록.</param>
    /// <param name="outposts">효과 뒤 현재 값으로 읽는 outpost 목록.</param>
    /// <param name="state">현재 로딩 깊이 상태.</param>
    /// <param name="hooks">섬·지역·공통·생산·진단 경계.</param>
    public RawVortexPreDestroy(RawSidPool pool, byte[] players, RawPostPopList temples,
        RawPostPopList residences, RawPostPopList outposts, VortexPreDestroyState state,
        VortexPreDestroyHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(players);
        ArgumentNullException.ThrowIfNull(temples);
        ArgumentNullException.ThrowIfNull(residences);
        ArgumentNullException.ThrowIfNull(outposts);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        // 원본 PATCH 의 실제 Player 저장소 크기와 다르면 진입 전에 거부한다
        if (players.Length != OwnerCount * PlayerStride)
        {
            throw new ArgumentException("신전 삭제의 실제 PATCH/Player 저장소 오류", nameof(players));
        }
        _pool = pool;
        _players = players;
        _temples = temples;
        _residences = residences;
        _outposts = outposts;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀. 연결 어댑터가 참조 대상을 확인할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 원본 004dca50 전체의 목록 제거→지역→공통 Damageable→현재 UI→현재 Player 4필드 순서다.
    /// </summary>
    /// <param name="sid">삭제되는 신전 번호.</param>
    /// <param name="flags">삭제에 적용되는 플래그 비트. 공통 하위에 그대로 넘긴다.</param>
    public void PreDestroy(int sid, uint flags)
    {
        // 가림 비트가 없으면 섬·지역 효과를 먼저 수행한다
        if ((_pool.Slot(sid)[ExtraOffset] & SkipMask) == 0)
        {
            Remove(_temples, sid);
            Remove(_residences, sid);
            (float X, float Y) point = Position(sid);
            int island = Need(_hooks.IslandNumber, "섬 번호 조회").Invoke(point.X, point.Y);
            // 존재하지 않는 섬이면 진단을 남기고 계속 진행한다
            if (!Need(_hooks.IslandExists, "섬 존재 판정").Invoke(island))
            {
                Need(_hooks.Assertion, "진단 통지").Invoke(AssertMissingIsland);
            }
            // 섬 신전이 삭제 번호와 다르면 진단을 남기고 계속 진행한다
            if (Need(_hooks.IslandTemple, "섬 신전 조회").Invoke(island) != (uint)sid)
            {
                Need(_hooks.Assertion, "진단 통지").Invoke(AssertTempleMismatch);
            }
            Need(_hooks.ClearIslandTemple, "섬 신전 지우기").Invoke(island);
            Need(_hooks.IslandOwner, "섬 소유자 전파").Invoke(island, 0);
            // 콜백이 좌표를 바꿨을 수 있어 지역 갱신 전에 다시 읽는다
            (float X, float Y) now = Position(sid);
            Need(_hooks.RegionOwnership, "지역 소유권 갱신").Invoke(now.X, now.Y);
            // outpost 목록 count 와 부모·후보 소유자를 효과 이후 현재 값으로 검사한다
            int count = checked((int)_outposts.Count);
            // 목록 개수만큼 같은 소유자의 outpost 좌표마다 지역 갱신을 부른다
            for (int index = 0; index < count; index++)
            {
                uint value = _outposts.Entries[index];
                // 풀을 벗어나거나 16비트를 넘는 번호는 쓰기 전에 거부한다
                if (value >= (uint)_pool.Capacity || value > MaxSid16)
                {
                    throw new ArgumentOutOfRangeException(nameof(sid), value, "신전 삭제의 outpost SID 범위 오류");
                }
                int current = (int)(ushort)value;
                // 부모와 같은 소유자의 outpost 만 지역 갱신 대상이다
                if (_pool.Slot(current)[OwnerOffset] == _pool.Slot(sid)[OwnerOffset])
                {
                    (float X, float Y) outpost = Position(current);
                    Need(_hooks.RegionOwnership, "지역 소유권 갱신").Invoke(outpost.X, outpost.Y);
                }
            }
        }
        Need(_hooks.DamageablePre, "실제 하위와 공통 사전 파괴").Invoke(sid, flags);
        // 로딩 중이 아니고 가림 비트가 없으면 생산 표시 갱신을 부른다
        if (_state.Loading == 0 && (_pool.Slot(sid)[ExtraOffset] & SkipMask) == 0)
        {
            Need(_hooks.RefreshProduction, "생산 표시 갱신").Invoke();
        }
        // 중립(0번)이 아니면 현재 Player 의 두 신전 참조를 정리한다
        if (_pool.Slot(sid)[OwnerOffset] != 0)
        {
            // 첫 번째 참조가 삭제 번호면 지우고 표시 필드에 같은 값을 복사한다
            if (Read(_players, Field(sid, PlayerFieldFirst)) == (uint)sid)
            {
                Put(_players, Field(sid, PlayerFieldFirst), 0);
                uint value = Read(_players, Field(sid, PlayerFieldFirst));
                Put(_players, Field(sid, PlayerFieldFirstMirror), value);
            }
            // 두 번째 참조가 삭제 번호면 지우고 표시 필드에 같은 값을 복사한다
            if (Read(_players, Field(sid, PlayerFieldSecond)) == (uint)sid)
            {
                Put(_players, Field(sid, PlayerFieldSecond), 0);
                uint value = Read(_players, Field(sid, PlayerFieldSecond));
                Put(_players, Field(sid, PlayerFieldSecondMirror), value);
            }
        }
    }

    /// <summary>
    /// 원본 removeAll 과 같은 앞방향 압축이며 사용하지 않는 물리 꼬리는 보존한다.
    /// </summary>
    /// <param name="list">압축할 목록.</param>
    /// <param name="sid">뺄 슬롯 번호.</param>
    private static void Remove(RawPostPopList list, int sid)
    {
        // 손상된 개수는 목록을 쓰기 전에 거부한다
        _ = list.Items().Length;
        uint removed = 0;
        // 생존 항목은 최초 상대 순서를 유지한다. 개수 밖 값은 읽지도 쓰지도 않는다.
        for (uint index = 0; index < list.Count; index++)
        {
            uint current = list.Entries[(int)index];
            // 삭제 번호와 같으면 세지 않고, 다르면 앞으로 당긴다
            if (current == (uint)sid)
            {
                removed++;
            }
            else
            {
                list.Entries[(int)(index - removed)] = current;
            }
        }
        list.Count -= removed;
    }

    /// <summary>
    /// 해당 효과 직전의 raw 좌표만 반환한다.
    /// </summary>
    /// <param name="sid">읽을 슬롯 번호.</param>
    private (float X, float Y) Position(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        // 네 바이트를 낮은 바이트부터 합쳐 float 비트로 읽는다
        return (BitConverter.UInt32BitsToSingle(Read(raw, XOffset)), BitConverter.UInt32BitsToSingle(Read(raw, YOffset)));
    }

    /// <summary>
    /// 현재 raw 소유자의 해당 Player DWORD 주소다. 범위 밖은 호스트 오류다.
    /// </summary>
    /// <param name="sid">소유자를 읽을 신전 번호.</param>
    /// <param name="field">Player 기록 안의 필드 위치(0x7c·0x80·0x84·0x88).</param>
    private int Field(int sid, int field)
    {
        // 현재 소유자를 매번 다시 읽어 진단·효과가 바꾼 기록을 반영한다
        byte owner = _pool.Slot(sid)[OwnerOffset];
        // 9 이상이면 원본 Player 배열을 벗어나므로 쓰기 전에 거부한다
        if (owner >= OwnerCount)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), owner, "신전 삭제의 현재 Player 범위 오류");
        }
        return owner * PlayerStride + field;
    }

    /// <summary>슬롯 안의 정렬되지 않은 값을 필요한 폭만큼 읽는다.</summary>
    /// <param name="bytes">슬롯 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        // 네 바이트를 낮은 바이트부터 합친다
        return (uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    /// <summary>Player 기록의 정렬되지 않은 DWORD 를 읽는다.</summary>
    /// <param name="bytes">Player 바이트.</param>
    /// <param name="offset">읽기 시작 위치.</param>
    private static uint Read(byte[] bytes, int offset)
    {
        // 네 바이트를 낮은 바이트부터 합친다
        return (uint)(bytes[offset] | (bytes[offset + 1] << 8) | (bytes[offset + 2] << 16) | (bytes[offset + 3] << 24));
    }

    /// <summary>Player 기록에 DWORD 를 리틀 엔디언으로 쓴다.</summary>
    /// <param name="bytes">Player 바이트.</param>
    /// <param name="offset">쓰기 시작 위치.</param>
    /// <param name="value">쓸 값.</param>
    private static void Put(byte[] bytes, int offset, uint value)
    {
        // 네 바이트를 낮은 바이트부터 쓴다
        for (int index = 0; index < 4; index++)
        {
            bytes[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>선택 위치의 미연결 효과는 앞선 원문 쓰기를 보존한 채 명시 거부한다.</summary>
    /// <param name="value">확인할 경계.</param>
    /// <param name="name">누락 진단에 쓸 경계 이름.</param>
    private static T Need<T>(T? value, string name)
        where T : class
    {
        // null 경계를 쓰기 전에 거부해 앞선 목록·지역 효과를 보존한다
        if (value is null)
        {
            throw new InvalidOperationException("신전 삭제의 실제 하위 소비자 미연결: " + name);
        }
        return value;
    }
}
