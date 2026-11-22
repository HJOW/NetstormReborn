using System.Buffers.Binary;

// 장기 Player 정기는 현재 두 owner와 Host를 사용하고 Template 후단만 약하게 빌린다.
namespace Netstorm.Core.Original;

/// <summary>cpppj/src/client/RawSurfaceWorld.h의 실제 State getter·Notify·전체 Runtime 대여다.</summary>
public sealed class RawSurfacePlayerBindings
{
    /// <summary>같은 현재 Template manager의 State 메모리를 빌린다.</summary>
    public Func<uint, Memory<byte>>? State { get; set; }
    /// <summary>State 쓰기 뒤 다시 읽은 현재 owner의 변경을 통지한다.</summary>
    public Action<uint>? Changed { get; set; }
    /// <summary>반환 State 쓰기와 Notify가 끝날 때까지 실제 전체 owner를 보유한다.</summary>
    public Func<IDisposable?>? Pin { get; set; }
}

/// <summary>cpppj/src/client/RawSurfacePlayerRegular.cpp의 장기 본체와 실제 할당/부착 경계다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>게임값을 복사하지 않는 같은 Client Player 상태 owner다.</summary>
    private PlayerWorldState? _playerRegularState;
    /// <summary>현재 Session이 소유하는 짧은 Template 후단의 약한 대여다.</summary>
    private WeakReference<RawSurfacePlayerBindings>? _playerRegularRuntime;
    /// <summary>할당 성공 뒤 placement 초기화할 같은 실제 Regular 객체다.</summary>
    private RegularProcess? _playerRegularReservation;
    /// <summary>현재 두 owner 주소가 명시 설치됐는지만 조회한다.</summary>
    public bool UsesNativePlayerRegularState() => _playerRegularState is not null;
    /// <summary>첫 World 전에 같은 현재 Player 주소만 연결하며 owner/known/모드 값을 쓰지 않는다.</summary>
    public void BindPlayerRegularState(PlayerWorldState state)
    {
        ArgumentNullException.ThrowIfNull(state);
        if (!UsesNativeTransmissionDevices() || _borrowed || _playerRegularState is not null) throw new ArgumentException("Player 정기의 실제 최초 현재 전역 계약", nameof(state));
        _playerRegularState = state;
    }
    /// <summary>살아 있는 다른 Session을 덮지 않고 현재 Template 후단만 빌린다.</summary>
    public void BindPlayerRegularRuntime(RawSurfacePlayerBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime);
        if (!UsesNativeTransmissionDevices() || (_playerRegularRuntime is not null && _playerRegularRuntime.TryGetTarget(out _))) throw new ArgumentException("Player 정기의 실제 Template 대여 중복/누락", nameof(runtime));
        _playerRegularRuntime = new(runtime);
    }
    /// <summary>현재 자기 후단만 결정적으로 반납하고 다른 Session 대여는 지우지 않는다.</summary>
    public void ReleasePlayerRegularRuntime(RawSurfacePlayerBindings runtime)
    {
        if (!ReferenceEquals(CurrentPlayerRegularRuntime(), runtime)) throw new ArgumentException("Player 정기의 같은 Template 대여 반납 필요", nameof(runtime));
        _playerRegularRuntime = null;
    }
    /// <summary>도달한 Template 소비 위치에서 실제 살아 있는 현재 후단만 요구한다.</summary>
    private RawSurfacePlayerBindings CurrentPlayerRegularRuntime() => _playerRegularRuntime is not null && _playerRegularRuntime.TryGetTarget(out var current)
        ? current : throw new InvalidOperationException("Player 정기의 실제 현재 Template Runtime 미연결");
    /// <summary>선택된 함수 누락을 원문 성공으로 대체하지 않는다.</summary>
    private static T NeedPlayerRegular<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("Player 정기의 실제 Template 하위 미연결");
    /// <summary>원문 new28의 실제 확보 실패를 보존하고 중첩 확보를 거부한다.</summary>
    public bool ReservePlayerRegular()
    {
        if (!UsesNativeTransmissionDevices()) throw new InvalidOperationException("Player 예약의 Native 장치 미연결");
        if (_playerRegularReservation is not null) throw new InvalidOperationException("Player 실제 Regular 확보 중첩");
        try { _playerRegularReservation = new(0, 0); return true; } catch (OutOfMemoryException) { return false; }
    }
    /// <summary>확보한 같은 객체에서 사건/payload를 초기화하고 실제 Host의 현재 시각으로 부착한다.</summary>
    public void ConstructPlayerRegular(int sid, uint @event, float payload)
    {
        RegularProcess process = _playerRegularReservation ?? throw new InvalidOperationException("Player 실제 Regular 생성 전 확보 누락");
        _playerRegularReservation = null; process.InitializeReserved(@event, payload); Processes().Host().AttachRegular(process, sid, false);
    }
    /// <summary>실제 VT와 현재 두 owner/모드·로그/진단·예약을 원문 Player 본체에 전달한다.</summary>
    public float PlayerRegular(int sid, uint @event, uint count, float payload)
    {
        if (!UsesNativeTransmissionDevices() || _playerRegularState is null) throw new InvalidOperationException("Player 정기의 실제 현재 전역 미연결");
        if (BinaryPrimitives.ReadUInt32LittleEndian(Pool.Slot(sid)) != 0x50ee08) throw new InvalidOperationException("다른 Player 정기 가상 표");
        return RawPlayerRegular.Run(Pool, _playerRegularState, OwnerMode, sid, @event, count, payload, new()
        {
            Assertion = _ => AssertDisplay("0", @"\Ns\O\Player.cpp", 0x41d),
            Log = (group, text) => (_graphTrace ?? throw new InvalidOperationException("Player 정기의 현재 실제 로그 미연결"))(unchecked((int)group), text),
            ReserveRegular = ReservePlayerRegular, AddRegular = ConstructPlayerRegular,
            TemplatePin = () => NeedPlayerRegular(CurrentPlayerRegularRuntime().Pin)() ?? throw new InvalidOperationException("Player Template의 실제 전체 Runtime owner 반납"),
            TemplateState = owner => NeedPlayerRegular(CurrentPlayerRegularRuntime().State)(owner),
            TemplateChanged = owner => NeedPlayerRegular(CurrentPlayerRegularRuntime().Changed)(owner),
        });
    }
}
