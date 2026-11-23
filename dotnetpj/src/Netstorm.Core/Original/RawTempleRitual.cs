using System.Buffers.Binary;
using Netstorm.Core.Simulation;

// 신전 의식의 현재 단계·SHP 목표점·사제/룬 조회와 실제 하위 요청 순서를 복원한다.
namespace Netstorm.Core.Original;

/// <summary>RawTempleRitual.cpp의 신전 의식 전체몸체다.</summary>
public sealed class RawTempleRitual
{
    /// <summary>실제 dais +0x80의 현재 방향N/M/L 우선 및 frame64/120 경계의 실제 네 DWORD다. PATCH00532620이다.</summary>
    private static readonly int[] PatchCountFirst = [1, 3, 64, 120];
    /// <summary>CD0052e2b4의 실제 네 DWORD이며 count0의 인접 문자열 바이트도 임의0으로 바꾸지 않는다.</summary>
    private static readonly int[] CdCountFirst = [10611, 3, 64, 120];
    /// <summary>독립 입력에서 직접 Count로 연결할 수 있는 PATCH 실제 Dais VT다. 0x00507950이다.</summary>
    private const uint PatchCountVtable = 0x00507950;
    /// <summary>독립 입력에서 직접 Count로 연결할 수 있는 CD 실제 Dais VT다. 0x00504090이다.</summary>
    private const uint CdCountVtable = 0x00504090;
    /// <summary>원본 각 의식 단계의 첫 프레임 오프셋이다.</summary>
    private static readonly int[] StageOffsets = [0, 11, 22, 33, 44];
    /// <summary>현재 단계의 위치 음향이다.</summary>
    private static readonly string[] StageSounds = ["forWind2.wav", "forSun2.wav", "forRain2.wav", "forThunder2.wav", "forStorm2.wav"];
    /// <summary>실제0x307 payload의 원본 float 비트다.</summary>
    private static readonly float[] StageDelays = [1.9600000381469727F, 2.25F, 1.8799999952316284F, 2.2200000286102295F, 2.4100000858306885F];
    /// <summary>발자국 사각형 우하단만 원본처럼 거의올린다. 0041d7a0/CD00440a80다.</summary>
    private const float AlmostCeil = 0.9999899864196777F;
    /// <summary>의식 동작이 예약하는 후속 공유 사건이다. 0x307이다.</summary>
    private const uint RitualEvent = 0x307;
    /// <summary>유일한 현재 raw 저장소다.</summary>
    private readonly RawSidPool _pool;
    /// <summary>사제와Rune이 공유하는 실제 네 단계 해시다.</summary>
    private readonly RawSquidHash _hash;
    /// <summary>현재 타입genus와일반finder발자국이다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;
    /// <summary>현재dais방향을 기존PathAnimation에서 읽는 표다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;
    /// <summary>실제SHP핫스폿 메타 자료를 참조한다.</summary>
    private readonly IReadOnlyList<SquidDisplayShape> _shapes;
    /// <summary>마지막목표점의 지면 보정이읽는 지도다.</summary>
    private readonly IReadOnlyList<byte> _spots;
    /// <summary>기존실제발자국몸체를 재사용한다.</summary>
    private readonly RawSquidNeighbors _footprint;
    /// <summary>기존실제최대HP를 재사용한다.</summary>
    private readonly RawSquidReward _hitPoints;
    /// <summary>기존실제K..P선택/604예약몸체다.</summary>
    private readonly RawPriestRitualActions _actions;
    /// <summary>효과뒤현재권한/타입/픽셀scale다.</summary>
    private readonly TempleRitualState _state;
    /// <summary>모든하위요청의실제명시연결이다.</summary>
    private readonly TempleRitualHooks _hooks;
    /// <summary>도달한 장치만 요구하며 호출 중 자신의 연결 교체에도 수명을 유지한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("신전 의식의 선택된 실제 장치 미연결");
    /// <summary>정렬되지않은 현재 raw 필드를 지정 폭의 little endian 값으로 읽는다.</summary>
    private static uint Read(ReadOnlySpan<byte> raw, int offset, int width = 4)
    {
        uint value = 0;
        // CDBYTE/WORD 옆 필드를 섞지 않는다.
        for (int index = 0; index < width; index++) value |= (uint)raw[offset + index] << (8 * index);
        return value;
    }
    /// <summary>실제 finder 정상 좌표 범위를 0방향 절삭한다. 손상/무한 좌표의 원본 불확정 읽기는 실행하지 않는다.</summary>
    private static int Cell(double value)
    {
        if (!double.IsFinite(value) || value < -2147483648.0 || value >= 2147483648.0) throw new ArgumentOutOfRangeException(nameof(value), "신전 의식 좌표 절삭 범위");
        return (int)value;
    }
    /// <summary>표/프레임/SHP/spot은 원본 타입 인덱스와 같은 목록이며 다른 풀/해시 연결은 거부한다.</summary>
    public RawTempleRitual(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types,
        IReadOnlyList<RiftTypeFrames> frames, IReadOnlyList<SquidDisplayShape> shapes, IReadOnlyList<byte> spots,
        RawSquidNeighbors footprint, RawSquidReward hitPoints, RawPriestRitualActions actions,
        TempleRitualState state, TempleRitualHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames); ArgumentNullException.ThrowIfNull(shapes); ArgumentNullException.ThrowIfNull(spots);
        ArgumentNullException.ThrowIfNull(footprint); ArgumentNullException.ThrowIfNull(hitPoints); ArgumentNullException.ThrowIfNull(actions);
        ArgumentNullException.ThrowIfNull(state); ArgumentNullException.ThrowIfNull(hooks);
        int count = pool.Edition == OriginalEdition.Patch1078 ? 188 : 171;
        if (!ReferenceEquals(pool, footprint.Pool) || !ReferenceEquals(hash, footprint.Hash) || !ReferenceEquals(pool, hitPoints.Pool) || !ReferenceEquals(pool, actions.Pool)
            || types.Count != count || frames.Count != count || shapes.Count != count || spots.Count != RawSquidHash.WorldCells * RawSquidHash.WorldCells
            || (pool.Edition == OriginalEdition.Cd1072 && (hooks.Create is null || hooks.SetHitPoints is null || hooks.SetOwner is null || hooks.Pop is null || hooks.PlayAt is null || hooks.ReserveShared is null || hooks.AddShared is null)))
            throw new ArgumentException("신전 의식 실제 하위/판본 연결 누락", nameof(footprint));
        _pool = pool; _hash = hash; _types = types; _frames = frames; _shapes = shapes; _spots = spots;
        _footprint = footprint; _hitPoints = hitPoints; _actions = actions; _state = state; _hooks = hooks.Copy();
    }
    /// <summary>실제 현재 raw 프레임은 패치signedDWORD/CDBYTE 폭이다.</summary>
    private int CurrentFrame(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return _pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[36..]) : raw[34];
    }
    /// <summary>효과 뒤의 현재 사제/신전 좌표를 raw에서 다시 읽는다.</summary>
    private TempleRitualPoint Position(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        return new(BinaryPrimitives.ReadSingleLittleEndian(raw[14..]), BinaryPrimitives.ReadSingleLittleEndian(raw[18..]));
    }
    /// <summary>owner의 판본별BYTE를 그대로 읽는다. 원본 effect 뒤에도 현재 owner를 읽도록 값을 보관하지 않는다.</summary>
    private uint Owner(int sid) => _pool.Slot(sid)[_pool.Edition == OriginalEdition.Patch1078 ? 34 : 32];
    /// <summary>선택된 실제 Dais +0x80은 TYPE165를 강제하지 않고 현재 TYPE 코드와 프레임을 읽는다. 상태 BYTE는 읽기의 거부 조건이 아니다.</summary>
    public uint Count(int temple)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(temple);
        if (raw[10] >= _types.Count || raw[10] >= _frames.Count) throw new ArgumentOutOfRangeException(nameof(temple), "Dais Count 현재 TYPE/프레임 표 범위 오류");
        int frame = CurrentFrame(temple); int direction = RawPathAnimation.Direction(_frames[raw[10]], frame);
        if (direction == 13) return 1; if (direction == 12) return 2; if (direction == 11) return 3;
        return frame < 64 ? 1u : frame <= 119 ? 2u : 3u;
    }
    /// <summary>같은 대상의 현재 가상+80을 조회하며 미연결 독립 입력은 실제 Dais VT에 한해 직접 Count를 사용한다.</summary>
    private uint VirtualCount(int temple)
    {
        if (_hooks.Count is not null) return Need(_hooks.Count)(temple);
        if (Read(_pool.Slot(temple), 0) != (_pool.Edition == OriginalEdition.Patch1078 ? PatchCountVtable : CdCountVtable))
            throw new InvalidOperationException("의식 Stage의 현재 가상80 분배 장치 미연결");
        return Count(temple);
    }
    /// <summary>현재 count별 첫 프레임을 빼고11/22/33/44/55 경계로0..5를 읽는다. frame-1은 현재 raw 프레임이다.</summary>
    public uint Stage(int temple, int frame = -1)
    {
        if (frame == -1) frame = CurrentFrame(temple); uint count = VirtualCount(temple);
        return StageFromCount(_pool.Edition, frame, count);
    }
    /// <summary>이미 저장한 frame과 실제 가상Count로 원문 단계 표를 조회한다. 가상 조회의 부작용 뒤 저장한 값만 소비하며 새 조회를 추가하지 않는다.</summary>
    public static uint StageFromCount(OriginalEdition edition, int frame, uint count)
    {
        int[] first = edition == OriginalEdition.Patch1078 ? PatchCountFirst : CdCountFirst;
        if (count >= (uint)first.Length) throw new ArgumentOutOfRangeException(nameof(count), "의식 Stage의 가상 count 첫 프레임 표 범위 오류");
        int relative = unchecked((int)(unchecked((uint)frame) - unchecked((uint)first[count])));
        if (relative < 11) return 0; if (relative < 22) return 1; if (relative < 33) return 2; if (relative < 44) return 3; return relative <= 54 ? 4u : 5u;
    }
    /// <summary>원본419850 반환 헤더+20/+22의 의식 목표점을 읽는다. 표시 기준점(+1c/+1e)과 구별한다.</summary>
    private (short X, short Y) Hotspot(uint type, int frame)
    {
        if (type >= (uint)_shapes.Count || frame < 0) throw new ArgumentOutOfRangeException(nameof(type), "신전 의식 SHP 타입/프레임 범위");
        SquidDisplayShape shape = _shapes[(int)type];
        long limit = (long)shape.FrameCount * (((_types[(int)type].Flags1 & 0x440000) != 0) ? 2 : 1);
        if (!shape.Loaded || frame >= limit || frame >= shape.Frames.Count) throw new ArgumentOutOfRangeException(nameof(frame), "신전 의식 SHP frameCheck 오류");
        SquidDisplayFrame current = shape.Frames[frame]; return (current.MuzzleX, current.MuzzleY);
    }
    /// <summary>각 축 정수픽셀/현재scale 덧셈 뒤단정도로 저장한다. 원본 x87의 int/float+float 계산이다.</summary>
    private TempleRitualPoint AddPixels(TempleRitualPoint point, int x, int y)
    {
        if (!float.IsFinite(_state.PixelScaleX) || !float.IsFinite(_state.PixelScaleY) || _state.PixelScaleX == 0 || _state.PixelScaleY == 0) throw new ArgumentOutOfRangeException(nameof(point), "신전 의식 픽셀 배율 오류");
        return new((float)((double)x / (double)_state.PixelScaleX + (double)point.X), (float)((double)y / (double)_state.PixelScaleY + (double)point.Y));
    }
    /// <summary>신전의 첫 count1 구간 핫스폿이 모든 단계 Anchor의 기준이다. 현재 count별 첫 프레임을 넣지 않는다.</summary>
    public TempleRitualPoint StageAnchor(int temple, uint stage)
    {
        if (stage >= (uint)StageOffsets.Length) throw new ArgumentOutOfRangeException(nameof(stage), "신전 의식 단계 범위 오류");
        (short X, short Y) hotspot = Hotspot(_pool.Slot(temple)[10], PatchCountFirst[1] + StageOffsets[stage]);
        return AddPixels(Position(temple), hotspot.X, hotspot.Y);
    }
    /// <summary>Anchor+룬SHP 핫스폿이며stage4의 절삭 칸 spot6/!8이면(-9,-6)픽셀 보정한다.</summary>
    public TempleRitualPoint StagePoint(int temple, uint stage)
    {
        TempleRitualPoint point = StageAnchor(temple, stage);
        (short X, short Y) hotspot = Hotspot(_state.RuneTypeCurrent(), (int)stage); point = AddPixels(point, hotspot.X, hotspot.Y);
        if (stage == 4)
        {
            long index = (long)Cell(point.Y) * RawSquidHash.WorldCells + Cell(point.X);
            if (index < 0 || index >= _spots.Count) throw new ArgumentOutOfRangeException(nameof(temple), "신전 의식 마지막 목표 지면 범위");
            byte spot = _spots[(int)index]; if ((spot & 6) != 0 && (spot & 8) == 0) point = AddPixels(point, -9, -6);
        }
        return point;
    }
    /// <summary>현재 단계의룬SHP 핫스폿을 빼고 실제 일반 점 finder의 첫 현재 runeType을 반환한다.</summary>
    public int RuneAt(int temple, TempleRitualPoint point)
    {
        uint stage = Stage(temple); uint type = _state.RuneTypeCurrent();
        if (type >= (uint)_types.Count) throw new ArgumentOutOfRangeException(nameof(temple), "신전 의식 현재Rune 타입 범위");
        (short X, short Y) hotspot = Hotspot(type, (int)stage); point = AddPixels(point, -hotspot.X, -hotspot.Y);
        int x = Cell(point.X), y = Cell(point.Y);
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 원본 첫 타입 일치에서 종료하며 별도dead/free/abstract 필터를 넣지 않는다.
        for (int sid = finder.Begin(new(x, y, x, y), (_types[(int)type].Flags2 & 6) == 0 ? 1u : 0u); sid != 0; sid = finder.Next())
            if (_pool.Slot(sid)[10] == type) return sid;
        return 0;
    }
    /// <summary>float 발자국을 사각형 ctor의 절삭 좌상단/거의올림 우하단으로 바꾼다.</summary>
    private static SquidSearchArea SearchArea(SquidFootprint area) =>
        new(Cell(area.Left), Cell(area.Top), Cell((double)area.Right + AlmostCeil), Cell((double)area.Bottom + AlmostCeil));
    /// <summary>신전 발자국±1의 일반 finder에서 같은 owner이며HP>=현재 최대HP/2인 첫 사제를 찾는다.</summary>
    public int FindStrongOwnedPriest(int temple) =>
        FindStrongOwnedPriestIn(_pool, _hash, _types, _footprint, _hitPoints, _state.PriestTypeCurrent(), temple);
    /// <summary>삭제의 장기 owner도 같은 현재 발자국/소유자/HP 조회 본문을 빌린다. 기존 실제 발자국만±1 확장하고 후보의 signed 현재HP와현재 최대HP를 비교한다.</summary>
    public static int FindStrongOwnedPriestIn(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, RawSquidNeighbors footprint,
        RawSquidReward hitPoints, uint priestType, int temple)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(hash); ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(footprint); ArgumentNullException.ThrowIfNull(hitPoints);
        SquidFootprint area = footprint.Footprint(temple);
        area = new(area.Left - 1, area.Top - 1, area.Right + 1, area.Bottom + 1);
        var finder = new RawSquidFinder(pool, hash, types);
        int ownerOffset = pool.Edition == OriginalEdition.Patch1078 ? 34 : 32;
        // 일반finder 순서에서 현재 type/owner/HP를 읽는다. 사제 자체extra/state는 조건이 아니다.
        for (int sid = finder.Begin(new(Cell(area.Left), Cell(area.Top), Cell((double)area.Right + AlmostCeil), Cell((double)area.Bottom + AlmostCeil))); sid != 0; sid = finder.Next())
        {
            ReadOnlySpan<byte> raw = pool.Slot(sid);
            if (raw[10] != priestType || raw[ownerOffset] != pool.Slot(temple)[ownerOffset]) continue;
            int hp = pool.Edition == OriginalEdition.Patch1078 ? BinaryPrimitives.ReadInt32LittleEndian(raw[26..]) : BinaryPrimitives.ReadInt16LittleEndian(raw[26..]);
            if (hp >= hitPoints.MaxHitPoints(sid) / 2) return sid;
        }
        return 0;
    }
    /// <summary>현재 실제 발자국과일반finder를 재사용해추상Rune 존재만읽는다.</summary>
    public bool HasAbstractRune(int temple)
    {
        int extra = _pool.Edition == OriginalEdition.Patch1078 ? 40 : 35; uint rune = _state.RuneTypeCurrent();
        var finder = new RawSquidFinder(_pool, _hash, _types);
        // 첫 현재RuneType/extra1 일치만 참이며 일반finder의 buried 제외를 그대로 유지한다.
        for (int sid = finder.Begin(SearchArea(_footprint.Footprint(temple))); sid != 0; sid = finder.Next())
        { ReadOnlySpan<byte> raw = _pool.Slot(sid); if (raw[10] == rune && (raw[extra] & 1) != 0) return true; }
        return false;
    }
    /// <summary>생성→HP0→현재stage 프레임→void assert→abstract1→현재owner→다시현재stage Anchor→Pop 순서다.</summary>
    public void SpawnRune(int temple)
    {
        int rune = Need(_hooks.Create)(_state.RuneTypeCurrent(), 0);
        if (rune == 0 || rune >= _pool.Capacity) throw new ArgumentOutOfRangeException(nameof(temple), "신전 의식Rune 생성SID 오류");
        Need(_hooks.SetHitPoints)(rune, 0); uint stage = Stage(temple); bool patch = _pool.Edition == OriginalEdition.Patch1078;
        Span<byte> raw = _pool.RawSlot(rune);
        // 원본 패치DWORD/CDBYTE 프레임만 쓰고 옆필드는 보존한다.
        for (int index = 0; index < (patch ? 4 : 1); index++) raw[(patch ? 36 : 34) + index] = (byte)(stage >> (8 * index));
        if ((raw[11] & 4) == 0)
        {
            if (patch) Need(_hooks.Assertion)("isVoid()", "c:\\ns\\o\\squid.h", 0x282);
            else throw new InvalidOperationException("신전 의식Rune isVoid assert");
        }
        raw[patch ? 40 : 35] |= 1; Need(_hooks.SetOwner)(rune, Owner(temple));
        TempleRitualPoint point = StageAnchor(temple, Stage(temple)); Need(_hooks.Pop)(rune, point.X, point.Y, 0);
    }
    /// <summary>전체00447e40/CD00481fc0: 목표점 일치와Rune 없음 뒤mode0은frame/음향/307,나머지는604후속/권한Rune이다.</summary>
    public void Ritual(int temple, int priest, uint mode)
    {
        uint stage = Stage(temple); if (stage > 4) return;
        TempleRitualPoint expected = StagePoint(temple, stage); TempleRitualPoint actual = Position(priest);
        if (actual.X != expected.X || actual.Y != expected.Y)
        {
            if (stage >= 4) return;
            expected = StagePoint(temple, stage + 1); actual = Position(priest);
            if (actual.X != expected.X || actual.Y != expected.Y) return;
        }
        if (RuneAt(temple, Position(priest)) != 0) return;
        if (mode == 0)
        {
            _actions.Begin((uint)priest, stage); actual = Position(priest);
            Need(_hooks.PlayAt)(actual.X, actual.Y, StageSounds[stage], 0);
            if (Need(_hooks.ReserveShared)()) Need(_hooks.AddShared)(temple, RitualEvent, StageDelays[stage]); return;
        }
        int found = FindStrongOwnedPriest(temple);
        if (found != 0 && found < _pool.Capacity) _actions.Continue((uint)found, Stage(temple));
        if (_state.BossCurrent() && !HasAbstractRune(temple)) SpawnRune(temple);
    }
    /// <summary>동일 월드의 PathCleanup/Host 연결을 검증한다.</summary>
    public RawSidPool Pool => _pool;
    /// <summary>공통몸체는새로계산하지않으며필수Rune Pop과위치음향은명시연결로남긴다. 실제 Factory/공통HP setter/Owner/Host를 연결한다.</summary>
    public static TempleRitualHooks MakeTempleRitualHooks(RawSidPool pool, RawSquidFactory factory, SquidOwner owner, SquidProcessHost host, TempleRitualHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool); ArgumentNullException.ThrowIfNull(factory); ArgumentNullException.ThrowIfNull(owner);
        ArgumentNullException.ThrowIfNull(host); ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, factory.Pool) || !ReferenceEquals(pool, owner.Pool) || !ReferenceEquals(pool, host.Pool())) throw new ArgumentException("신전 의식 Factory/owner/Host SID 풀 불일치", nameof(host));
        hooks.Create = (type, flags) => factory.Create(type, (SidAllocation)flags);
        hooks.SetHitPoints = (sid, value) => RawSquidHitPoints.Set(pool, sid, value);
        hooks.SetOwner = (sid, player) => owner.Set(sid, player);
        hooks.AddShared = (sid, @event, payload) => { host.AddSharedRegular(sid, @event, payload); };
        return hooks;
    }
}
