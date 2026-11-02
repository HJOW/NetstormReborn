using Netstorm.Core.Simulation;

namespace Netstorm.Core.Original;

/// <summary>
/// 객체·내용물 로더가 각 분기에서 공유하는 원본 작업 영역이다(cpppj <c>o/RawFortObjectLoad.h</c>의 <c>FortObjectLoadState</c>).
/// 미션 기본 소유자는 Load의 Template 칸으로 전달한다.
/// </summary>
/// <remarks>2026-11-03 F2c-2에서 추가했다. 원문의 참조·포인터 공유는 읽기·쓰기 대리자와 직접 필드로 옮겼다.</remarks>
public sealed class FortObjectLoadState
{
    /// <summary>005c89c0의 manager 생성 지연과 같은 실제 DWORD를 빌린다. 호출보다 오래 살아야 한다.</summary>
    public uint Version { get; set; }

    /// <summary>005c89b8의 중첩 로딩 깊이(Terr1/Chaff2 구분 진입)다.</summary>
    public uint LoadingDepth { get; set; }

    /// <summary>005c89b0의 분기 영역이다.</summary>
    public uint Scope { get; set; }

    /// <summary>실제 PE의 다리 타입 영역이다.</summary>
    public uint BridgeType { get; set; } = 82;

    /// <summary>실제 PE의 island 타입 영역이다. 지면 isle97·98과 구별한다.</summary>
    public uint IslandType { get; set; } = 94;

    /// <summary>실제 PE의 큰 섬 타입 영역이다.</summary>
    public uint LargeIslandType { get; set; } = 95;

    /// <summary>실제 PE의 noIsland 타입 영역이다.</summary>
    public uint NoIslandType { get; set; } = 157;

    /// <summary>원본 PE 초기 mask다.</summary>
    public uint OwnerSavedMask { get; set; } = 0x5d77cf00;

    /// <summary>원본 PE 초기 mask다.</summary>
    public uint OwnerClearedMask { get; set; } = 0x10002000;

    /// <summary>원본 PE 초기 mask다.</summary>
    public uint FortKeepMask { get; set; } = 0x40480002;

    /// <summary>편집·미션·Fort·Chaff 사용과 편집 raw의 Fort 파괴 차단 fallback이다.</summary>
    public bool Special { get; set; }

    /// <summary>미션 사용 여부다.</summary>
    public bool Mission { get; set; }

    /// <summary>Fort 사용 여부다.</summary>
    public bool Fort { get; set; }

    /// <summary>Chaff 모드 여부다.</summary>
    public bool ChaffMode { get; set; }

    /// <summary>Fort 파괴 차단 여부다.</summary>
    public bool FortDestroyBlocked { get; set; }

    /// <summary>현재 기본 지형 크기다. 이후 실행에서 실제 값으로 비교한다.</summary>
    public int ChunksX { get; set; } = 16;

    /// <summary>현재 기본 지형 크기다.</summary>
    public int ChunksY { get; set; } = 16;

    /// <summary>0054263c의 실제 flags2와 비교하며 fortKeepMask와 다른 영역이다.</summary>
    public uint BattleSavedMask { get; set; } = 0x40480002;

    /// <summary>선택한 054db14 값이 비어 있지 않으면 실제 owner가 저장 로더보다 오래 살아야 한다. null이면 내부 값을 쓴다.</summary>
    public Func<uint>? SharedFullDeck { get; set; }

    /// <summary>실제 Client의 0542640을 빌린다. null이면 내부 값을 쓴다.</summary>
    public Func<uint>? SharedFortKeepGenus { get; set; }

    /// <summary>후보마다 실제 현재 마스크를 읽는다.</summary>
    public uint FortKeepGenus()
    {
        return SharedFortKeepGenus is null ? FortKeepMask : SharedFortKeepGenus();
    }

    /// <summary>Pop·Contents 효과 뒤 실제 DWORD를 읽으며 편집 raw 입력의 기존 bool을 사용한다.</summary>
    public bool IsFortDestroyBlocked()
    {
        return SharedFullDeck is null ? FortDestroyBlocked : SharedFullDeck() != 0;
    }
}

/// <summary>
/// 계산을 기본값으로 대체하지 않는 실제 가변 필드 경계와 반환값의 실제 raw·type 사용이다(cpppj <c>o/RawFortObjectLoad.h</c>의 <c>FortObjectLoadHooks</c>).
/// 큰 하위 몸체만 연결하고 소유자·HP·Pop·파괴 분배 경계는 호출자가 둔다.
/// </summary>
/// <remarks>2026-11-03 F2c-2에서 추가했다. 검사가 일부 장치를 빼고 호출하므로 속성은 읽고 쓸 수 있게 열었다.</remarks>
public sealed class FortObjectLoadHooks
{
    /// <summary>실제 factory(type,0)다.</summary>
    public Func<uint, uint, int>? Create { get; set; }

    /// <summary>WORD 지정 뒤 실제 다리 표시면 알린다.</summary>
    public Action<int>? NotifySurface { get; set; }

    /// <summary>가변 +0x84·+0x74며 실제 재생 규칙을 분배한다. 앞이 상태, 뒤가 소유자다.</summary>
    public Action<int, uint>? SetState { get; set; }

    /// <summary>가변 소유자 쓰기다.</summary>
    public Action<int, uint>? SetOwner { get; set; }

    /// <summary>실제 dais·abstract·flags1 판정이다.</summary>
    public Func<int, bool>? HasHitPoints { get; set; }

    /// <summary>실제 최대 HP·가변 +0x80이다.</summary>
    public Func<int, int>? MaxHitPoints { get; set; }

    /// <summary>실제 최대 HP 조회다. Fort 파괴 분기에서 가상 카운트로 쓴다.</summary>
    public Func<int, int>? VirtualCount { get; set; }

    /// <summary>가변 +0x90이며 같은 객체의 flags9다.</summary>
    public Action<int, float, float, uint>? Pop { get; set; }

    /// <summary>Fort 편집 파괴 처리의 가변 Destroy0이다.</summary>
    public Action<int, uint>? Destroy { get; set; }

    /// <summary>실제 004afa70이다.</summary>
    public Func<int, uint, uint, uint, uint, uint, int>? CreateContent { get; set; }

    /// <summary>기본 지형 BYTE를 signed로 읽는다.</summary>
    public Func<int, int, sbyte>? ChunkRegion { get; set; }

    /// <summary>0046f200의 지정 소유 Terr 조건이다.</summary>
    public Func<sbyte, uint, int, bool>? TerritoryAllowed { get; set; }

    /// <summary>같은 월드 모듈의 현재 깊이 DWORD를 전달한다.</summary>
    public Action<uint>? LoadingDepthChanged { get; set; }

    /// <summary>040e1a0 후처리를 같은 깊이로 잇는다.</summary>
    public Action<uint>? FinishOwner { get; set; }
}

/// <summary>
/// 값형 Fort 해석의 데이터만 사용하며 결과는 실제 SID·표시·함수 수다(cpppj <c>o/RawFortObjectLoad</c>).
/// 모든 경계는 types·frames·state·pool·hash·random과 같은 객체보다 오래 살아야 한다.
/// </summary>
/// <remarks>2026-11-03 F2c-2에서 추가했다. 스트림 적재(ObjectStream·LoadStream 계열)는 F2c-3에서 잇는다.</remarks>
public sealed class RawFortObjectLoad
{
    /// <summary>공통 raw 읽기 WORD·HP와 기본 프레임 위치다.</summary>
    private const int TypeOffset = 10;

    /// <summary>직접 읽는 WORD 위치다.</summary>
    private const int WordOffset = 12;

    /// <summary>HP 위치다.</summary>
    private const int HpOffset = 26;

    /// <summary>패치 프레임 DWORD 위치다.</summary>
    private const int PatchFrameOffset = 36;

    /// <summary>CD 프레임 BYTE 위치다.</summary>
    private const int CdFrameOffset = 34;

    /// <summary>수 프레임 contained·dais 분류에 쓰는 flags1 변환 비트다.</summary>
    private const uint TerrainFrameBit = 0x200000;

    /// <summary>수 랜덤 분류에 쓰는 flags1 변환 비트다.</summary>
    private const uint RandomFrameBit = 0x100000;

    /// <summary>container 비트다.</summary>
    private const uint ContainerFlag = 0x20000;

    /// <summary>dais 비트다.</summary>
    private const uint DaisFlag = 0x400000;

    /// <summary>내용물 로컬 플래그다.</summary>
    private const uint ContentLocal = 0x10;

    /// <summary>내용물 필터 비트다.</summary>
    private const uint ContentFiltered = 0x100;

    /// <summary>기본 P/P 고정 요청의 방향 글자다.</summary>
    private const byte LetterP = (byte)'P';

    /// <summary>대체 방향 글자다.</summary>
    private const byte LetterA = (byte)'A';

    /// <summary>기본 P/P 후보 최대 개수다.</summary>
    private const int Candidates = 256;

    /// <summary>객체 슬롯 풀이다. 생성·조회·반납에 쓰는 SID 풀이다.</summary>
    private readonly RawSidPool _pool;

    /// <summary>좌표로 객체 정보를 찾는 공간 표시다.</summary>
    private readonly RawSquidHash _hash;

    /// <summary>타입 번호로 생성·플래그를 조회하는 타입 표다.</summary>
    private readonly IReadOnlyList<OriginalTypeRecord> _types;

    /// <summary>타입별 프레임 자료다.</summary>
    private readonly IReadOnlyList<RiftTypeFrames> _frames;

    /// <summary>원본 FUN_004558c0 난수다. 상태 0이면 0x0bad0bad로 바꾼 뒤 상태*0x10003+3의 상위 16비트를 limit으로 나눈 나머지를 둔다.</summary>
    private readonly NetstormRandom _random;

    /// <summary>객체·내용물 로더가 각 분기에서 공유하는 원본 작업 영역이다. 미션 기본 소유자는 Load의 Template 칸으로 전달한다.</summary>
    private readonly FortObjectLoadState _state;

    /// <summary>하위 생성·조회와 가변 동작을 연결하는 콜백 묶음이다.</summary>
    private readonly FortObjectLoadHooks _hooks;

    /// <summary>
    /// 원본 계산 전에 기본·같은 자료 읽기의 모든 실제 효과 경계를 검증한다.
    /// </summary>
    /// <param name="pool">객체 슬롯 풀이다.</param>
    /// <param name="hash">좌표로 객체 정보를 찾는 공간 표시다.</param>
    /// <param name="types">타입 번호 순서의 타입 표다.</param>
    /// <param name="frames">타입별 프레임 자료다.</param>
    /// <param name="random">원본 게임 난수다.</param>
    /// <param name="state">공유 작업 영역이다. 호출자가 소유한다.</param>
    /// <param name="hooks">하위 몸체와 실제 저장소·표시다.</param>
    /// <exception cref="ArgumentException">Fort 로더의 기본·자료·실제 효과 연결 오류다.</exception>
    public RawFortObjectLoad(RawSidPool pool, RawSquidHash hash, IReadOnlyList<OriginalTypeRecord> types, IReadOnlyList<RiftTypeFrames> frames, NetstormRandom random, FortObjectLoadState state, FortObjectLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(hash);
        ArgumentNullException.ThrowIfNull(types);
        ArgumentNullException.ThrowIfNull(frames);
        ArgumentNullException.ThrowIfNull(random);
        ArgumentNullException.ThrowIfNull(state);
        ArgumentNullException.ThrowIfNull(hooks);
        uint expected = pool.Edition == OriginalEdition.Patch1078 ? 188u : 171u;
        if ((uint)types.Count != expected || frames.Count != types.Count ||
            hooks.Create is null || hooks.NotifySurface is null || hooks.SetState is null || hooks.SetOwner is null ||
            hooks.HasHitPoints is null || hooks.MaxHitPoints is null || hooks.VirtualCount is null || hooks.Pop is null ||
            hooks.Destroy is null || hooks.CreateContent is null || hooks.ChunkRegion is null || hooks.TerritoryAllowed is null ||
            hooks.LoadingDepthChanged is null || hooks.FinishOwner is null)
        {
            throw new ArgumentException("Fort 로더의 기본·자료·실제 효과 연결 오류");
        }
        _pool = pool;
        _hash = hash;
        _types = types;
        _frames = frames;
        _random = random;
        _state = state;
        _hooks = hooks;
    }

    /// <summary>
    /// 도달한 미연결 장치만 거부하여 앞선 조회·생성의 부분 효과를 보존한다.
    /// </summary>
    /// <typeparam name="T">장치 대리자 형식(참조 형식).</typeparam>
    /// <param name="function">확인할 장치다.</param>
    /// <exception cref="InvalidOperationException">Fort 객체 적재의 실제 경계가 연결되지 않았다.</exception>
    private static T NeedObject<T>(T? function) where T : class
    {
        if (function is null)
        {
            throw new InvalidOperationException("Fort 객체 적재 실제 경계 미연결");
        }
        return function;
    }

    /// <summary>
    /// 비정렬 little endian으로 직접 payload를 보존한다. 필요한 바이트만 기록한다.
    /// </summary>
    /// <param name="raw">쓸 슬롯 바이트다.</param>
    /// <param name="offset">쓸 위치다.</param>
    /// <param name="value">쓸 값이다.</param>
    /// <param name="width">쓸 너비(BYTE 수)다.</param>
    private static void Put(Span<byte> raw, int offset, uint value, int width)
    {
        // 실제 지정 바깥 바이트만 기록한다
        for (int index = 0; index < width; index++)
        {
            raw[offset + index] = (byte)(value >> (index * 8));
        }
    }

    /// <summary>
    /// 비정렬 little endian DWORD를 읽는다.
    /// </summary>
    /// <param name="raw">읽을 바이트다.</param>
    /// <param name="offset">읽을 위치다.</param>
    private static uint Get(ReadOnlySpan<byte> raw, int offset)
    {
        uint value = 0;
        // 낮은 바이트부터 네 바이트만 합친다
        for (int index = 0; index < 4; index++)
        {
            value |= (uint)raw[offset + index] << (index * 8);
        }
        return value;
    }

    /// <summary>
    /// signed 원본 좌표에 DWORD 감산 셈을 한다.
    /// </summary>
    /// <param name="origin">원점 좌표다.</param>
    /// <param name="value">지를 값이다.</param>
    private static int Add(int origin, uint value)
    {
        return unchecked((int)(unchecked((uint)origin) + value));
    }

    /// <summary>
    /// 실제 분기가 요구하는 필드가 없으면 앞의 0 대신 명시 오류로 거부한다.
    /// </summary>
    /// <typeparam name="T">값 형식.</typeparam>
    /// <param name="value">확인할 값이다.</param>
    /// <param name="field">누락 시 표시할 필드 이름이다.</param>
    /// <exception cref="InvalidOperationException">Fort 적재 필수 필드 누락이다.</exception>
    private static T Required<T>(T? value, string field) where T : struct
    {
        if (value is null)
        {
            throw new InvalidOperationException("Fort 적재 필수 필드 누락: " + field);
        }
        return value.Value;
    }

    /// <summary>
    /// 참조형 필수 필드가 없으면 명시 오류로 거부한다.
    /// </summary>
    /// <param name="value">확인할 값이다.</param>
    /// <param name="field">누락 시 표시할 필드 이름이다.</param>
    /// <exception cref="InvalidOperationException">Fort 적재 필수 필드 누락이다.</exception>
    private static FortContentList Required(FortContentList? value, string field)
    {
        if (value is null)
        {
            throw new InvalidOperationException("Fort 적재 필수 필드 누락: " + field);
        }
        return value;
    }

    /// <summary>
    /// callback이 바꾼 현재 타입을 읽으며 객체 할당 뒤에 타입 범위를 확인한다.
    /// </summary>
    /// <param name="sid">조회할 슬롯 번호다.</param>
    private OriginalTypeRecord Type(int sid)
    {
        ReadOnlySpan<byte> raw = _pool.Slot(sid);
        uint type = raw[TypeOffset];
        if (type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "Fort 적재 타입 범위 오류");
        }
        return _types[(int)type];
    }

    /// <summary>
    /// 초기 무작위 수 프레임을 표시 갱신 없이 기본 필드에 직접 둔다.
    /// </summary>
    /// <param name="sid">쓸 슬롯 번호다.</param>
    /// <param name="value">쓸 프레임 값이다.</param>
    private void Frame(int sid, uint value)
    {
        if (_pool.Edition == OriginalEdition.Patch1078)
        {
            Put(_pool.AllocatedBytes(sid), PatchFrameOffset, value, 4);
        }
        else
        {
            Put(_pool.AllocatedBytes(sid), CdFrameOffset, value, 1);
        }
    }

    /// <summary>
    /// QA 끝 BYTE·QB WORD 값을 현재 다리 영역과 비교한다.
    /// </summary>
    /// <param name="sid">쓸 슬롯 번호다.</param>
    /// <param name="value">쓸 WORD 값이다.</param>
    private void Word(int sid, uint value)
    {
        Put(_pool.AllocatedBytes(sid), WordOffset, value, 2);
        if (_pool.Slot(sid)[TypeOffset] == _state.BridgeType)
        {
            NeedObject(_hooks.NotifySurface)(sid);
        }
    }

    /// <summary>
    /// Fort 좌표를 같은 signed 정수에서 float로 변환했다. 같은 레벨의 일반 finder로 island genus를 선택한다.
    /// </summary>
    /// <param name="x">월드 x 좌표다.</param>
    /// <param name="y">월드 y 좌표다.</param>
    private int Terrain(float x, float y)
    {
        int ix = (int)x;
        int iy = (int)y;
        RawSquidFinder finder = new(_pool, _hash, _types);
        // finder 자체는 void·footprint·수행 링크 규칙을 쓰며 별도 helper에서 추측 상태 전파를 쓰지 않는다
        for (int sid = finder.Begin(new SquidSearchArea(ix, iy, ix, iy)); sid != 0; sid = finder.Next())
        {
            if ((Type(sid).Flags2 & TypeFlag2.Island) != 0)
            {
                return sid;
            }
        }
        return 0;
    }

    /// <summary>
    /// P/P fallback 변환을 실제 원본 PE의 방향 mask에서 P=0 계산으로 찾는다. 항상 P/P를 찾고 P/A, P/무 변환을 찾는다.
    /// </summary>
    /// <param name="sid">고를 슬롯 번호다.</param>
    private int RandomFrame(int sid)
    {
        uint type = _pool.Slot(sid)[TypeOffset];
        if (type >= (uint)_frames.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), sid, "Fort 랜덤 프레임 자료 오류");
        }
        IReadOnlyList<RiftFrameCode> codes = _frames[(int)type].Codes;
        int[] candidates = new int[Candidates];
        int count = 0;
        // 실제 고정 wildcard 요청은 P/P만으로, 다음은 P/A, P/무 변환 순으로 찾는다
        for (int pass = 0; pass < 4 && count == 0; pass++)
        {
            // 최대 256 후보를 초과하면 나머지 코드는 계속 조회한다
            for (int index = 0; index < codes.Count; index++)
            {
                RiftFrameCode code = codes[index];
                if (code.Side == LetterP && (pass == 3 || code.Variant == (pass == 2 ? LetterA : LetterP)) && count < Candidates)
                {
                    candidates[count++] = index;
                }
            }
        }
        return count == 0 ? -1 : candidates[_random.Next(count)];
    }

    /// <summary>
    /// 004bdc60·CD00428320: 청크 시작 칸에 position을 nibble로 깔고 초기 필드·소유자·HP·프레임→Pop9·내용물 사용을 적용한다.
    /// </summary>
    /// <param name="object">값형 Fort 해석 객체다.</param>
    /// <param name="originX">청크 원점 x다.</param>
    /// <param name="originY">청크 원점 y다.</param>
    /// <param name="defaultOwner">미션 기본 소유자다.</param>
    public int Object(FortObject @object, int originX, int originY, uint defaultOwner)
    {
        ArgumentNullException.ThrowIfNull(@object);
        uint owner = defaultOwner;
        uint type = @object.StoredType >= 0xf6 ? _state.BridgeType : unchecked((uint)@object.Type);
        if (@object.StoredType >= 0xf6)
        {
            owner = 255u - @object.StoredType;
        }
        if (type == 0 || type == _state.IslandType)
        {
            return 0;
        }
        if (type >= (uint)_types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(@object), "Fort 객체 타입 범위 오류");
        }
        float x = Add(originX, (uint)@object.CellX);
        float y = Add(originY, (uint)@object.CellY);
        int sid = NeedObject(_hooks.Create)(type, 0);
        Type(sid);
        if ((Type(sid).Flags1 & TypeFlag1.SaveFrame) != 0)
        {
            Frame(sid, Required(@object.Frame, "frame"));
        }
        if ((Type(sid).Flags1 & TypeFlag1.SaveQA) != 0)
        {
            Word(sid, ((uint)_pool.Slot(sid)[WordOffset + 1] << 8) | Required(@object.QuantityByte, "quantityByte"));
        }
        if ((Type(sid).Flags1 & TypeFlag1.SaveQB) != 0)
        {
            Word(sid, unchecked((uint)Required(@object.QuantityWord, "quantityWord")));
        }
        if ((Type(sid).Flags2 & TypeFlag2.Bridge) != 0)
        {
            Frame(sid, Required(@object.BridgeFrame, "bridgeFrame"));
        }
        if (_state.Version == 0)
        {
            NeedObject(_hooks.SetState)(sid, Required(@object.LegacyState, "legacyState"));
        }
        if (unchecked((int)_state.Version) > 1 && (Type(sid).Flags2 & TypeFlag2.Factory) != 0)
        {
            NeedObject(_hooks.SetState)(sid, Required(@object.FactoryState, "factoryState"));
        }
        if (unchecked((int)_state.Version) > 0)
        {
            if ((Type(sid).Flags2 & _state.OwnerSavedMask) != 0)
            {
                string context = "owner (stored=" + @object.StoredType + ", requested=" + @object.Type + ", current=" + _pool.Slot(sid)[TypeOffset] + ")";
                owner = Required(@object.Owner, context);
                if (owner == 0 || owner > 8)
                {
                    owner = 1;
                }
                if (!_state.Special && !_state.Mission)
                {
                    owner = defaultOwner;
                }
            }
        }
        else
        {
            owner = defaultOwner;
        }
        if ((Type(sid).Flags2 & _state.OwnerClearedMask) != 0)
        {
            owner = 0;
        }
        uint currentType = _pool.Slot(sid)[TypeOffset];
        if (currentType == _state.IslandType || currentType == _state.NoIslandType)
        {
            owner = 0;
        }
        if (currentType == _state.LargeIslandType)
        {
            throw new InvalidOperationException("Fort isleBig 로드 원본 assert");
        }
        NeedObject(_hooks.SetOwner)(sid, owner);
        if (NeedObject(_hooks.HasHitPoints)(sid))
        {
            int hp = NeedObject(_hooks.MaxHitPoints)(sid);
            Put(_pool.AllocatedBytes(sid), HpOffset, unchecked((uint)hp), _pool.Edition == OriginalEdition.Patch1078 ? 4 : 2);
        }
        if ((Type(sid).Flags1 & TerrainFrameBit) != 0)
        {
            int ground = Terrain(x, y);
            if (ground != 0)
            {
                ReadOnlySpan<byte> raw = _pool.Slot(ground);
                uint frame = raw[CdFrameOffset];
                if (_pool.Edition == OriginalEdition.Patch1078)
                {
                    frame = 0;
                    // 원본 DWORD 전체를 읽는다
                    for (int index = 0; index < 4; index++)
                    {
                        frame |= (uint)raw[PatchFrameOffset + index] << (index * 8);
                    }
                    if (frame == 0xffffffffu)
                    {
                        frame = 0;
                    }
                }
                Frame(sid, frame);
            }
        }
        if ((Type(sid).Flags1 & RandomFrameBit) != 0)
        {
            int frame = RandomFrame(sid);
            Frame(sid, frame == -1 ? 0u : unchecked((uint)frame));
        }
        NeedObject(_hooks.Pop)(sid, x, y, 9);
        if ((Type(sid).Flags1 & ContainerFlag) != 0)
        {
            Contents(sid, Required(@object.Contents, "contents"), 9);
        }
        if (_state.Fort && !_state.IsFortDestroyBlocked())
        {
            uint genus = Type(sid).Flags2;
            if ((genus & _state.FortKeepGenus()) == 0 ||
                (!_state.Special && !_state.Mission && (Type(sid).Flags2 & DaisFlag) != 0 && NeedObject(_hooks.VirtualCount)(sid) == 1))
            {
                NeedObject(_hooks.Destroy)(sid, 0);
            }
        }
        return sid;
    }

    /// <summary>
    /// 004bd130·CD00426dd0: 목록의 attach flags 변경 뒤 같은 실제값도 없으며 중첩 호출에서도 같은 값으로 전달한다.
    /// </summary>
    /// <param name="parent">붙일 부모 슬롯 번호다.</param>
    /// <param name="contents">내용물 목록이다.</param>
    /// <param name="flags">전달 플래그다.</param>
    public bool Contents(int parent, FortContentList contents, uint flags = 9)
    {
        ArgumentNullException.ThrowIfNull(contents);
        // 각 목록 밖에서 Local 플래그를 켜고 개별 실제값도 다룬다
        foreach (FortContent item in contents.Items)
        {
            if (item.Type == 0)
            {
                return false;
            }
            if (item.Type < 0 || item.Type >= _types.Count)
            {
                throw new ArgumentOutOfRangeException(nameof(contents), "Fort 내용물 타입 범위 오류");
            }
            uint savedFlags = _types[item.Type].Flags1;
            uint listFlags = _types[item.Type].ContentListFlags;
            if (listFlags == 0)
            {
                listFlags = Type(parent).ContainerListFlags;
            }
            if (listFlags == 0)
            {
                if (!item.ListFlagsStored)
                {
                    throw new InvalidOperationException("Fort 적재 내용물 부모 요구 listFlags 저장 바이트 누락");
                }
                listFlags = item.ListFlags;
            }
            if ((listFlags & ContentLocal) != 0)
            {
                flags = (flags & ~8u) | 16u;
            }
            int child = NeedObject(_hooks.CreateContent)(parent, unchecked((uint)item.Type), unchecked((uint)item.Quantity), listFlags & ~ContentFiltered, flags, 0);
            if ((savedFlags & ContainerFlag) != 0)
            {
                FortContentList nested = new() { Items = item.Contents, Complete = item.ContentsComplete };
                if (!Contents(child, nested, flags))
                {
                    return false;
                }
            }
        }
        return contents.Complete;
    }

    /// <summary>
    /// 004be160·CD004288a0: 청크 좌표를 16배하고 같은 레벨에서 실제 객체를 적용한다.
    /// </summary>
    /// <param name="chunk">청크 해석이다.</param>
    /// <param name="x">청크 x 칸이다.</param>
    /// <param name="y">청크 y 칸이다.</param>
    /// <param name="defaultOwner">미션 기본 소유자다.</param>
    public void Chunk(FortChunk chunk, int x, int y, uint defaultOwner)
    {
        ArgumentNullException.ThrowIfNull(chunk);
        int originX = unchecked((int)(unchecked((uint)x) << 4));
        int originY = unchecked((int)(unchecked((uint)y) << 4));
        // 청크 각 객체 순서대로 즉시 필드를 바꾼다
        foreach (FortObject @object in chunk.Objects)
        {
            Object(@object, originX, originY, defaultOwner);
        }
    }

    /// <summary>
    /// 004be610·CD00428d50: Terr[start,end) 적용 뒤 Chaff, 청크 섹션은 y 바깥·x 오른쪽 순서다.
    /// Template은 0..8 할당 기본 소유자다. 위치값의 각 4칸은 0이며 CD는 기본 assert 외에 거부한다.
    /// </summary>
    /// <param name="section">청크 섹션 해석이다.</param>
    /// <param name="owner">적을 소유자다.</param>
    /// <param name="territory">적용 영역 번호다. Chaff는 -1이다.</param>
    public void Section(FortChunkSection section, uint owner, int territory)
    {
        ArgumentNullException.ThrowIfNull(section);
        _state.Version = section.Version;
        if (!section.Present)
        {
            return;
        }
        _state.Scope = territory < 0 ? 2u : 1u;
        int index = 0;
        // 기본은 y 바깥 회전이며 각 반복에서 실제 지역 읽기를 둔다
        for (int y = 0; y < _state.ChunksY; y++)
        {
            // x가 오른쪽이며 책갈피 callback이 바꾼 수행 지역 범위를 다음 비교에서 반영한다
            for (int x = 0; x < _state.ChunksX; x++)
            {
                sbyte region = NeedObject(_hooks.ChunkRegion)(x, y);
                if (territory >= 0 && !NeedObject(_hooks.TerritoryAllowed)(region, owner, territory))
                {
                    continue;
                }
                if (index >= section.Chunks.Count)
                {
                    throw new ArgumentOutOfRangeException(nameof(section), "Fort 책갈피 청크 자료 부족");
                }
                Chunk(section.Chunks[index++], x, y, owner);
            }
        }
    }

    /// <summary>
    /// 단위 depth·기본·Terr 순서와 후처리까지 실제 필드 경계를 전달한다.
    /// </summary>
    /// <param name="fort">값형 Fort 해석이다.</param>
    /// <param name="templateSlot">고정 this의 Template 칸 번호다.</param>
    /// <param name="start">시작 영역 번호다.</param>
    /// <param name="end">끝 영역 번호다(미포함).</param>
    public void Load(FortTemplate fort, int templateSlot, int start = 0, int end = 20)
    {
        ArgumentNullException.ThrowIfNull(fort);
        if (start == end || start < 0 || end > 20)
        {
            throw new ArgumentException("Fort Terr 범위 오류", nameof(start));
        }
        if (_pool.Edition == OriginalEdition.Cd1072 && (templateSlot < 1 || templateSlot > 8))
        {
            throw new ArgumentOutOfRangeException(nameof(templateSlot), "CD Fort Template 소유물 원본 assert");
        }
        uint owner = templateSlot >= 1 && templateSlot <= 8 ? (uint)templateSlot : 0u;
        _state.LoadingDepth++;
        NeedObject(_hooks.LoadingDepthChanged)(_state.LoadingDepth);
        try
        {
            // Terr 번호는 start부터 end 직전까지 증가하며 섹션은 현재 버전을 사용한다
            for (int territory = start; territory < end; territory++)
            {
                Section(fort.Territories[territory], owner, territory);
            }
            if (_state.Fort || _state.ChaffMode)
            {
                Section(fort.Chaff, owner, -1);
            }
            NeedObject(_hooks.FinishOwner)(owner);
        }
        catch
        {
            _state.LoadingDepth--;
            NeedObject(_hooks.LoadingDepthChanged)(_state.LoadingDepth);
            throw;
        }
        _state.LoadingDepth--;
        NeedObject(_hooks.LoadingDepthChanged)(_state.LoadingDepth);
    }

    /// <summary>
    /// 실제 파일 공유 어댑터 검증에 사용하는 참조다.
    /// </summary>
    public RawSidPool Pool()
    {
        return _pool;
    }

    /// <summary>
    /// 같은 풀의 실제 생성·HP·내용물 모듈을 같은 영역에서 사용한다.
    /// </summary>
    /// <param name="pool">검사할 풀이다.</param>
    /// <param name="factory">같은 풀의 생성 모듈이다.</param>
    /// <param name="hp">같은 풀의 HP 모듈이다.</param>
    /// <param name="contained">같은 풀의 내용물 모듈이다.</param>
    /// <param name="hooks">물을 훅 묶음이다.</param>
    /// <exception cref="ArgumentException">Fort 생성·HP·내용물 SID 풀 불일치다.</exception>
    public static FortObjectLoadHooks MakeFortObjectLoadHooks(RawSidPool pool, RawSquidFactory factory, RawSquidReward hp, RawContainedCreate contained, FortObjectLoadHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(pool);
        ArgumentNullException.ThrowIfNull(factory);
        ArgumentNullException.ThrowIfNull(hp);
        ArgumentNullException.ThrowIfNull(contained);
        ArgumentNullException.ThrowIfNull(hooks);
        if (!ReferenceEquals(pool, factory.Pool) || !ReferenceEquals(pool, hp.Pool) || !ReferenceEquals(pool, contained.Pool))
        {
            throw new ArgumentException("Fort 생성·HP·내용물 SID 풀 불일치");
        }
        hooks.Create = (type, flags) => factory.Create(type, (SidAllocation)flags);
        hooks.HasHitPoints = hp.HasHitPoints;
        hooks.MaxHitPoints = hp.MaxHitPoints;
        hooks.CreateContent = contained.Create;
        return hooks;
    }
}
