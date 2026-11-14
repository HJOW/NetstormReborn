namespace Netstorm.Core.Original;

/// <summary>
/// 배치 파이프라인 조회에 쓰는 원본 쿼리다
/// (cpppj <c>o/RawCanonPlacement.h</c>의 <c>CanonPlacementQuery</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 전체 파이프라인(<c>RawCanonPlacementPipeline</c>)은 R6 범위라 여기서는 조회 형식만 둔다.</remarks>
public readonly record struct CanonPlacementQuery(uint Type, uint Argument, float X, float Y, uint Flags, uint Owner, uint Mode)
{
    /// <summary>배치할 타입 번호다. 별도 Argument와 구별한다.</summary>
    public uint Type { get; init; } = Type;

    /// <summary>타입과 구별하는 패턴 번호 또는 첫 모양 인자다.</summary>
    public uint Argument { get; init; } = Argument;

    /// <summary>실제 배치가 전달하는 월드 X 좌표다.</summary>
    public float X { get; init; } = X;

    /// <summary>실제 배치가 전달하는 월드 Y 좌표다.</summary>
    public float Y { get; init; } = Y;

    /// <summary>방향·번호 바이트 아래쪽·충돌 대역을 합친 값이다.</summary>
    public uint Flags { get; init; } = Flags;

    /// <summary>소유자다.</summary>
    public uint Owner { get; init; } = Owner;

    /// <summary>모드다.</summary>
    public uint Mode { get; init; } = Mode;
}

/// <summary>
/// 매 실제 분기에서 읽는 원본 타입·편집 소유자 전역이다
/// (cpppj <c>o/RawFortRandomPlacement.h</c>의 <c>FortRandomPlacementState</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 호출자가 소유하며 이 모듈보다 오래 살아야 한다.</remarks>
public sealed class FortRandomPlacementState
{
    /// <summary>두 판본의 섬 타입 초기값이다.</summary>
    public uint IslandType { get; set; } = 97u;

    /// <summary>두 판본의 가장자리 농지 타입 초기값이다.</summary>
    public uint EdgeFarmType { get; set; } = 121u;

    /// <summary>00544ce0·CD 005187b8의 값이다. 0이면 지역 owner·force0 규칙을 유지한다.</summary>
    public uint OwnerOverride { get; set; }
}

/// <summary>
/// 임의 성공·기본값을 제공하지 않는 실제 월드 효과 경계다
/// (cpppj <c>o/RawFortRandomPlacement.h</c>의 <c>FortRandomPlacementHooks</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortRandomPlacementHooks
{
    /// <summary>완전한 0049b510·CD 00445200 공간·발자국·그래프 검사다.</summary>
    public Func<CanonPlacementQuery, bool>? MayPlace { get; set; }

    /// <summary>현재 덩어리 격자의 부호 있는 바이트 지역이다.</summary>
    public Func<int, int, sbyte>? ChunkRegion { get; set; }

    /// <summary>현재 Islandlist owner 0046f320·CD 004bda70이다.</summary>
    public Func<int, uint>? RegionOwner { get; set; }

    /// <summary>실제 생성자(type, 0)와 월드 등록이다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>타입 변경 뒤 현재 타입의 생성자만 실행한다. 생성 뒤 만들기는 부르지 않는다.</summary>
    public Action<int, uint>? Construct { get; set; }

    /// <summary>현재 받침·추상·HP 플래그 검사다.</summary>
    public Func<int, bool>? HasHitPoints { get; set; }

    /// <summary>가상 최대 HP가 아닌 현재 타입 HP getter다.</summary>
    public Func<uint, int>? TypeHitPoints { get; set; }

    /// <summary>실제 가상 +74다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>실제 가상 +48이다.</summary>
    public Action<int, uint>? Unpop { get; set; }

    /// <summary>실제 가상 +4c이며 공간·등록 뒤까지 이어진다.</summary>
    public Action<int, uint>? Repop { get; set; }

    /// <summary>새 객체의 실제 가상 +90이다. 플래그는 0이다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }
}

/// <summary>
/// Fort 후처리의 단일 랜덤 배치를 현재 raw 공간·지역·생성·등록에 연결한다
/// (cpppj <c>o/RawFortRandomPlacement</c>).
/// </summary>
/// <remarks>2026-11-04 F2c-11에서 추가했다. 전체 배치 파이프라인을 잇는 <c>MakeFortRandomPlacementHooks</c>는 R6 범위라 제외했다.</remarks>
public sealed class RawFortRandomPlacement
{
    /// <summary>원본 raw 타입·상태·HP와 판본별 프레임 위치다.</summary>
    private const int TypeOffset = 10;

    /// <summary>원본 raw 상태 바이트 위치다.</summary>
    private const int StateOffset = 11;

    /// <summary>원본 raw HP 필드 위치다.</summary>
    private const int HpOffset = 26;

    /// <summary>패치판 원본 raw 프레임 DWORD 위치다.</summary>
    private const int PatchFrameOffset = 36;

    /// <summary>CD판 원본 raw 프레임 바이트 위치다.</summary>
    private const int CdFrameOffset = 34;

    /// <summary>지형 프레임 플래그1 비트다.</summary>
    private const uint TerrainFrame = 0x200000u;

    /// <summary>랜덤 프레임 플래그1 비트다.</summary>
    private const uint RandomFrameFlag = 0x100000u;

    /// <summary>단일 배치가 먼저 읽는 spot 하위 비트 마스크다.</summary>
    private const byte SpotMask = 0x3f;

    /// <summary>원본 void 상태 비트다.</summary>
    private const byte VoidState = 4;

    /// <summary>고정 P·P 와일드카드 검색의 최대 후보다.</summary>
    private const int Candidates = 256;

    /// <summary>실제 방향 문자 P다.</summary>
    private const byte DirectionP = (byte)'P';

    /// <summary>실제 방향 문자 A다.</summary>
    private const byte DirectionA = (byte)'A';

    /// <summary>
    /// 자료·모든 필수 효과·실제 hash0을 검사한 뒤 빌린 현재 저장소를 유지한다.
    /// </summary>
    /// <param name="pool">객체 슬롯의 생성·조회에 쓰는 풀이다.</param>
    /// <param name="hash">좌표별 객체 후보를 찾는 공간 해시다.</param>
    /// <param name="spots">사본 없이 읽는 현재 공간 배열이다.</param>
    /// <param name="types">타입 번호로 속성·플래그를 조회하는 타입 표다.</param>
    /// <param name="frames">실제 로딩 자료다.</param>
    /// <param name="globalRandom">원본 FUN_004558c0 난수다. 프레임 선택이 소비한다.</param>
    /// <param name="state">매 실제 분기에서 읽는 원본 타입·편집 소유자 전역이다.</param>
    /// <param name="hooks">현재 전역·실제 가상 경계다.</param>
    /// <exception cref="ArgumentException">Fort 단일 배치의 판본·자료·실제 효과 오류다.</exception>
    public RawFortRandomPlacement(
        RawSidPool pool,
        RawSquidHash hash,
        byte[] spots,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeFrames> frames,
        GameRandom globalRandom,
        FortRandomPlacementState state,
        FortRandomPlacementHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(globalRandom);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected || frames.Count != types.Count
            || spots.Length != 65536 || hash.Entries(0).Length != 65536
            || hooks.MayPlace is null || hooks.ChunkRegion is null || hooks.RegionOwner is null
            || hooks.Create is null || hooks.Construct is null || hooks.HasHitPoints is null || hooks.TypeHitPoints is null
            || hooks.SetOwner is null || hooks.Unpop is null || hooks.Repop is null || hooks.Pop is null)
        {
            throw new ArgumentException("Fort 단일 배치 판본·자료·실제 효과 오류");
        }
        _pool = pool;
        _hash = hash;
        _spots = spots;
        _types = types;
        _frames = frames;
        _random = globalRandom;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>객체 슬롯의 생성·조회·반납에 사용하는 SID 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표별 객체 후보를 찾는 공간 해시. 실제 hash0 배열이다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>현재 공간 배열을 사본 없이 읽는다.</summary>
    private readonly byte[] _spots;

    /// <summary>타입 번호로 속성·플래그를 조회하는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>실제 로딩 자료다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>원본 FUN_004558c0 난수다. 상태 0이면 0x0BAD0BAD로 바꾼 뒤 상태×0x10003+3의 상위 16비트를 한도로 나눈 나머지를 쓴다.</summary>
    private readonly GameRandom _random;

    /// <summary>매 실제 분기에서 읽는 원본 타입·편집 소유자 전역이다.</summary>
    private readonly FortRandomPlacementState _state;

    /// <summary>현재 전역·실제 가상 경계다.</summary>
    private readonly FortRandomPlacementHooks _hooks;

    /// <summary>x87 CRT 절삭은 하위 DWORD를 사용한다. 원본이 주소를 벗어나는 비정상 입력은 거부한다.</summary>
    /// <param name="value">자를 double 값이다.</param>
    /// <returns>원본 절삭의 하위 32비트다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Fort 배치 CRT 좌표 범위 오류다.</exception>
    private static int Ftol(double value)
    {
        if (!double.IsFinite(value) || Math.Abs(value) >= 9.2e18)
        {
            throw new ArgumentOutOfRangeException(nameof(value), value, "Fort 배치 CRT 좌표 범위 오류");
        }
        long cut = (long)value;
        return unchecked((int)unchecked((uint)cut));
    }

    /// <summary>판본 좁은 필드를 읽거나 쓰며 옆 내용은 보존한다.</summary>
    /// <param name="raw">읽을 슬롯이다.</param>
    /// <param name="offset">읽을 위치다.</param>
    /// <param name="width">읽을 바이트 수다.</param>
    /// <returns>리틀 엔디언으로 조합한 값이다.</returns>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width)
    {
        uint result = 0;
        // 지정 폭만 리틀 엔디언으로 조합한다
        for (int index = 0; index < width; index++)
        {
            result |= (uint)raw[offset + index] << (8 * index);
        }
        return result;
    }

    /// <summary>지정한 DWORD·WORD·바이트만 원본 raw 슬롯에 기록한다.</summary>
    /// <param name="raw">쓸 슬롯이다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓰는 바이트 수다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 실제 쓰기 폭만 변경한다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>요청·현재 raw 타입을 실제 자료 범위 안에서 읽는다.</summary>
    /// <param name="number">타입 번호다.</param>
    /// <returns>타입 기록이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Fort 단일 배치 타입 범위 오류다.</exception>
    private OriginalTypeRecord Type(uint number)
    {
        if (number >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(number), number, "Fort 단일 배치 타입 범위 오류");
        }
        return _types[(int)number];
    }

    /// <summary>현재 일반 해시·탐색기의 정확 타입·genus 검색이다. 초기 spot의 반올림 좌표와 구별한다.</summary>
    /// <param name="x">검색 X 좌표다.</param>
    /// <param name="y">검색 Y 좌표다.</param>
    /// <param name="value">찾을 타입 번호 또는 genus 비트다.</param>
    /// <param name="genus">참이면 genus로 찾는다.</param>
    /// <returns>처음 통과한 SID다. 없으면 0이다.</returns>
    private int Find(float x, float y, uint value, bool genus)
    {
        int ix = Ftol(x);
        int iy = Ftol(y);
        RawSquidFinder finder = new(_pool, _hash, _types);
        // 정확 타입의 표면·다리 부류가 아니면 hash0을 건너뛴다
        uint flags = !genus && (Type(value).Flags2 & 6u) == 0 ? 1u : 0u;
        // 원본 탐색기의 void·발자국·후행 next 규칙과 처음 통과 순서를 유지한다
        for (int sid = finder.Begin(new SquidSearchArea(ix, iy, ix, iy), flags); sid != 0; sid = finder.Next())
        {
            uint type = _pool.Slot(sid)[TypeOffset];
            if (genus ? (Type(type).Flags2 & value) != 0 : type == value)
            {
                return sid;
            }
        }
        return 0;
    }

    /// <summary>단어 표면 번호·타입만 검사한다. 게임 상태 선택을 새로 넣지 않는다.</summary>
    /// <param name="x">셀 X 좌표다.</param>
    /// <param name="y">셀 Y 좌표다.</param>
    /// <returns>섬 타입이면 참이다.</returns>
    private bool Island(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256)
        {
            return false;
        }
        int sid = _hash.Entries(0)[y * 256 + x];
        return sid != 0 && _pool.Slot(sid)[TypeOffset] == _state.IslandType;
    }

    /// <summary>북서→북동→남동→남서의 원본 단락 평가를 유지한다.</summary>
    /// <param name="x">셀 X 좌표다.</param>
    /// <param name="y">셀 Y 좌표다.</param>
    /// <returns>교체 가능하면 참이다.</returns>
    /// <exception cref="ArgumentOutOfRangeException">Fort 교체 좌표 범위 오류다.</exception>
    public bool MayReplace(int x, int y)
    {
        if (x < 0 || y < 0 || x >= 256 || y >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(x), x, "Fort 교체 좌표 범위 오류");
        }
        if (!Island(x - 1, y - 1) && Island(x, y - 1) && Island(x - 1, y))
        {
            return false;
        }
        if (!Island(x + 1, y - 1) && Island(x, y - 1) && Island(x + 1, y))
        {
            return false;
        }
        if (!Island(x + 1, y + 1) && Island(x, y + 1) && Island(x + 1, y))
        {
            return false;
        }
        if (!Island(x - 1, y + 1) && Island(x, y + 1) && Island(x - 1, y))
        {
            return false;
        }
        return true;
    }

    /// <summary>원본 타입 변경은 void와 두 발자국 동일성만 검사하고 현재 생성자만 재실행한다.</summary>
    /// <param name="sid">바꿀 슬롯 번호다.</param>
    /// <param name="number">새 타입 번호다.</param>
    /// <exception cref="InvalidOperationException">Fort 타입 변경의 원본 void·동일 발자국 assert다.</exception>
    public void Retype(int sid, uint number)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        OriginalTypeRecord before = Type(raw[TypeOffset]);
        OriginalTypeRecord after = Type(number);
        if ((raw[StateOffset] & VoidState) == 0 || before.FootX != after.FootX || before.FootY != after.FootY)
        {
            throw new InvalidOperationException("Fort 타입 변경 원본 void·동일 발자국 assert");
        }
        raw[TypeOffset] = (byte)number;
        Need(_hooks.Construct)(sid, raw[TypeOffset]);
    }

    /// <summary>최초 프레임은 가상 SetFrame 없이 패치 DWORD·CD 바이트로 직접 쓴다.</summary>
    /// <param name="sid">쓸 슬롯 번호다.</param>
    /// <param name="value">쓸 프레임 값이다.</param>
    private void Frame(int sid, uint value)
    {
        bool patch = _pool.Edition == OriginalEdition.Patch1078;
        Put(_pool.AllocatedBytes(sid), patch ? PatchFrameOffset : CdFrameOffset, value, patch ? 4 : 1);
    }

    /// <summary>HasHitPoints 호출 뒤 현재 타입 HP를 읽고 공통 HP 필드에 직접 쓴다.</summary>
    /// <param name="sid">쓸 슬롯 번호다.</param>
    private void HitPoints(int sid)
    {
        if (Need(_hooks.HasHitPoints)(sid))
        {
            int hp = Need(_hooks.TypeHitPoints)(_pool.Slot(sid)[TypeOffset]);
            Put(_pool.AllocatedBytes(sid), HpOffset, unchecked((uint)hp),
                _pool.Edition == OriginalEdition.Patch1078 ? 4 : 2);
        }
    }

    /// <summary>고정 P·P·와일드카드 요청의 실제 전체 난수·최대 256 후보다.</summary>
    /// <param name="type">프레임을 고를 타입 번호다.</param>
    /// <returns>고른 코드 번호다. 비어 있으면 -1이다.</returns>
    private int RandomFrame(uint type)
    {
        Type(type);
        IReadOnlyList<RiftFrameCode> codes = _frames[(int)type].Codes;
        int[] candidates = new int[Candidates];
        int count = 0;
        // P·P→변환 P·P(P 방향 마스크 0)→P·A→P·임의 변형의 첫 비어 있지 않은 집합이다
        for (int pass = 0; pass < 4 && count == 0; pass++)
        {
            // 앞 256개의 일치 번호만 저장하며 나머지 코드도 원본처럼 계속 읽는다
            for (int index = 0; index < codes.Count; index++)
            {
                if (codes[index].Side == DirectionP && (pass == 3 || codes[index].Variant == (pass == 2 ? DirectionA : DirectionP)) && count < Candidates)
                {
                    candidates[count++] = index;
                }
            }
        }
        return count != 0 ? candidates[unchecked((int)_random.Next((uint)count))] : -1;
    }

    /// <summary>
    /// 실제 단일 몸체의 조기 반환·지면 교체·새 객체 효과를 순서대로 실행한다.
    /// 0040db90·CD 0042c3d0이며 localRandom 인자는 원본 단일 몸체에서 사용하지 않는다.
    /// 모서리 거부 반환 1은 실제 새 SID가 아니므로 호출자가 새 객체로 해석해서는 안 된다.
    /// </summary>
    /// <param name="localRandom">군집이 전달하는 local 난수다. 단일 몸체는 소비하지 않는다.</param>
    /// <param name="request">배치 요청이다.</param>
    /// <returns>새 SID·교체 지면 SID·모서리 거부 1·실패 0이다.</returns>
    public uint Place(GameRandom localRandom, FortRandomPlaceRequest request)
    {
        int sx = Ftol((double)request.X + 0.5);
        int sy = Ftol((double)request.Y + 0.5);
        if (sx < 0 || sy < 0 || sx >= 256 || sy >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(request), request, "Fort 반올림 spot 주소 범위 오류");
        }
        if ((_spots[sy * 256 + sx] & SpotMask) == 0)
        {
            return 0;
        }
        if (Find(request.X, request.Y, _state.EdgeFarmType, false) != 0)
        {
            return 0;
        }
        CanonPlacementQuery query = new(request.Type, 0, request.X, request.Y, 0, 0, 0);
        if (!Need(_hooks.MayPlace)(query))
        {
            return 0;
        }
        if (request.Force == 0 && (int)Need(_hooks.ChunkRegion)(Ftol(request.X) / 16, Ftol(request.Y) / 16) != request.Region)
        {
            return 0;
        }
        OriginalTypeRecord requested = Type(request.Type);
        uint frame = 0;
        if ((requested.Flags1 & TerrainFrame) != 0)
        {
            int ground = Find(request.X, request.Y, TypeFlag2.Island, true);
            if (ground == 0)
            {
                throw new InvalidOperationException("Fort 지형 프레임 원본 지면 assert");
            }
            if (request.Mode != 0)
            {
                if (!MayReplace(Ftol(request.X), Ftol(request.Y)))
                {
                    return 1;
                }
                Need(_hooks.Unpop)(ground, 0);
                Retype(ground, _state.EdgeFarmType);
                HitPoints(ground);
                Need(_hooks.Repop)(ground, 64);
                return unchecked((uint)ground);
            }
            bool patch = _pool.Edition == OriginalEdition.Patch1078;
            frame = Read(_pool.Slot(ground), patch ? PatchFrameOffset : CdFrameOffset, patch ? 4 : 1);
        }
        if ((requested.Flags1 & RandomFrameFlag) != 0)
        {
            frame = unchecked((uint)RandomFrame(request.Type));
        }
        if (frame == 0xffffffffu)
        {
            throw new InvalidOperationException("Fort 단일 배치 프레임 -1 원본 assert");
        }
        int sid = Need(_hooks.Create)(request.Type, 0);
        Frame(sid, frame);
        HitPoints(sid);
        uint owner = request.Force != 0 ? 0u : Need(_hooks.RegionOwner)(request.Region);
        if (_state.OwnerOverride != 0)
        {
            owner = _state.OwnerOverride;
        }
        Need(_hooks.SetOwner)(sid, owner);
        Need(_hooks.Pop)(sid, request.X, request.Y, 0);
        return unchecked((uint)sid);
    }

    /// <summary>실제 월드 묶음 연결의 pool 동일성 검사에 사용한다.</summary>
    /// <returns>이 모듈이 쓰는 풀이다.</returns>
    public RawSidPool Pool()
    {
        return _pool;
    }

    /// <summary>FinishOwner의 필수 단일 배치를 실제 몸체에 연결한다.</summary>
    /// <param name="pool">검사할 풀이다.</param>
    /// <param name="placement">같은 월드의 단일 배치 몸체다.</param>
    /// <param name="hooks">물을 훅 묶음이다.</param>
    /// <returns>배치가 연결된 훅 묶음이다.</returns>
    /// <exception cref="ArgumentException">Fort 종료·단일 배치 SID 풀이 다르다.</exception>
    public static FortFinishOwnerHooks MakeFortSinglePlacementHook(RawSidPool pool, RawFortRandomPlacement placement, FortFinishOwnerHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(placement);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, placement.Pool()))
        {
            throw new ArgumentException("Fort 종료·단일 배치 SID 풀이 다릅니다");
        }
        hooks.Place = (random, request) => placement.Place(random, request);
        return hooks;
    }

    /// <summary>도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.</summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <returns>연결된 장치다.</returns>
    /// <exception cref="InvalidOperationException">단일 배치의 실제 장치가 연결되지 않았다.</exception>
    private static T Need<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort 단일 배치 실제 장치 누락");
        }
        return function;
    }
}
