// 초기 TerrainBuilder의 두 이웃 mask·독립 CRT cache·실제 지면/fringe 생성을 같은 raw 풀에 연결한다(cpppj o/RawTerrainStamp.h/.cpp).
// 2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 이식했다.
namespace Netstorm.Core.Original;

/// <summary>
/// 실제 TerrainBuilder code/지역 지도와 변경 가능한 문자 타입 mapping 및 context다
/// (cpppj <c>o/RawTerrainStamp.h</c>의 <c>TerrainStampMap</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainStampMap
{
    /// <summary>원본 X열 우선 65,536 BYTE 지면 문자다. 렌더의 Y행 자료는 명시적으로 전치해야 한다.</summary>
    public byte[] Codes { get; set; } = [];

    /// <summary>codes와 같은 X열 우선 지역 BYTE 지도다.</summary>
    public byte[] Regions { get; set; } = [];

    /// <summary>문자별 현재 타입 DWORD다. Stamp는 x/X/= 항목을 현재 전역 타입으로 다시 쓴다.</summary>
    public int[] TypeMapping { get; set; } = [];

    /// <summary>문자별 대응문자 DWORD다. FrameAt는 기준 문자의 하위 BYTE를 부호 확장한다.</summary>
    public int[] Aliases { get; set; } = [];

    /// <summary>left/top/right/bottom이며 stamp 루프는 반열림, 이웃 검사는 양 끝을 포함한다.</summary>
    public int[] Bounds { get; set; } = new int[4];

    /// <summary>factory의 두 번째 DWORD 인자로 매 생성마다 현재 값을 읽는다.</summary>
    public uint MakePredictable { get; set; }

    /// <summary>지면 factory 뒤, owner 호출 전에 저장하는 원본 context의 마지막 SID다.</summary>
    public int LastSid { get; set; }

    /// <summary>fringe의 signed codeflags와 겹치면 실제 검색/게임 난수를 반복 소비한다.</summary>
    public uint ExcludedFlags { get; set; }
}

/// <summary>
/// 초기 stamp만 사용하는 전역 99 cache다. 같은 월드의 RegionPaint cache와 섞지 않는다
/// (cpppj <c>o/RawTerrainStamp.h</c>의 <c>TerrainStampState</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainStampState
{
    /// <summary>첫 FrameAt에서 현재 CRT state를 99회 연속 소비해 만들고 월드 교체 뒤에도 보존한다.</summary>
    public int[]? Variants { get; set; }

    /// <summary>원본 기본 본섬 타입이다. 실제 로드 전역값을 주입할 수 있다.</summary>
    public uint IslandType { get; set; } = 97u;

    /// <summary>문자 X에 매핑하는 원본 기본 경계 타입이다.</summary>
    public uint EdgeType { get; set; } = 98u;

    /// <summary>현재 frame flags 1에서 추가하는 실제 장식 타입이다.</summary>
    public uint FringeType { get; set; } = 102u;

    /// <summary>현재 ChunkMap 조회가 필요한 원본 지역 없음 값이다.</summary>
    public int NoRegion { get; set; } = 127;

    /// <summary>검색 실패 -1 뒤 원본이 읽는 코드 배열 바로 앞 BYTE의 명시 입력이다.</summary>
    public byte FringePrefixFlags { get; set; }
}

/// <summary>
/// 원본의 두 assertion 지점을 가리킨다. 판본에 따라 실제 호출 유무가 다르다
/// (cpppj <c>o/RawTerrainStamp.h</c>의 <c>TerrainStampAssertion</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public enum TerrainStampAssertion
{
    /// <summary>패치의 섬 genus 지면이 지역 127이면 현재 ChunkMap 조회 전에 보고한다.</summary>
    MissingRegion,

    /// <summary>fringe 생성 직전 makePredictable이 0이면 두 판본의 assertion을 보고한다.</summary>
    UnpredictableFringe,
}

/// <summary>
/// 객체 생성·가상 owner/Pop·현재 지역 setter를 월드의 실제 복원 몸체에 연결하는 콜백 묶음이다
/// (cpppj <c>o/RawTerrainStamp.h</c>의 <c>TerrainStampHooks</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class TerrainStampHooks
{
    /// <summary>같은 pool에 type/predictable을 전달해 생성한 실제 SID를 반환한다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>지면의 가상 +0x74(0)이며 raw 소유자와 후속 장부를 갱신한다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>가상 +0x90이며 같은 SID의 실제 Pop/PostPop과 flags 0x02000061을 실행한다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>실제 004acd20 / CD 004aca80이며 현재 타입 조건과 raw 지역 WORD 쓰기를 보존한다.</summary>
    public Action<int, int>? SetRegion { get; set; }

    /// <summary>지역 127에서 x/16, y/16을 원본 절삭으로 얻어 현재 ChunkMap 지역 BYTE를 조회한다.</summary>
    public Func<int, int, sbyte>? ChunkRegion { get; set; }

    /// <summary>원본 assertion 뒤 계속 진행하는 경로를 호출자에게 보고한다.</summary>
    public Action<TerrainStampAssertion>? Assertion { get; set; }
}

/// <summary>
/// 패치 004c04b0/004c0640/004bfe50와 CD 0049cf20/0049d2c0의 초기 프레임/stamp 몸체다
/// (cpppj <c>o/RawTerrainStamp</c>).
/// </summary>
/// <remarks>2026-11-07 R4 지형·섬 생성 및 인구/배치 1차에서 추가했다.</remarks>
public sealed class RawTerrainStamp
{
    /// <summary>원본 factory 결과의 Pop flags다.</summary>
    private const uint PopFlags = 0x02000061u;

    /// <summary>실제 _rand의 holdrand 곱셈 값이다.</summary>
    private const uint CrtMultiplier = 0x343fdu;

    /// <summary>실제 _rand의 holdrand 증가 값이다.</summary>
    private const uint CrtIncrement = 0x269ec3u;

    /// <summary>15비트 반환 mask다.</summary>
    private const uint CrtMask = 0x7fffu;

    /// <summary>실제 양판본의 북부터 시계 방향 bit 표다.</summary>
    private static readonly int[] NeighborBits = [1, 2, 2, 4, 4, 8, 8, 1];

    /// <summary>생성/owner/프레임/Pop이 공유하는 실제 raw 슬롯 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>현재 타입의 genus를 실제 생성 뒤 조회하는 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>실제 J 시작·codeflags/공유 후보 검색에 사용하는 프레임 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>현재 code/지역/mapping/bounds/context다.</summary>
    private readonly TerrainStampMap _map;

    /// <summary>비AA와 fringe 검색이 소비하는 실제 GameRandom이다. 2026-11-12 0.457에서 단일 공유 스트림으로 바꿨다(근거 cpppj RawCampaignSession.cpp 498행).</summary>
    private readonly GameRandom _random;

    /// <summary>초기 99 cache가 소비하는 공유 CRT holdrand 읽기 대리자다.</summary>
    private readonly Func<uint> _getCrtState;

    /// <summary>초기 99 cache가 소비하는 공유 CRT holdrand 쓰기 대리자다.</summary>
    private readonly Action<uint> _setCrtState;

    /// <summary>전역 수명의 초기 cache와 현재 특수 타입/원본 입력이다.</summary>
    private readonly TerrainStampState _state;

    /// <summary>필수 생성/owner/Pop/지역/진단 효과다.</summary>
    private readonly TerrainStampHooks _hooks;

    /// <summary>
    /// 타입/프레임/지도와 두 난수 및 필수 하위 효과를 연결해 생성한다.
    /// </summary>
    /// <param name="pool">SID 풀이다.</param>
    /// <param name="types">타입 레코드 표다.</param>
    /// <param name="frames">프레임 코드 표다.</param>
    /// <param name="map">스탬프 지도다.</param>
    /// <param name="random">게임 난수다.</param>
    /// <param name="getCrtState">CRT holdrand 읽기 대리자다.</param>
    /// <param name="setCrtState">CRT holdrand 쓰기 대리자다.</param>
    /// <param name="state">스탬프 상태다.</param>
    /// <param name="hooks">필수 효과 훅이다.</param>
    public RawTerrainStamp(
        RawSidPool pool,
        IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeFrames> frames,
        TerrainStampMap map,
        GameRandom random,
        Func<uint> getCrtState,
        Action<uint> setCrtState,
        TerrainStampState state,
        TerrainStampHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(map);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(getCrtState);
        ArgumentNullException.ThrowIfNull(setCrtState);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);

        int count = pool.Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (types.Count != count
            || frames.Count != count
            || map.Codes.Length != 65536
            || map.Regions.Length != 65536
            || map.TypeMapping.Length != 256
            || map.Aliases.Length != 256
            || hooks.Create is null
            || hooks.SetOwner is null
            || hooks.Pop is null
            || hooks.SetRegion is null
            || hooks.ChunkRegion is null
            || hooks.Assertion is null)
        {
            throw new ArgumentException("초기stamp 지도/타입/필수 효과 누락");
        }

        _pool = pool;
        _types = types;
        _frames = frames;
        _map = map;
        _random = random;
        _getCrtState = getCrtState;
        _setCrtState = setCrtState;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>같은 raw pool을 사용하는 월드/팝 결합인지 검사할 때 쓴다.</summary>
    public RawSidPool Pool => _pool;

    /// <summary>
    /// 현재 좌표의 직선/대각선 mask·타입·첫/재사용 cache를 읽어 실제 물리 프레임을 고른다.
    /// </summary>
    /// <param name="x">X 좌표다.</param>
    /// <param name="y">Y 좌표다.</param>
    /// <returns>선택된 물리 프레임 인덱스다.</returns>
    public int FrameAt(int x, int y)
    {
        int index = Index(x, y);
        byte code = _map.Codes[index];
        int character = Character(code);
        int alias = (sbyte)(byte)_map.Aliases[character];
        byte region = _map.Regions[index];
        int straight = 0;
        int diagonal = 0;

        // 짝수는 직선/홀수는 대각선이며 실제 이웃별 bit 표로 두 mask를 각각 누적한다.
        for (int neighbor = 0; neighbor < TerrainBuilder.NeighborCells.Length; neighbor++)
        {
            (int dx, int dy) = TerrainBuilder.NeighborCells[neighbor];
            if (Same(x + dx, y + dy, alias, region))
            {
                if ((neighbor % 2) != 0)
                {
                    diagonal |= NeighborBits[neighbor];
                }
                else
                {
                    straight |= NeighborBits[neighbor];
                }
            }
        }

        int type = _map.TypeMapping[character];
        if (type < 0 || type >= _frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "초기stamp frame 타입 범위 오류");
        }
        char side = TerrainBuilder.Orientation(straight);
        char variant = TerrainBuilder.Orientation(diagonal);
        Initialize();
        int frame;
        if (code == (byte)'x' && side == 'A' && variant == 'A')
        {
            int[] values = _state.Variants!;
            int sum = (values[(x / 3) % 99] + values[(y / 3) % 99]) % 99;
            if (sum < 0)
            {
                throw new ArgumentOutOfRangeException(nameof(x), "초기stamp cache 합계 범위 오류");
            }
            int chosen = (values[sum] + y / 3) % 4;
            frame = _frames[type].Run((byte)'J').First + 1 + (y % 3 + chosen * 3) * 3 + x % 3;
        }
        else
        {
            frame = RawFrameChoose.ChooseTerrainFrame(
                _frames[type],
                new RiftFrameCode((byte)side, (byte)variant, 0, 0),
                _pool.Edition == OriginalEdition.Patch1078 ? 0 : -1,
                _random);
        }
        return frame == -1 ? 0 : frame;
    }

    /// <summary>
    /// 현재 bounds의 Y 외부 / X 내부로 alias가 요청 문자와 같은 nonzero mapping만 실제 생성한다.
    /// </summary>
    /// <param name="code">스탬프할 문자 코드다.</param>
    public void Stamp(sbyte code)
    {
        // Y 외부 / X 내부 반열린 순서는 난수와 factory/Pop 호출의 순서를 결정한다.
        for (int y = _map.Bounds[1]; y < _map.Bounds[3]; y++)
        {
            // 각 행의 left/right도 현재 context에서 다시 읽는다.
            for (int x = _map.Bounds[0]; x < _map.Bounds[2]; x++)
            {
                int index = Index(x, y);
                int character = Character(_map.Codes[index]);
                if (x < _map.Bounds[0] || y < _map.Bounds[1] || x > _map.Bounds[2] || y > _map.Bounds[3]
                    || _map.Aliases[character] != code)
                {
                    continue;
                }
                _map.TypeMapping['x'] = (int)_state.IslandType;
                _map.TypeMapping['X'] = (int)_state.EdgeType;
                _map.TypeMapping['='] = (int)_state.IslandType;
                int type = _map.TypeMapping[character];
                if (type == 0)
                {
                    continue;
                }
                int region = (sbyte)_map.Regions[index];
                int frame = FrameAt(x, y);
                Create(x, y, (uint)type, frame, region);
            }
        }
    }

    /// <summary>프레임 flags BYTE를 MSVC signed char → DWORD로 부호 확장한다.</summary>
    private static uint SignedFlags(byte flags) => unchecked((uint)(int)(sbyte)flags);

    /// <summary>실제 문자의 하위 BYTE를 부호 있는 값으로 읽는다.</summary>
    private static int Character(byte code)
    {
        if ((sbyte)code < 0)
        {
            throw new ArgumentOutOfRangeException(nameof(code), "초기stamp signed 문자mapping 범위 오류");
        }
        return code;
    }

    /// <summary>현재 좌표의 X열 우선 index를 얻고 원본 지도 범위 밖 읽기는 진단한다.</summary>
    private static int Index(int x, int y)
    {
        if (x < 0 || x >= 256 || y < 0 || y >= 256)
        {
            throw new ArgumentOutOfRangeException(nameof(x), "초기stamp 지도 범위 오류");
        }
        return x * 256 + y;
    }

    /// <summary>기준 alias/지역과 실제 이웃이 일치하면 true이며 bounds 검사에는 끝 칸을 포함한다.</summary>
    private bool Same(int x, int y, int alias, byte region)
    {
        int[] bounds = _map.Bounds;
        if (x < bounds[0] || y < bounds[1] || x > bounds[2] || y > bounds[3])
        {
            return false;
        }
        int index = Index(x, y);
        return _map.Aliases[Character(_map.Codes[index])] == alias && _map.Regions[index] == region;
    }

    /// <summary>첫 호출에서 현재 공유 CRT state를 99회 소비하며 직전 mod 4 중복을 1 증가시킨다.</summary>
    private void Initialize()
    {
        if (_state.Variants is not null)
        {
            return;
        }
        int[] variants = new int[99];
        _state.Variants = variants;
        // 원본 99개 연속 _rand와 앞 원소의 signed mod 4 비교 순서를 그대로 수행한다.
        for (int index = 0; index < variants.Length; index++)
        {
            uint nextCrt = unchecked(_getCrtState() * CrtMultiplier + CrtIncrement);
            _setCrtState(nextCrt);
            int value = (int)((nextCrt >> 16) & CrtMask);
            if (index > 0 && variants[index - 1] % 4 == value % 4)
            {
                value++;
            }
            variants[index] = value;
        }
    }

    /// <summary>원본 frame 필드만 DWORD 또는 BYTE로 직접 쓴다. 가상 SetFrame을 호출하지 않는다.</summary>
    private void Frame(int sid, int frame)
    {
        Span<byte> raw = _pool.AllocatedBytes(sid);
        uint bits = unchecked((uint)frame);
        int offset = _pool.Edition == OriginalEdition.Patch1078 ? 36 : 34;
        int width = _pool.Edition == OriginalEdition.Patch1078 ? 4 : 1;
        // 실제 필드 폭의 바이트만 little endian으로 쓴다.
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(bits >> (index * 8));
        }
    }

    /// <summary>단일 지면의 실제 factory/owner/frame/Pop/region/fringe 사슬을 수행한다.</summary>
    private void Create(int x, int y, uint type, int frame, int region)
    {
        int sid = _hooks.Create!(type, _map.MakePredictable);
        _map.LastSid = sid;
        _hooks.SetOwner!(sid, 0u);
        Frame(sid, frame);
        byte actualType = _pool.Slot(sid)[10];
        if (actualType >= _types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "초기stamp 생성 타입 범위 오류");
        }
        _hooks.Pop!(sid, x, y, PopFlags);
        if ((_types[actualType].Flags2 & 2u) == 0)
        {
            return;
        }
        if (region == _state.NoRegion)
        {
            if (_pool.Edition == OriginalEdition.Patch1078)
            {
                _hooks.Assertion!(TerrainStampAssertion.MissingRegion);
            }
            region = _hooks.ChunkRegion!(x / 16, y / 16);
        }
        _hooks.SetRegion!(sid, region);
        IReadOnlyList<RiftFrameCode> codes = _frames[actualType].Codes;
        if (frame < 0 || frame >= codes.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(frame), "초기stamp 생성 프레임 범위 오류");
        }
        RiftFrameCode code = codes[frame];
        if ((code.Flags & 1u) == 0)
        {
            return;
        }
        if (_state.FringeType >= (uint)_frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(type), "초기stamp fringe 타입 범위 오류");
        }
        int selected;
        // 원본은 후보 flags를 -1 판정 전에 읽으며 제외 flags가 겹치면 난수를 다시 소비한다.
        do
        {
            selected = RawFrameChoose.ChooseTerrainFrame(_frames[(int)_state.FringeType], code, -1, _random);
            byte flags = selected == -1 ? _state.FringePrefixFlags : _frames[(int)_state.FringeType].Codes[selected].Flags;
            if ((_map.ExcludedFlags & SignedFlags(flags)) == 0)
            {
                break;
            }
        }
        while (true);

        if (selected == -1)
        {
            return;
        }
        if (_map.MakePredictable == 0)
        {
            _hooks.Assertion!(TerrainStampAssertion.UnpredictableFringe);
        }
        int fringe = _hooks.Create(_state.FringeType, _map.MakePredictable);
        Frame(fringe, selected);
        _hooks.Pop(fringe, x, y + 4.0f, PopFlags);
    }
}
