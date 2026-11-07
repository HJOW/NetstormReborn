// 다리 수명·직접 프레임 쓰기와 실제 효과 호출 순서를 복원한다.
using Netstorm.Core.Bridges;
using System.Globalization;

namespace Netstorm.Core.Original;

// 붕괴 몸체 모듈을 담는 네임스페이스다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.

// 가상 삭제·표시·무상태 표면 진단·이동체 확인·위치 소리만 같은 현재 월드에서 공급한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.
public sealed class RawBridgeDecayHooks
{
    /// <summary>실제 가상 삭제다. 거부·재귀 삭제도 호출자가 처리한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>수명 WORD를 쓴 뒤 호출한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>가상 화면 갱신이다. 프레임 Set·Unpop·Pop으로 바꾸지 않는다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    public Action<int>? Refresh { get; set; }

    /// <summary>방문 칸 처리 뒤 현재 칸 위 이동체를 확인한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    public Action<int>? Carriers { get; set; }

    /// <summary>방문 칸 처리 뒤 원천 현재 위치에서 위치 소리를 재생한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    public Action<int>? CrackSound { get; set; }

    /// <summary>현재 checking에서만 선택하는 최소 수명 진단이다. 복귀 뒤 제한·쓰기를 계속한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    public Action<string, string, uint>? Assertion { get; set; }
}

// 원문 한 칸은 이웃·방문 목록을 먼저 모은 뒤 실제 슬롯에 순서대로 수명을 적용한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.
public sealed class RawBridgeDecay
{
    /// <summary>원문 Squid +0xc의 3~6비트가 다리 수명이다. 정상 범위는 0~7이다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const int LifeMask = 0x78;

    /// <summary>원문 J/K 판자 제거 시 가상 함수에 넘기는 플래그다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const uint PlankRemoval = 0x02000000u;

    /// <summary>수명이 이 값 아래로 처음 내려가면 금 간 프레임으로 바꾼다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const int CrackLife = 5;

    /// <summary>수명이 이 값 이하가 되면 칸 위 이동체를 확인한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const int CarrierLife = 5;

    /// <summary>한 칸 처리가 그래프 표면 수가 이 값보다 작은 다리를 곧바로 없앤다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const int MinimumGraphSurfaces = 5;

    /// <summary>프레임 코드 플래그: 단단한 프레임. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const int HardFrameFlag = 0x40;

    /// <summary>무효 그래프 번호다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private const int InvalidGraph = 254;

    /// <summary>WORD·프레임이 실제로 바뀌는 단일 50바이트 풀이다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 종류·발자국 표다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>현재 종류의 방향·금·단단 프레임 코드 표다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>flag 8과 열린 끝이 함께 읽는 실제 머리 지도다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<ushort> _surface;

    /// <summary>내부 AND·후보 기준점의 현재 비트다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<byte> _spots;

    /// <summary>Graph 분할·반납의 변경도 바로 보이는 255행 배열이다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly IReadOnlyList<GraphRecord> _graphs;

    /// <summary>같은 현재 server·authority·editor·개발용 표시 상태를 읽는다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly Func<BridgeDecayMode> _mode;

    /// <summary>필요한 효과다. 도달 시점에 검사하며 앞선 실제 쓰기를 보존한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly RawBridgeDecayHooks _hooks;

    /// <summary>현재 Client 서버 전역의 주소만 빌리며 별도 사본을 만들지 않는다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly RawAuthorityState? _authority;

    /// <summary>최소 수명 선택 뒤 읽는 같은 Client DWORD의 비소유 뷰다. 독립 입력 기본값은 0이다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private readonly RawCheckingFlag _checking;

    /// <summary>방문 목록과 두 전역 플래그다. 실제 수명·그림·삭제는 별도 처리한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
    private sealed class DecayWalk
    {
        /// <summary>방문 목록이다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
        public List<int> Visited = [];

        /// <summary>짧은 접합 여부다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
        public bool ShortJunction;

        /// <summary>가능 붕괴 여부다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
        public bool CanDecay;

        /// <summary>원문 무한 재귀할 접합 고리·과도한 깊이는 새 코드에서 중단한다. 2026-11-07 F2d 붕괴 몸체 이식에서 추가됐다.</summary>
        public bool Complete = true;
    }

    /// <summary>초기화·구성은 저장된 수명·스캔·Graph를 수정하지 않는다.</summary>
    /// <param name="pool">WORD·프레임이 바뀌는 풀이다.</param>
    /// <param name="types">현재 종류·발자국 표다.</param>
    /// <param name="frames">현재 종류의 프레임 코드 표다.</param>
    /// <param name="surface">실제 머리 지도 65,536칸이다.</param>
    /// <param name="spots">spot 지도 65,536칸이다.</param>
    /// <param name="graphs">Graph 255행 배열이다.</param>
    /// <param name="mode">현재 실행 상태를 읽는다.</param>
    /// <param name="hooks">필요한 효과 묶음이다.</param>
    /// <param name="authority">현재 서버 전역이다. null이면 모드를 읽는다.</param>
    /// <param name="checking">같은 Client 진단 DWORD의 비소유 뷰다.</param>
    public RawBridgeDecay(RawSidPool pool, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<ushort> surface, IReadOnlyList<byte> spots, IReadOnlyList<GraphRecord> graphs, Func<BridgeDecayMode> mode, RawBridgeDecayHooks hooks, RawAuthorityState? authority = null, RawCheckingFlag? checking = null)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(surface);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(graphs);
        ArgumentNullException.ThrowIfNull(mode);
        ArgumentNullException.ThrowIfNull(hooks);
        if (pool.Edition != OriginalEdition.Patch1078 || types.Count != OriginalConstructors.TypeCount || frames.Count != types.Count || surface.Count != 65536 || spots.Count != surface.Count || graphs.Count != 255)
        {
            throw new ArgumentException("다리 붕괴의 PATCH 풀·현재 표면·Graph 표 오류");
        }

        _pool = pool;
        _types = types;
        _frames = frames;
        _surface = surface;
        _spots = spots;
        _graphs = graphs;
        _mode = mode;
        _hooks = hooks;
        _authority = authority;
        _checking = checking ?? new RawCheckingFlag();
    }

    /// <summary>작은 Graph→단단→이웃·열린 끝→방문 목록→현재 수명·삭제 순서다. 원문 예외는 바깥 스캔이 처리한다.</summary>
    /// <param name="root">처리할 원천 번호다.</param>
    public void Cell(int root)
    {
        int graph = _pool.Slot(root)[30];
        if (graph == InvalidGraph || graph >= _graphs.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(root), "다리 붕괴의 무효 Graph 원문 예외");
        }

        if (_graphs[graph].Surfaces < MinimumGraphSurfaces)
        {
            Need(_hooks.Destroy)(root, 0u);
            return;
        }

        RiftFrameCode code = Code(root);
        if ((code.Flags & HardFrameFlag) != 0)
        {
            return;
        }

        List<int> neighbors = Neighbors(root);
        if (neighbors.Count == 0)
        {
            ApplyLife(root, 1);
            return;
        }

        // 원문의 첫 10개 목록만 열린 방향 판정에 쓴다.
        List<int> listed = neighbors.Count <= 10 ? [.. neighbors] : neighbors.GetRange(0, 10);
        ReadOnlySpan<byte> raw = _pool.Slot(root);
        float x = BitConverter.UInt32BitsToSingle(Read(raw, 14));
        float y = BitConverter.UInt32BitsToSingle(Read(raw, 18));
        ushort[] snapshot = _surface as ushort[] ?? _surface.ToArray();
        if (BridgeSurfaceRules.OpenDirection((char)code.Side, x, y, snapshot.AsSpan(), listed) == -1)
        {
            return;
        }

        DecayWalk walk = Collect(root);
        if (!walk.Complete)
        {
            throw new InvalidOperationException("원문 다리 재귀 고리의 호스트 안전 중단");
        }

        if (!walk.CanDecay)
        {
            return;
        }

        int lowest = int.MaxValue;
        foreach (int number in walk.Visited)
        {
            int life = (int)((Read(_pool.Slot(number), 12, 2) & 0x78u) >> 3);
            if (life != 0 && life <= lowest)
            {
                lowest = life;
            }
        }

        // 방문 목록의 현재 비영 수명만 최소값에 반영한다.
        bool none = lowest == int.MaxValue;
        if (!none && _checking && lowest > 7)
        {
            Need(_hooks.Assertion)("0 <= lowestTimeLeft && lowestTimeLeft <= brMAX_TIME_LEFT", "\\Ns\\O\\Bridge.cpp", 0x268u);
        }

        if (!none && lowest > 7)
        {
            lowest = 7;
        }

        int target = none ? 7 : lowest - 1;
        foreach (int number in walk.Visited)
        {
            ReadOnlySpan<byte> current = _pool.Slot(number);
            if ((current[RawSidPool.StateOffset] & 2u) == 0)
            {
                ApplyLife(number, (int)((Read(current, 12, 2) & 0x78u) >> 3) - target);
            }
        }

        if (target < CrackLife && lowest >= CrackLife)
        {
            Need(_hooks.CrackSound)(root);
        }
    }

    /// <summary>정상 7 초과를 미리 거부하지 않고 원문의 부호 산술·4비트 쓰기를 보존한다.</summary>
    /// <param name="sid">적용할 번호다.</param>
    /// <param name="reduction">줄일 수명이다.</param>
    public void ApplyLife(int sid, int reduction)
    {
        uint previous = (Read(_pool.Slot(sid), 12, 2) & 0x78u) >> 3;
        int remaining = unchecked((int)previous) - reduction;
        if (remaining < 0)
        {
            remaining = 0;
        }

        if (remaining == 0 && (_authority is not null ? _authority.Server : _mode().Server))
        {
            byte side = Code(sid).Side;
            Need(_hooks.Destroy)(sid, side == (byte)'J' || side == (byte)'K' ? PlankRemoval : 0u);
            return;
        }

        Span<byte> raw = _pool.RawSlot(sid);
        Put(raw, 12, (Read(raw, 12, 2) & ~0x78u) | (((uint)remaining * 8u) & 0x78u), 2);
        Need(_hooks.NotifySurface)(sid);
        if (remaining < CrackLife && (int)previous >= CrackLife)
        {
            ReadOnlySpan<byte> current = _pool.Slot(sid);
            if (current[RawSidPool.TypeOffset] >= (uint)_frames.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(sid), "다리 수명 통지 뒤의 현재 TYPE 범위 오류");
            }

            int frame = RawBridgeFrames.CrackedFrame(_frames[current[RawSidPool.TypeOffset]], BitConverter.ToInt32(current.Slice(36, 4)), false);
            if (frame != -1)
            {
                Put(_pool.RawSlot(sid), 36, (uint)frame, 4);
                Need(_hooks.Refresh)(sid);
            }
        }
        else if (_mode().DebugRedraw)
        {
            Need(_hooks.Refresh)(sid);
        }

        if (remaining <= CarrierLife)
        {
            Need(_hooks.Carriers)(sid);
        }
    }

    /// <summary>잘못된 프레임은 해당 칸의 원문 예외 경계로 올리며 임의 프레임으로 대신하지 않는다.</summary>
    /// <param name="sid">읽을 번호다.</param>
    /// <returns>현재 프레임 코드다.</returns>
    private RiftFrameCode Code(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        if (raw[RawSidPool.TypeOffset] >= (uint)_frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "다리 붕괴의 현재 TYPE 범위 오류");
        }

        IReadOnlyList<RiftFrameCode> codes = _frames[raw[RawSidPool.TypeOffset]].Codes;
        uint number = Read(raw, 36);
        if (number >= (uint)codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "다리 붕괴의 현재 프레임 범위 오류");
        }

        return codes[(int)number];
    }

    /// <summary>생성된 원천 기하만 고정하고 후보·지도는 같은 이미 복원된 flag 8 순회에서 읽는다.</summary>
    /// <param name="sid">원천 번호다.</param>
    /// <returns>표면 순서의 이웃 목록이다.</returns>
    private List<int> Neighbors(int sid)
    {
        var result = new List<int>();
        var walk = new RawCanonSurfaceWalk(_pool, _types, _frames, _surface, _spots, sid);
        for (int neighbor = walk.Current(); neighbor != 0; neighbor = walk.Next())
        {
            result.Add(neighbor);
        }

        return result;
    }

    /// <summary>원문 100칸 목록과 경계 삭제·접합 재귀를 보존한다.</summary>
    /// <param name="root">원천 번호다.</param>
    /// <returns>방문 목록과 두 전역 플래그다.</returns>
    private DecayWalk Collect(int root)
    {
        var result = new DecayWalk();
        result.Visited.Add(root);
        var active = new List<int>();
        void Visit(int sid, int depth, int parent)
        {
            if (depth >= 256 || active.Contains(sid))
            {
                result.Complete = false;
                result.CanDecay = false;
                return;
            }

            active.Add(sid);
            foreach (int neighbor in Neighbors(sid))
            {
                ReadOnlySpan<byte> raw = _pool.Slot(neighbor);
                uint flags = _types[raw[RawSidPool.TypeOffset]].Flags2;
                bool bridge = (flags & TypeFlag2.Bridge) != 0;
                bool island = (flags & TypeFlag2.Island) != 0;
                // 다리·섬만 처리하며 부모 외의 중복은 원문처럼 목록에 남긴다.
                if ((!bridge && !island) || neighbor == parent)
                {
                    continue;
                }

                if (bridge && result.Visited.Count < 100)
                {
                    result.Visited.Add(neighbor);
                }

                if (Code(neighbor).Side < (byte)'J' && !island)
                {
                    if (depth == 0 && Neighbors(neighbor).Count < 3)
                    {
                        result.ShortJunction = true;
                    }

                    Visit(neighbor, depth + 1, sid);
                    if (!result.Complete)
                    {
                        break;
                    }
                }
                else if (depth == 0 || (depth == 1 && result.ShortJunction))
                {
                    result.CanDecay = true;
                    result.Visited.RemoveAll(n => n == neighbor);
                }
                else
                {
                    if (!bridge || Neighbors(neighbor).Count < 2)
                    {
                        result.CanDecay = true;
                    }
                    else
                    {
                        result.Visited.RemoveAll(n => n == neighbor);
                    }
                }
            }

            active.Remove(sid);
        }

        Visit(root, 0, 0);
        return result;
    }

    /// <summary>선택된 현재 효과가 빠졌으면 앞선 WORD·프레임 쓰기 상태에서 오류를 전달한다.</summary>
    /// <typeparam name="T">효과 대리자 형식이다.</typeparam>
    /// <param name="callback">검사할 효과다.</param>
    /// <returns>연결된 효과다.</returns>
    private static T Need<T>(T? callback) where T : class
    {
        if (callback is null)
        {
            throw new InvalidOperationException("다리 붕괴의 실제 효과 미연결");
        }

        return callback;
    }

    /// <summary>비정렬 raw 필드를 리틀 엔디언 DWORD로 읽는다.</summary>
    /// <param name="raw">읽을 슬롯 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 폭이다.</param>
    /// <returns>조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        // 지정한 필드 폭의 낮은 바이트부터 조립한다.
        uint value = 0u;
        for (int i = 0; i < width; i++)
        {
            value |= (uint)raw[offset + i] << (8 * i);
        }

        return value;
    }

    /// <summary>직접 프레임 쓰기는 별도 공간 재등록을 하지 않는다.</summary>
    /// <param name="slot">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 폭이다.</param>
    private static void Put(Span<byte> slot, int offset, uint value, int width = 4)
    {
        // 원문 지정 폭의 낮은 바이트부터 쓴다.
        for (int i = 0; i < width; i++)
        {
            slot[offset + i] = (byte)(value >> (8 * i));
        }
    }
}
