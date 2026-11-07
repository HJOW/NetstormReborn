// 다리·섬 연결과 연결 객체 생성·소유자 전파를 복원한다.
using System.Buffers.Binary;

namespace Netstorm.Core.Original;

// 다리·섬 연결 모듈을 담는 네임스페이스다. 2026-11-07 F2c 나머지 이식에서 추가됐다.

// 다리·섬 연결 본체를 복원한 클래스다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
public sealed class RawBridgeConnect
{
    /// <summary>소유자 색이 없을 때의 섬 프레임 번호다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    public const int NeutralOwnerFrame = 8;

    /// <summary>연결 객체 생성에 넘기는 플래그다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    public const uint ConnectCreateFlags = 2u;

    /// <summary>다리가 연결을 만드는 상대 표면의 분류 마스크다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    public const uint IslandSurfaceMask = 0x01000002u;

    /// <summary>섬 받침 클래스의 가상 표 기록값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const uint PatchIslandVtable = 0x00506d98u;

    /// <summary>섬 받침 클래스의 가상 표 기록값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const uint CdIslandVtable = 0x00506960u;

    /// <summary>섬 종유석 클래스의 가상 표 기록값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const uint PatchStalagVtable = 0x00506e50u;

    /// <summary>섬 종유석 클래스의 가상 표 기록값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const uint CdStalagVtable = 0x00506a18u;

    /// <summary>슬롯 안 둘째 참조의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int SecondReferenceOffset = 8;

    /// <summary>슬롯 안 타입 번호의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>슬롯 안 첫 참조의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int FirstReferenceOffset = 12;

    /// <summary>슬롯 안 현재 가로 좌표의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int XOffset = 14;

    /// <summary>슬롯 안 현재 세로 좌표의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int YOffset = 18;

    /// <summary>슬롯 안 화면 가로 좌표의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int ScreenXOffset = 22;

    /// <summary>슬롯 안 화면 세로 좌표의 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const int ScreenYOffset = 24;

    /// <summary>추가 비트의 추상·매립 마스크다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const byte AbstractOrBuriedMask = 9;

    /// <summary>중심과 사각형을 거의 올림하는 실제 값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private static readonly float RoundUpBias = BitConverter.UInt32BitsToSingle(0x3f7fff58u);

    /// <summary>방향별 가로 한 칸 이동량이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private static readonly float[] StepX = [0f, 1f, 1f, 1f, 0f, -1f, -1f, -1f];

    /// <summary>방향별 세로 한 칸 이동량이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private static readonly float[] StepY = [-1f, -1f, 0f, 1f, 1f, 1f, 0f, -1f];

    /// <summary>화면 가로 좌표의 칸 크기다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const double ScreenScaleX = 16.0;

    /// <summary>화면 세로 좌표의 칸 크기다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const double ScreenScaleY = 11.0;

    /// <summary>화면 좌표의 반올림 값이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const double ScreenBias = 0.5;

    /// <summary>한 칸 조회가 0단계 해시부터 찾는 타입의 분류다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private const uint SurfaceLevelGenus = TypeFlag2.Island | TypeFlag2.Bridge;

    /// <summary>슬롯을 읽는 풀이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>발자국에 인접한 표면·객체를 조회하는 검색기다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private readonly RawSquidNeighbors _neighbors;

    /// <summary>연결 함수들이 읽는 원본 전역이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private readonly BridgeConnectState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 묶음이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    private readonly BridgeConnectHooks _hooks;

    /// <summary>
    /// 풀·이웃 탐색기·상태는 이 객체보다 오래 살아야 한다. 이웃 탐색기는 같은 풀을 읽어야 하고 훅은 모두 연결돼 있어야 한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="neighbors">이웃 탐색기다.</param>
    /// <param name="state">연결 전역이다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <exception cref="ArgumentException">다리 연결의 풀 불일치·훅 미연결이다.</exception>
    public RawBridgeConnect(RawSidPool pool, RawSquidNeighbors neighbors, BridgeConnectState state, BridgeConnectHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(neighbors);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(neighbors.Pool, pool))
        {
            throw new ArgumentException("다리 연결의 이웃 탐색기 풀이 다릅니다", nameof(neighbors));
        }

        if (hooks.Create is null || hooks.SetOwner is null || hooks.Pop is null || hooks.SetFrame is null || hooks.NotifySurface is null)
        {
            throw new ArgumentException("다리 연결 훅이 모두 연결되어야 합니다", nameof(hooks));
        }

        _pool = pool;
        _neighbors = neighbors;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>커서·건설 연결기가 같은 실제 풀의 연결 객체를 생성하는지 확인한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <returns>이 몸체가 쓰는 풀이다.</returns>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 소유자의 색 프레임이다. 전투 모드이고 번호가 양수이면 색 표 값 - 1, 아니면 중립이다.
    /// </summary>
    /// <param name="state">연결 전역이다.</param>
    /// <param name="owner">소유자 번호다.</param>
    /// <returns>색 프레임 번호다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">소유자 색 표 범위 오류다.</exception>
    public static int OwnerColorFrame(BridgeConnectState state, int owner)
    {
        ArgumentNullException.ThrowIfNull(state);
        if (!state.Battle || owner <= 0)
        {
            return NeutralOwnerFrame;
        }

        if (owner >= state.OwnerColors.Length)
        {
            throw new ArgumentOutOfRangeException(nameof(owner), "소유자 색 표 범위 오류");
        }

        return unchecked((int)unchecked((uint)state.OwnerColors[owner] - 1u));
    }

    /// <summary>
    /// 기본 소유자 지정을 부른 뒤 프레임 필드를 소유자 색으로 직접 쓴다. 미션 중이고 현재 프레임이 중립이면 그대로 둔다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="state">연결 전역이다.</param>
    /// <param name="sid">지정할 번호다.</param>
    /// <param name="owner">쓸 소유자다.</param>
    /// <param name="inner">기본 소유자 지정이다.</param>
    public static void SetIslandOwner(RawSidPool pool, BridgeConnectState state, int sid, uint owner, Action<int, uint> inner)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        if (inner is null)
        {
            throw new ArgumentException("섬 소유자 지정의 기본 훅 누락", nameof(inner));
        }

        inner(sid, owner);
        if (state.Mission && ReadFrame(pool, sid) == NeutralOwnerFrame)
        {
            return;
        }

        WriteFrame(pool, sid, OwnerColorFrame(state, unchecked((int)owner)));
    }

    /// <summary>
    /// 가상 소유자 지정을 객체의 가상 표 기록값으로 분배한다. 섬 받침·종유석이면 재정의를, 아니면 기본을 부른다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="state">연결 전역이다.</param>
    /// <param name="inner">기본 소유자 지정이다.</param>
    /// <returns>분배한 소유자 지정이다.</returns>
    public static Action<int, uint> MakeOwnerDispatch(RawSidPool pool, BridgeConnectState state, Action<int, uint> inner)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(state);
        if (inner is null)
        {
            throw new ArgumentException("소유자 분배의 기본 훅 누락", nameof(inner));
        }

        bool patch = pool.Edition == OriginalEdition.Patch1078;
        uint island = patch ? PatchIslandVtable : CdIslandVtable;
        uint stalag = patch ? PatchStalagVtable : CdStalagVtable;
        return (sid, owner) =>
        {
            uint vtable = Read(pool.Slot(sid), 0, 4);
            if (vtable == island || vtable == stalag)
            {
                SetIslandOwner(pool, state, sid, owner, inner);
            }
            else
            {
                inner(sid, owner);
            }
        };
    }

    /// <summary>
    /// 좌표를 자른 한 칸에서 일반 탐색 순서로 지정 타입의 첫 객체를 찾는다. 없으면 0이다.
    /// </summary>
    /// <param name="x">가로 좌표다.</param>
    /// <param name="y">세로 좌표다.</param>
    /// <param name="type">찾을 타입 번호다.</param>
    /// <returns>찾은 번호다. 없으면 0이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">한 칸 조회 타입 범위 오류다.</exception>
    public int FindTypeAt(float x, float y, uint type)
    {
        IReadOnlyList<OriginalTypeRecord> types = _neighbors.Types;
        if (type >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "한 칸 조회 타입 범위 오류");
        }

        int cellX = Ftol(x);
        int cellY = Ftol(y);
        RawSquidFinder finder = new(_pool, _neighbors.Hash, types);
        // 원본은 현재 번호가 풀 범위 안인 동안 타입을 비교하고, 다르면 일반 다음으로 넘어가는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int current = finder.Begin(new SquidSearchArea(cellX, cellY, cellX, cellY), (types[(int)type].Flags2 & SurfaceLevelGenus) == 0u ? 1u : 0u); current != 0; current = finder.Next())
        {
            if (_pool.Slot(current)[TypeOffset] == type)
            {
                return current;
            }
        }

        return 0;
    }

    /// <summary>
    /// 첫 객체의 위치에서 둘째 객체의 중심 쪽으로 한 칸 옮긴 곳에 연결 객체를 만든다.
    /// </summary>
    /// <param name="first">기준 객체다.</param>
    /// <param name="second">상대 객체다.</param>
    /// <param name="owner">쓸 소유자다.</param>
    /// <param name="preview">미리보기 목록이다. 비우면 실제 등록한다.</param>
    /// <returns>만든 연결 객체다.</returns>
    public int Link(int first, int second, uint owner, List<int>? preview = null) => LinkImpl(first, second, owner, preview, null);

    /// <summary>
    /// 둘째 객체의 소유자를 첫 객체 쪽 섬 받침이 주인이 없을 때 전파한다.
    /// </summary>
    /// <param name="first">건네주는 객체다.</param>
    /// <param name="second">받는 쪽 객체다.</param>
    /// <exception cref="InvalidOperationException">주인 없는 받침에 줄 소유자가 없다.</exception>
    public void PropagateOwner(int first, int second)
    {
        int ownerOffset = OwnerOffset(_pool);
        int extraOffset = ExtraOffset(_pool);
        ReadOnlySpan<byte> target = _pool.Slot(second);
        int bottom = FindTypeAt(Coordinate(target, XOffset), Coordinate(target, YOffset), _state.IslandType);
        if ((_pool.Slot(first)[extraOffset] & AbstractOrBuriedMask) != 0 || bottom == 0)
        {
            return;
        }

        if (_pool.Slot(bottom)[ownerOffset] != 0 || (_pool.Slot(bottom)[extraOffset] & AbstractOrBuriedMask) != 0)
        {
            return;
        }

        uint owner = _pool.Slot(first)[ownerOffset];
        if (owner == 0u)
        {
            throw new InvalidOperationException("newPlayerId != INVALID_PLAYER_ID (Bridge.cpp assert)");
        }

        _hooks.SetOwner!(bottom, owner);
        _hooks.SetFrame!(bottom, OwnerColorFrame(_state, unchecked((int)owner)), 0u);
        ReadOnlySpan<byte> island = _pool.Slot(bottom);
        int stalag = FindTypeAt(Coordinate(island, XOffset), Coordinate(island, YOffset), _state.StalagType);
        if (stalag != 0)
        {
            _hooks.SetFrame!(stalag, Frame(bottom), 0u);
        }

        SquidFootprint foot = _neighbors.Footprint(bottom);
        RawSquidFinder finder = new(_pool, _neighbors.Hash, _neighbors.Types);
        // 발자국에 걸리는 모든 객체 가운데 무인도 타입에만 소유자를 주는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int current = finder.Begin(new SquidSearchArea(Ftol(foot.Left), Ftol(foot.Top), RoundUp(foot.Right), RoundUp(foot.Bottom))); current != 0; current = finder.Next())
        {
            if (_pool.Slot(current)[TypeOffset] == _state.NoIslandType)
            {
                _hooks.SetOwner!(current, owner);
            }
        }
    }

    /// <summary>
    /// 이웃을 순회하며 다리·섬 표면 쌍마다 연결 객체를 만들고 소유자를 전파한다.
    /// </summary>
    /// <param name="sid">기준 객체다.</param>
    /// <param name="preview">미리보기 목록이다. 비우면 실제 등록한다.</param>
    public void Connect(int sid, List<int>? preview = null) => ConnectImpl(sid, preview, null);

    /// <summary>
    /// 미리보기 번호를 생성 직후 목록 꼬리에 쓴다.
    /// </summary>
    /// <param name="sid">기준 객체다.</param>
    /// <param name="append">쓸 소비자다.</param>
    /// <exception cref="ArgumentException">다리 미리보기 번호 소비자 누락이다.</exception>
    public void ConnectPreview(int sid, Action<int> append)
    {
        if (append is null)
        {
            throw new ArgumentException("다리 미리보기 번호 소비자 누락", nameof(append));
        }

        ConnectImpl(sid, null, append);
    }

    /// <summary>
    /// 섬 받침의 최초 등록이면 같은 위치에 종유석을 만들고 소유자와 Pop을 준 뒤 연결 순회를 한다.
    /// </summary>
    /// <param name="island">섬 받침 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    public void IslandPostPopPrefix(int island, uint flags)
    {
        if ((flags & 1u) == 0u)
        {
            return;
        }

        int stalag = _hooks.Create!(_state.StalagType, ConnectCreateFlags);
        _hooks.SetOwner!(stalag, _pool.Slot(island)[OwnerOffset(_pool)]);
        ReadOnlySpan<byte> raw = _pool.Slot(island);
        _hooks.Pop!(stalag, Coordinate(raw, XOffset), Coordinate(raw, YOffset), 0u);
        Connect(island);
    }

    /// <summary>
    /// 접두 뒤에 같은 플래그로 공통 후처리를 부른다.
    /// </summary>
    /// <param name="island">섬 받침 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    /// <param name="inner">공통 후처리다.</param>
    /// <exception cref="ArgumentException">섬 받침 후처리의 공통 후처리 훅 누락이다.</exception>
    public void IslandPostPop(int island, uint flags, Action<int, uint>? inner)
    {
        if (_pool.Edition != OriginalEdition.Patch1078 && inner is null)
        {
            throw new ArgumentException("섬 받침 후처리의 공통 후처리 훅 누락", nameof(inner));
        }

        IslandPostPopPrefix(island, flags);
        if (inner is null)
        {
            throw new ArgumentException("섬 받침 후처리의 공통 후처리 훅 누락", nameof(inner));
        }

        inner(island, flags);
    }

    /// <summary>두 미리보기 방식이 동일한 순회·연결 몸체를 쓴다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="sid">기준 객체다.</param>
    /// <param name="preview">미리보기 목록이다.</param>
    /// <param name="append">쓸 소비자다.</param>
    private void ConnectImpl(int sid, List<int>? preview, Action<int>? append)
    {
        int ownerOffset = OwnerOffset(_pool);
        bool bridge = (Genus(sid) & TypeFlag2.Bridge) != 0u;
        uint mask = bridge ? IslandSurfaceMask : TypeFlag2.Bridge;
        RawSquidNeighborWalk walk = new(_neighbors, sid, NeighborFlags.SkipAbstract);
        // 연결 객체의 등록과 소유자 지정이 풀·해시를 바꾸므로 다음 이웃은 그 뒤의 상태에서 읽는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int other = walk.Current; other != 0; other = walk.Next())
        {
            if ((Genus(other) & mask) == 0u)
            {
                continue;
            }

            int first = bridge ? sid : other;
            int second = bridge ? other : sid;
            LinkImpl(first, second, _pool.Slot(first)[ownerOffset], preview, append);
            PropagateOwner(first, second);
        }
    }

    /// <summary>기존 목록과 실제 커서 배열이 같은 생성·좌표 쓰기 및 추가 시점을 공유한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="first">기준 객체다.</param>
    /// <param name="second">상대 객체다.</param>
    /// <param name="owner">쓸 소유자다.</param>
    /// <param name="preview">미리보기 목록이다.</param>
    /// <param name="append">쓸 소비자다.</param>
    /// <returns>만든 연결 객체다.</returns>
    private int LinkImpl(int first, int second, uint owner, List<int>? preview, Action<int>? append)
    {
        int born = _hooks.Create!(_state.ConnectorType, ConnectCreateFlags);
        ReadOnlySpan<byte> origin = _pool.Slot(first);
        float x = Coordinate(origin, XOffset);
        float y = Coordinate(origin, YOffset);
        float[] center = _neighbors.Center(second);
        float targetX = RoundUp(center[0]);
        float targetY = RoundUp(center[1]);
        int direction = RawSquidNeighbors.Direction(x, y, targetX, targetY);
        x = (float)((double)StepX[direction] + (double)x);
        y = (float)((double)StepY[direction] + (double)y);
        SetFrameField(born, direction / 2);
        _hooks.SetOwner!(born, owner);
        if (preview is null && append is null)
        {
            _hooks.Pop!(born, x, y, 0u);
            Span<byte> raw = _pool.RawSlot(born);
            Write(raw, FirstReferenceOffset, unchecked((uint)first), 2);
            if (raw[TypeOffset] == _state.BridgeType)
            {
                _hooks.NotifySurface!(born);
            }

            Write(_pool.RawSlot(born), SecondReferenceOffset, unchecked((uint)second), 2);
            return born;
        }

        Span<byte> previewRaw = _pool.RawSlot(born);
        Write(previewRaw, XOffset, BitConverter.SingleToUInt32Bits(x), 4);
        Write(previewRaw, YOffset, BitConverter.SingleToUInt32Bits(y), 4);
        Write(previewRaw, ScreenXOffset, unchecked((uint)Ftol((double)x * ScreenScaleX + ScreenBias)), 2);
        Write(previewRaw, ScreenYOffset, unchecked((uint)Ftol((double)y * ScreenScaleY + ScreenBias)), 2);
        if (append is not null)
        {
            append(born);
        }
        else
        {
            preview!.Add(born);
        }

        return born;
    }

    /// <summary>판본별 프레임 번호 필드를 읽는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>프레임 번호다.</returns>
    private int Frame(int sid) => ReadFrame(_pool, sid);

    /// <summary>판본별 프레임 번호 필드를 쓴다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="frame">쓸 프레임 번호다.</param>
    private void SetFrameField(int sid, int frame) => WriteFrame(_pool, sid, frame);

    /// <summary>객체 타입의 분류 비트다. 표 밖이면 거부한다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>분류 비트다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">다리 연결 타입 범위 오류다.</exception>
    private uint Genus(int sid)
    {
        IReadOnlyList<OriginalTypeRecord> types = _neighbors.Types;
        uint number = _pool.Slot(sid)[TypeOffset];
        if (number >= (uint)types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "다리 연결 타입 범위 오류");
        }

        return types[(int)number].Flags2;
    }

    /// <summary>판본별 소유자 필드 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <returns>바이트 위치다.</returns>
    private static int OwnerOffset(RawSidPool pool) => pool.Edition == OriginalEdition.Patch1078 ? 0x22 : 0x20;

    /// <summary>판본별 추가 비트 위치다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <returns>바이트 위치다.</returns>
    private static int ExtraOffset(RawSidPool pool) => pool.Edition == OriginalEdition.Patch1078 ? 0x28 : 0x23;

    /// <summary>판본별 프레임 번호 필드를 읽는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>프레임 번호다.</returns>
    private static int ReadFrame(RawSidPool pool, int sid)
    {
        ReadOnlySpan<byte> raw = pool.Slot(sid);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            return raw[0x22];
        }

        return BinaryPrimitives.ReadInt32LittleEndian(raw.Slice(0x24, 4));
    }

    /// <summary>판본별 프레임 번호 필드를 쓴다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="frame">쓸 프레임 번호다.</param>
    private static void WriteFrame(RawSidPool pool, int sid, int frame)
    {
        Span<byte> raw = pool.RawSlot(sid);
        if (pool.Edition != OriginalEdition.Patch1078)
        {
            raw[0x22] = (byte)frame;
            return;
        }

        BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(0x24, 4), frame);
    }

    /// <summary>정렬과 무관하게 리틀 엔디언으로 읽는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint value = 0u;
        // 낮은 바이트부터 합치는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>폭 밖의 바이트를 보존하면서 쓴다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 폭이다.</param>
    private static void Write(Span<byte> raw, int offset, uint value, int width)
    {
        // 낮은 바이트부터 쓰는 반복문이다. 2026-11-07 F2c 나머지 이식에서 추가됐다.
        for (int i = 0; i < width; ++i)
        {
            raw[offset + i] = (byte)(value >> (8 * i));
        }
    }

    /// <summary>바이트를 단정밀도 좌표로 읽는다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>좌표 값이다.</returns>
    private static float Coordinate(ReadOnlySpan<byte> raw, int offset) => BitConverter.UInt32BitsToSingle(Read(raw, offset, 4));

    /// <summary>원본 CRT 절삭의 하위 DWORD다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>칸 번호다.</returns>
    private static int Ftol(double value)
    {
        if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0))
        {
            return 0;
        }

        return unchecked((int)unchecked((uint)(long)value));
    }

    /// <summary>0.99999 계열 bias를 더한 x87 값을 자른 정수다. 2026-11-07 F2c 나머지 이식에서 추가됐다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>칸 번호다.</returns>
    private static int RoundUp(float value) => Ftol((double)value + RoundUpBias);
}
