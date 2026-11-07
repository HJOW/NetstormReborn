// 무인도 표면의 최초 등록과 3×3 받침 소유자 전파를 복원한다.
using System.Buffers.Binary;
using System.Text;

namespace Netstorm.Core.Original;

// 섬 후처리 모듈을 담는 네임스페이스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.

// 섬 후처리 Pop 처리 중 조회·갱신하는 상태 묶음이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandPostPopState
{
    /// <summary>요새 읽기 중이면 서·북 칸으로 프레임 글자를 선택한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public uint LoadingDepth { get; set; }

    /// <summary>받침 소유자 전파를 억제한다. 공유 저장 게임 값이 연결되면 읽지 않는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public bool OwnerPropagationSuppressed { get; set; }

    /// <summary>실제 Client가 소유한 현재 저장 게임 값이다. 연결되면 매번 현재 값을 직접 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<bool>? SavedGameSource { get; set; }

    /// <summary>현재 저장 게임 관문 값을 돌려준다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <returns>저장 게임 중이면 참이다.</returns>
    public bool SavedGame() => SavedGameSource?.Invoke() ?? OwnerPropagationSuppressed;
}

// 섬 후처리 Pop 외부 동작과 저장소 조회를 연결하는 묶음이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class IslandPostPopHooks
{
    /// <summary>기존 생성·가상 소유자·가상 Pop 효과를 공유한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public BridgeConnectHooks Objects { get; set; } = new();

    /// <summary>표면 칸의 연결 순회다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int>? Connect { get; set; }

    /// <summary>받침·종유석 조회다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<float, float, uint, int>? FindTypeAt { get; set; }

    /// <summary>소유자를 지정한 직후 표시 갱신이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<int>? UpdateDisplay { get; set; }

    /// <summary>표면 칸·받침·종유석 누락 시 원본의 진단·경계다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<bool, bool, bool>? IncompleteSupport { get; set; }

    /// <summary>원문 식·파일·줄을 전달하며 정상 복귀 뒤 현재 물리로 계속한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }

    /// <summary>받침 진단의 현재 문자열을 실제 상세 소유자에 복사한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Action<string>? Detail { get; set; }

    /// <summary>현재 DWORD이며 생략한 독립 입력은 최초 0이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public Func<uint>? TypeWas { get; set; }
}

// 무인도 최초 등록과 3×3 받침 소유자 전파를 복원한 클래스다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
public sealed class RawIslandPostPop
{
    /// <summary>슬롯 안 타입 번호의 위치다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int TypeOffset = 10;

    /// <summary>지도 한 변의 칸 수다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Side = 256;

    /// <summary>지도 전체 칸 수다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int Cells = Side * Side;

    /// <summary>첫 프레임 검색에 쓰는 변형이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const byte Variant = (byte)'P';

    /// <summary>첫 프레임 검색에 쓰는 번호다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const byte FrameNumber = 1;

    /// <summary>받침 묶음의 한 변이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private const int SupportSize = 3;

    /// <summary>표면 조회가 더하는 실제 값이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private static readonly float SurfaceBias = BitConverter.UInt32BitsToSingle(0x3f7ff972u);

    /// <summary>CD 받침 조회가 빼는 값이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private static readonly float CdHOffset = BitConverter.UInt32BitsToSingle(0x3f800347u);

    /// <summary>객체 슬롯 풀이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>표면 지도다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<ushort> _surfaceMap;

    /// <summary>소유자 지도다. 생략하면 표면 지도를 쓴다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<ushort> _ownerMap;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입별 프레임 자료다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>연결 함수들이 읽는 원본 전역이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly BridgeConnectState _typeState;

    /// <summary>처리 중 조회·갱신하는 상태다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IslandPostPopState _state;

    /// <summary>외부 저장소 조회와 가상 동작을 연결하는 묶음이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    private readonly IslandPostPopHooks _hooks;

    /// <summary>
    /// 지도는 256×256 번호 머리 배열이다. 고정 지도와 현재 지도가 달라질 수 있어 별도로 받는다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="surfaceMap">표면 지도 65,536칸이다.</param>
    /// <param name="types">타입 표다.</param>
    /// <param name="frames">타입별 프레임 자료다.</param>
    /// <param name="typeState">연결 전역이다.</param>
    /// <param name="state">처리 중 상태다.</param>
    /// <param name="hooks">외부 효과 묶음이다.</param>
    /// <param name="ownerMap">소유자 지도다. 비우면 표면 지도를 쓴다.</param>
    /// <exception cref="ArgumentException">무인도 후처리 지도·타입·훅 범위 오류다.</exception>
    public RawIslandPostPop(RawSidPool pool, IReadOnlyList<ushort> surfaceMap, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, BridgeConnectState typeState, IslandPostPopState state, IslandPostPopHooks hooks, IReadOnlyList<ushort>? ownerMap = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(surfaceMap);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(typeState);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        IReadOnlyList<ushort> owners = ownerMap is null || ownerMap.Count == 0 ? surfaceMap : ownerMap;
        if (surfaceMap.Count != Cells || owners.Count != Cells || types.Count != frames.Count)
        {
            throw new ArgumentException("무인도 후처리 지도·타입 표 범위 오류");
        }

        if (pool.Edition != OriginalEdition.Patch1078)
        {
            if (typeState.IslandType >= (uint)types.Count || typeState.NoIslandType >= (uint)types.Count || typeState.StalagType >= (uint)types.Count)
            {
                throw new ArgumentException("CD 무인도 후처리 타입 표 범위 오류");
            }

            if (hooks.Objects.Create is null || hooks.Objects.SetOwner is null || hooks.Objects.Pop is null
                || hooks.Connect is null || hooks.FindTypeAt is null || hooks.UpdateDisplay is null || hooks.IncompleteSupport is null)
            {
                throw new ArgumentException("CD 무인도 후처리 훅이 모두 연결되어야 합니다");
            }
        }

        _pool = pool;
        _surfaceMap = surfaceMap;
        _ownerMap = owners;
        _types = types;
        _frames = frames;
        _typeState = typeState;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>이 몸체가 쓰는 슬롯 풀이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 범위 안의 한 칸이 무인도면 프레임 방향 글자, 그 밖에는 0이다.
    /// </summary>
    /// <param name="x">격자 가로 좌표다.</param>
    /// <param name="y">격자 세로 좌표다.</param>
    /// <returns>방향 글자 또는 0이다.</returns>
    public byte SurfaceLetter(int x, int y)
    {
        int sid = SurfaceAt(x, y, _ownerMap);
        return sid != 0 && _pool.Slot(sid)[TypeOffset] == _typeState.NoIslandType ? (byte)Letter(sid) : (byte)0;
    }

    /// <summary>
    /// 최초 등록에서만 프레임 선택·받침 생성·소유자 전파·연결 순회를 수행한다.
    /// </summary>
    /// <param name="surface">표면 칸 번호다.</param>
    /// <param name="flags">호출 플래그다.</param>
    public void Prefix(int surface, uint flags)
    {
        if ((flags & 1u) == 0u)
        {
            return;
        }

        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        if (!patch && _pool.Slot(surface)[TypeOffset] >= (uint)_frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(surface), "무인도 후처리 타입 범위 오류");
        }

        if (_state.LoadingDepth != 0u)
        {
            double x = Coordinate(surface, 14);
            double y = Coordinate(surface, 18);
            byte west = SurfaceLetter(Cell(x - 1.0), Cell(y));
            byte north = SurfaceLetter(Cell(patch ? Coordinate(surface, 14) : x), Cell((patch ? Coordinate(surface, 18) : y) - 1.0));
            byte letter = NextLetter(west, north);
            ReadOnlySpan<byte> raw = _pool.Slot(surface);
            if (raw[TypeOffset] >= (uint)_frames.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(surface), "무인도 선택 프레임 타입 범위 오류");
            }

            int frame = _frames[raw[TypeOffset]].FindNumber(letter, Variant, FrameNumber);
            if (frame < 0)
            {
                if (!patch)
                {
                    throw new InvalidOperationException("무인도 방향에 해당하는 첫 프레임이 없습니다");
                }

                SurfaceNeed(_hooks.Assertion)("0", "\\Ns\\O\\Rifttype.cpp", 299u);
            }

            WriteFrame(surface, frame);
            if (patch && Letter(surface) != letter)
            {
                SurfaceNeed(_hooks.Assertion)("getOrientation() == orientation", "\\Ns\\O\\Construction.cpp", 0xfau);
            }
        }

        uint ownerOffset = _pool.Edition == OriginalEdition.Patch1078 ? 34u : 32u;
        if (Letter(surface) == (byte)'F')
        {
            int island = SurfaceNeed(_hooks.Objects.Create)(_typeState.IslandType, BridgeConnectValues.ConnectCreateFlags);
            if (patch)
            {
                if (_typeState.IslandType >= (uint)_types.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(surface), "무인도 생성 뒤 받침 타입 범위 오류");
                }

                OriginalTypeRecord type = _types[(int)_typeState.IslandType];
                SurfaceNeed(_hooks.Objects.SetOwner)(island, _pool.Slot(surface)[(int)ownerOffset]);
                SurfaceNeed(_hooks.Objects.Pop)(island, (float)(FootDelta(type.FootX) + Coordinate(surface, 14)), (float)(FootDelta(type.FootY) + Coordinate(surface, 18)), 0u);
            }
            else
            {
                _hooks.Objects.SetOwner!(island, _pool.Slot(surface)[(int)ownerOffset]);
                OriginalTypeRecord type = _types[(int)_typeState.IslandType];
                _hooks.Objects.Pop!(island, (float)((double)Coordinate(surface, 14) + type.FootX - 1), (float)((double)Coordinate(surface, 18) + type.FootY - 1), 0u);
            }
        }

        if (!_state.SavedGame() && Letter(surface) == (byte)'H')
        {
            float x = Coordinate(surface, 14);
            float y = Coordinate(surface, 18);
            int HCell(float value) => patch ? Cell((double)(float)((double)value - 2) + SurfaceBias) : Cell((double)value - CdHOffset);
            int first = SurfaceAt(HCell(x), HCell(y), _surfaceMap);
            if (first == 0)
            {
                if (patch)
                {
                    SurfaceNeed(_hooks.Assertion)("gSid", "\\Ns\\O\\Construction.cpp", 0x10au);
                }
            }
            else
            {
                SetSupportOwner(x, y, _pool.Slot(first)[(int)ownerOffset]);
            }
        }

        SurfaceNeed(_hooks.Connect)(surface);
    }

    /// <summary>
    /// 무인도인 경우 오른쪽 아래 기준점의 3×3 칸을 갱신한다. 이어 받침과 종유석을 갱신한다.
    /// </summary>
    /// <param name="x">기준 가로 좌표다.</param>
    /// <param name="y">기준 세로 좌표다.</param>
    /// <param name="owner">쓸 소유자다.</param>
    public void SetSupportOwner(float x, float y, uint owner)
    {
        int right = Cell(x);
        int bottom = Cell(y);
        int target = SurfaceAt(Cell((double)x + SurfaceBias), Cell((double)y + SurfaceBias), _surfaceMap);
        if (target == 0 || _pool.Slot(target)[TypeOffset] != _typeState.NoIslandType)
        {
            return;
        }

        bool missingCells = false;
        StringBuilder detail = new();
        // 원본은 가로를 바깥 반복문, 세로를 안쪽 반복문으로 순회한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
        for (int cx = right - SupportSize + 1; cx <= right; ++cx)
        {
            // 타입을 제한하지 않고 각 칸의 현재 머리 객체에 소유자와 표시 갱신을 주는 반복문이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.
            for (int cy = bottom - SupportSize + 1; cy <= bottom; ++cy)
            {
                int sid = SurfaceAt(cx, cy, _ownerMap);
                if (sid == 0)
                {
                    missingCells = true;
                    detail.Append('-');
                    continue;
                }

                SurfaceNeed(_hooks.Objects.SetOwner)(sid, owner);
                SurfaceNeed(_hooks.UpdateDisplay)(sid);
                uint type = _pool.Slot(sid)[TypeOffset];
                detail.Append(type == _typeState.NoIslandType ? "X" : "[" + type + "]");
            }
        }

        int island = SurfaceNeed(_hooks.FindTypeAt)(x, y, _typeState.IslandType);
        if (island != 0)
        {
            SurfaceNeed(_hooks.Objects.SetOwner)(island, owner);
            SurfaceNeed(_hooks.UpdateDisplay)(island);
        }
        else
        {
            detail.Append(" (No islandSid) ");
        }

        int stalag = SurfaceNeed(_hooks.FindTypeAt)(x, y, _typeState.StalagType);
        if (stalag != 0)
        {
            SurfaceNeed(_hooks.Objects.SetOwner)(stalag, owner);
            SurfaceNeed(_hooks.UpdateDisplay)(stalag);
        }
        else
        {
            detail.Append(" (No islandStalagSid) ");
        }

        if (!(missingCells || island == 0 || stalag == 0))
        {
            return;
        }

        if (_pool.Edition != OriginalEdition.Patch1078 || (_hooks.Detail is null && _hooks.Assertion is null))
        {
            SurfaceNeed(_hooks.IncompleteSupport)(missingCells, island == 0, stalag == 0);
            return;
        }

        uint typeWas = _hooks.TypeWas is not null ? SurfaceNeed(_hooks.TypeWas)() : 0u;
        if (typeWas != 0u)
        {
            detail.Append(" TypeWas ").Append(unchecked((int)typeWas));
        }

        SurfaceNeed(_hooks.Detail)(detail.ToString());
        SurfaceNeed(_hooks.Assertion)("0", "\\Ns\\O\\Damageable.cpp", 0x18bu);
    }

    /// <summary>현재 프레임 코드의 방향 글자를 검증해 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 번호다.</param>
    /// <returns>방향 글자다.</returns>
    private int Letter(int sid)
    {
        if (_pool.Edition == OriginalEdition.Patch1078 && _pool.Slot(sid)[TypeOffset] < 70u)
        {
            SurfaceNeed(_hooks.Assertion)("!isForm()", "\\Ns\\O\\Squid.cpp", 0x854u);
        }

        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[TypeOffset] >= (uint)_frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "무인도 프레임 타입 범위 오류");
        }

        IReadOnlyList<RiftFrameCode> codes = _frames[raw[TypeOffset]].Codes;
        int frame = _pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw.Slice(36, 4)) : raw[34];
        if (frame < 0 || frame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "무인도 프레임 번호 범위 오류");
        }

        return _pool.Edition == OriginalEdition.Patch1078 ? (sbyte)codes[frame].Side : codes[frame].Side;
    }

    /// <summary>지도 밖은 0, 지도 안은 머리 번호 그대로다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="x">격자 가로 좌표다.</param>
    /// <param name="y">격자 세로 좌표다.</param>
    /// <param name="map">읽을 지도다.</param>
    /// <returns>머리 번호다.</returns>
    private static int SurfaceAt(int x, int y, IReadOnlyList<ushort> map)
    {
        if (x < 0 || y < 0 || x >= Side || y >= Side)
        {
            return 0;
        }

        return map[y * Side + x];
    }

    /// <summary>북쪽 칸에 따른 다음 행의 첫 글자다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="north">북쪽 글자다.</param>
    /// <returns>고른 글자다.</returns>
    private static byte FromNorth(byte north) => north == (byte)'F' ? (byte)'B' : north == (byte)'B' ? (byte)'I' : (byte)'F';

    /// <summary>원본 분기 순서대로 서쪽·북쪽 칸의 글자로 자신의 글자를 정한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="west">서쪽 글자다.</param>
    /// <param name="north">북쪽 글자다.</param>
    /// <returns>고른 글자다.</returns>
    private static byte NextLetter(byte west, byte north)
    {
        return west switch
        {
            (byte)'C' => (byte)'G',
            (byte)'F' => (byte)'C',
            (byte)'A' => (byte)'D',
            (byte)'B' => (byte)'A',
            (byte)'E' => (byte)'H',
            (byte)'I' => (byte)'E',
            _ => FromNorth(north),
        };
    }

    /// <summary>원본 _ftol의 하위 DWORD다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="value">자를 값이다.</param>
    /// <returns>칸 번호다.</returns>
    private static int Cell(double value)
    {
        if (!(value > -9223372036854775808.0 && value < 9223372036854775808.0))
        {
            return 0;
        }

        return unchecked((int)unchecked((uint)(long)value));
    }

    /// <summary>현재 슬롯 좌표를 단정밀도로 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="sid">조회할 번호다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>좌표 값이다.</returns>
    private float Coordinate(int sid, int offset) => BitConverter.UInt32BitsToSingle(Read(_pool.Slot(sid), offset));

    /// <summary>프레임 필드의 폭만큼 쓰며 주변 바이트는 보존한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="sid">쓸 번호다.</param>
    /// <param name="frame">쓸 프레임 값이다.</param>
    private void WriteFrame(int sid, int frame)
    {
        Span<byte> raw = _pool.RawSlot(sid);
        if (_pool.Edition != OriginalEdition.Patch1078)
        {
            raw[34] = (byte)frame;
            return;
        }

        BinaryPrimitives.WriteInt32LittleEndian(raw.Slice(36, 4), frame);
    }

    /// <summary>호스트 정렬에 의존하지 않고 DWORD를 읽는다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="raw">슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset) => BinaryPrimitives.ReadUInt32LittleEndian(raw.Slice(offset, 4));

    /// <summary>10진 발자국의 x87 정수 변환이다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <param name="footprint">발자국 값이다.</param>
    /// <returns>좌표 덧셈에 올릴 값이다.</returns>
    private static double FootDelta(int footprint) => (double)unchecked((int)unchecked((uint)footprint - 1u));

    /// <summary>도달한 장치만 복사해 요구한다. 2026-11-07 R4 지형·섬 생성 2차 이식에서 추가됐다.</summary>
    /// <typeparam name="T">장치 대리자 형식이다.</typeparam>
    /// <param name="callback">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">표면 후처리의 선택 장치 미연결이다.</exception>
    private static T SurfaceNeed<T>(T? callback)
        where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("표면 후처리의 선택 장치 미연결");
        }

        return callback;
    }
}
