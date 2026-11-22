// Native 장기 자원 장치와 너겟/간헐천 실제 부모를 같은 저장소 수명에 연결한다.
// cpppj/src/client/RawSurfaceWorld.h 의 RawSurfaceResourceDevices 구조와
// cpppj/src/client/RawSurfaceStorage.cpp 의 BindResourceDevices/AllocateResourceRegular/ConstructResourceRegular 와
// ResourceCarrierPostPop/ResourceAssetPostPop/ResourceNuggetPostPop 장기 본문을 이식한다. 0.568에서 추가됐다. (R4-1 잔여 중 Native 기본 장기 장치 설치)
namespace Netstorm.Core.Original;

/// <summary>
/// 같은 Client 게임 난수와 선택된 큰 자산 장치를 장기로 빌리는 자원 장치 묶음이다.
/// 원본 RawSurfaceResourceDevices 와 같은 일곱 항목이며 짧은 Session 주소를 저장하지 않는다.
/// 0.568에서 추가됐다. (R4-1 Native 기본 장기 장치 설치)
///
/// </summary>
public sealed class RawSurfaceResourceDevices
{
    /// <summary>새 시드/스냅숏 없이 실제 현재 게임 난수 한 개를 조회한다. 0.568에서 추가됐다.</summary>
    public Func<GameRandom>? Random { get; set; }

    /// <summary>실제 가상 +0xCC 의 현재 운반체 검사를 선택 위치에서 호출한다. 0.568에서 추가됐다.</summary>
    public Action<int>? CarrierCheck { get; set; }

    /// <summary>실제 피해 사건의 현재 큰 세계 종단이다. 자연 기본 사건은 장기 본문이 직접 판정한다. 0.568에서 추가됐다.</summary>
    public Func<int, uint, uint, float, float>? DamageableRegular { get; set; }

    /// <summary>실제 추상/현재 폼 조회 뒤 건설 시작만 선택해 호출한다. 0.568에서 추가됐다.</summary>
    public Action<int>? StartConstruction { get; set; }

    /// <summary>선택된 현재 가상 삭제의 수락/거부를 그대로 반환한다. 미복원을 성공으로 꾸미지 않는다. 0.568에서 추가됐다.</summary>
    public Func<int, uint, bool>? Destroy { get; set; }

    /// <summary>현재 Player 9 기록/두뇌 순회의 실제 등록 부모다. 0.568에서 추가됐다.</summary>
    public Action<int, uint>? GeyserRegistration { get; set; }

    /// <summary>벌룬의 비 3 사건에서만 현재 실제 운반체 본체와 전체 런타임을 빌린다. 필수 설치는 아니다. 0.568에서 추가됐다.</summary>
    public Func<int, uint, uint, float, float>? CarrierRegular { get; set; }

    /// <summary>원문 함수 묶음처럼 값만 복사하며 장치를 실행하지 않는다. 0.568에서 추가됐다.</summary>
    internal RawSurfaceResourceDevices Copy() => new()
    {
        Random = Random,
        CarrierCheck = CarrierCheck,
        DamageableRegular = DamageableRegular,
        StartConstruction = StartConstruction,
        Destroy = Destroy,
        GeyserRegistration = GeyserRegistration,
        CarrierRegular = CarrierRegular,
    };
}

/// <summary>World 없이 같은 장기 물리/장부로 자원·너겟·간헐천 실제 부모를 보유한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>같은 Client 장기 자원 장치의 함수 사본이다. 첫 자원 파생 전에만 설치한다. 0.568에서 추가됐다.</summary>
    private RawSurfaceResourceDevices _resourceDevices = new();

    /// <summary>같은 확보 주소에서 두 번째 할당 없이 재구성하는 장기 자원 예약 메모리다. 0.568에서 추가됐다.</summary>
    private RegularProcess? _resourceRegular;

    /// <summary>실제 공통 부모를 World 없이 같은 물리/현재 전역으로 구성하는 장기 운반체다. 0.568에서 추가됐다.</summary>
    private RawCarrierPostPop? _resourceCarrier;

    /// <summary>같은 장기 권한/로딩/간헐천 타입을 읽는 운반체 상태다. 0.568에서 추가됐다.</summary>
    private CarrierPostPopState? _resourceCarrierState;

    /// <summary>장기 간헐천 목록/다리 계열과 실제 권한을 같은 주소에 보존하는 자산 상태다. 0.568에서 추가됐다.</summary>
    private readonly AssetPostPopState _resourceAssetState = new();

    /// <summary>장기 자산 전용 후처리 본문을 한 번만 구성하고 World 반납 뒤에도 유지한다. 0.568에서 추가됐다.</summary>
    private RawAssetPostPop? _resourceGeyser;

    /// <summary>장기 너겟 전용 후처리 본문을 한 번만 구성하고 World 반납 뒤에도 유지한다. 0.568에서 추가됐다.</summary>
    private RawNuggetPostPop? _resourceNugget;

    /// <summary>실제 장기 자원 장치가 설치됐는지만 읽는다. 공장/Host/원문 값은 만들지 않는다. 0.568에서 추가됐다.</summary>
    public bool UsesNativeResourceDevices() => _resourceDevices.Random is not null;

    /// <summary>현재 Client 장치를 최초 보관하며 공장/Host/원문 값을 조기 구성하지 않는다. 0.568에서 추가됐다.</summary>
    /// <param name="devices">설치할 실제 장기 장치 묶음이다.</param>
    public void BindResourceDevices(RawSurfaceResourceDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        // Native 전송 장치가 먼저 있어야 자원 예약을 실제 방송과 같은 수명으로 묶는다.
        if (!UsesNativeTransmissionDevices() || _resourceDevices.Random is not null || _resourceCarrier is not null)
        {
            throw new ArgumentException("장기 자원 후처리의 실제 최초 Client 장치 계약", nameof(devices));
        }

        // 필수 여섯 항목이 모두 있어야 최초 간헐천/너겟 부모가 도달 위치에서 실패하지 않는다.
        if (devices.Random is null || devices.CarrierCheck is null || devices.GeyserRegistration is null
            || devices.DamageableRegular is null || devices.StartConstruction is null || devices.Destroy is null)
        {
            throw new ArgumentException("장기 자원 후처리의 실제 최초 Client 장치 계약", nameof(devices));
        }

        _resourceDevices = devices.Copy();
    }

    /// <summary>같은 확보 메모리의 실패를 그 시점에서만 반환한다. 0.568에서 추가됐다.</summary>
    public bool AllocateResourceRegular()
    {
        // 중복 확보는 원문처럼 두 번째 new 를 거부한다.
        if (_resourceRegular is not null)
        {
            throw new InvalidOperationException("장기 자원 Regular 의 확보 메모리 중복");
        }

        try
        {
            _resourceRegular = new RegularProcess(0, 0.0f);
        }
        catch (OutOfMemoryException)
        {
            return false;
        }

        return true;
    }

    /// <summary>두 번째 할당 없이 동일 확보 주소에서 사건/지연을 소비하고 현재 시각을 쓴다. 0.568에서 추가됐다.</summary>
    /// <param name="sid">붙일 부모 번호다.</param>
    /// <param name="event">예약할 사건 번호다.</param>
    /// <param name="payload">예약 지연과 함께 전달할 실수 인자다.</param>
    public void ConstructResourceRegular(int sid, uint @event, float payload)
    {
        RegularProcess reserved = _resourceRegular ?? throw new InvalidOperationException("장기 자원 Regular 의 실제 확보 메모리 누락");
        _resourceRegular = null;
        reserved.InitializeReserved(@event, payload);
        RegularProcess? attached = Processes().Host().AttachRegular(reserved, sid, false);
        // 원문 00496e80/00496f00 처럼 부착 뒤 현재 시각으로 다음 실행 시각을 둔다.
        if (attached is not null)
        {
            attached.ResetTask(@event, Processes().State().Now);
        }

        GC.KeepAlive(reserved);
    }

    /// <summary>같은 장기 Unpop 실행기로 현재 타입의 실제 공간 해제를 수행한다. 0.568에서 추가됐다.</summary>
    /// <param name="sid">해제할 객체 번호다.</param>
    /// <param name="flags">해제 플래그다.</param>
    public void UnpopAsset(int sid, uint flags)
    {
        byte number = Pool.Slot(sid)[RawSidPool.TypeOffset];
        // 타입 번호가 표를 벗어나면 원문처럼 진입 전에 거부한다.
        if (number >= Types.Count)
        {
            throw new ArgumentOutOfRangeException(nameof(sid), "장기 자산 해제의 현재 TYPE 범위 오류");
        }

        UnpopExecutor().Unpop(sid, Types[number], flags);
        NoteChanged();
    }

    /// <summary>현재 슬롯의 정확한 두 실수 좌표만 읽어 같은 공간에 다시 등록한다. 0.568에서 추가됐다.</summary>
    /// <param name="sid">다시 등록할 객체 번호다.</param>
    /// <param name="flags">등록 플래그다.</param>
    public void RepopAsset(int sid, uint flags)
    {
        ReadOnlySpan<byte> raw = Pool.Slot(sid);
        // 낮은 바이트부터 네 바이트를 조합해 원문 float 비트를 복원한다.
        uint bitsX = (uint)(raw[14] | (raw[15] << 8) | (raw[16] << 16) | (raw[17] << 24));
        uint bitsY = (uint)(raw[18] | (raw[19] << 8) | (raw[20] << 16) | (raw[21] << 24));
        float x = BitConverter.UInt32BitsToSingle(bitsX);
        float y = BitConverter.UInt32BitsToSingle(bitsY);
        PopAsset(sid, x, y, flags);
    }

    /// <summary>실제 공통 부모를 World 없이 같은 물리/현재 전역으로 구성한다. 0.568에서 추가됐다.</summary>
    public RawCarrierPostPop ResourceCarrierPostPop()
    {
        if (_resourceCarrier is null)
        {
            // 실제 Template 로딩 깊이 owner 없이 장기 후처리를 만들지 않는다.
            if (_serializationGlobals is null)
            {
                throw new InvalidOperationException("장기 자원 후처리의 실제 Template 로딩 owner 미결합");
            }

            var state = new CarrierPostPopState
            {
                SharedBoss = () => Authority().Boss,
                SharedLoadingDepth = () => LoadingDepth,
                SharedGeyserType = () => GameAux().GeyserType,
            };
            CarrierPostPopHooks hooks = RawCarrierPostPop.MakeCarrierPostPopHooks(
                Pool,
                PostPopExecutor(),
                sid => (_resourceDevices.CarrierCheck ?? throw new InvalidOperationException("장기 운반체의 현재 가상 확인 미연결"))(sid),
                (x, y, owner) => SurfaceExecutor().SetSupportOwner(x, y, owner));
            var carrier = new RawCarrierPostPop(Pool, Types, state, hooks);
            // PATCH 현재 TYPE 표를 빌려 이후 표 교체로 초기 코드를 바꾸지 않는다.
            if (_edition == OriginalEdition.Patch1078)
            {
                carrier.BindCurrentTypes(Types);
            }

            _resourceCarrierState = state;
            _resourceCarrier = carrier;
        }

        return _resourceCarrier;
    }

    /// <summary>장기 자산 전용 후처리 본문을 한 번만 구성하고 World 반납 뒤에도 유지한다. 0.568에서 추가됐다.</summary>
    public RawAssetPostPop ResourceAssetPostPop()
    {
        if (_resourceGeyser is null)
        {
            _resourceAssetState.SharedGeysers = SidLists.Geysers;
            _resourceAssetState.SharedAuthority = Authority();
            _resourceAssetState.SharedBridgeType = () => Links.BridgeType;
            _resourceAssetState.SharedIslandType = () => Links.IslandType;
            _resourceAssetState.SharedStalagType = () => Links.StalagType;
            var hooks = new AssetPostPopHooks
            {
                ReserveRegular = () => AllocateResourceRegular(),
                AddRegular = (parent, @event, payload) => ConstructResourceRegular(parent, @event, payload),
                GeyserRegistration = (parent, operation) =>
                    (_resourceDevices.GeyserRegistration ?? throw new InvalidOperationException("장기 간헐천의 현재 Player 등록 미연결"))(parent, operation),
                NotifySurface = parent => BuildingExecutor().NotifySurface(parent),
                Unpop = (parent, value) => UnpopAsset(parent, value),
                Repop = (parent, value) => RepopAsset(parent, value),
                VisitAtPosition = RawAssetPostPop.MakeAssetPostPopFinder(Pool, Hash, Types, native: true),
            };
            _resourceGeyser = new RawAssetPostPop(Pool, Types, PostPopExecutor(), ResourceCarrierPostPop(), _resourceAssetState, hooks);
        }

        return _resourceGeyser;
    }

    /// <summary>장기 너겟 전용 후처리 본문을 한 번만 구성하고 World 반납 뒤에도 유지한다. 0.568에서 추가됐다.</summary>
    public RawNuggetPostPop ResourceNuggetPostPop()
    {
        if (_resourceNugget is null)
        {
            var hooks = new NuggetPostPopHooks
            {
                HasRegular = (parent, @event) => Processes().Host().FindEvent(parent, @event) is not null,
                Allocate = () => AllocateResourceRegular(),
                Random = (low, high) =>
                {
                    GameRandom random = (_resourceDevices.Random ?? throw new InvalidOperationException("장기 너겟의 실제 게임 난수 미연결"))();
                    // 원문처럼 65536 다음 값의 상위 분수에 구간 너비를 곱해 낮은 값에 더한다.
                    float width = MathF.Abs(high - low);
                    double fraction = random.Next(65536) * (1.0 / 65536.0);
                    return low + (float)(fraction * width);
                },
                Construct = (parent, @event, payload) => ConstructResourceRegular(parent, @event, payload),
                Damageable = (parent, flags) => ResourceCarrierPostPop().DamageablePostPop(parent, flags),
            };
            _resourceNugget = new RawNuggetPostPop(Pool, hooks);
        }

        return _resourceNugget;
    }
}
