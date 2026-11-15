// 실제 장기 Client 전송 장치와 짧은 World writer 대여를 같은 버퍼/누적에 연결한다.
namespace Netstorm.Core.Original;

/// <summary>현재 Registry/Player를 조회하는 실제 Client 수명의 세 함수다.</summary>
public sealed class RawSurfaceTransmissionDevices
{
    /// <summary>선택된 실제 방송에 전체 가변2048BYTE 예약을 전달한다.</summary>
    public Action<byte[]>? Broadcast { get; set; }
    /// <summary>writer 뒤 현재 대상의 Player 연결 관문을 읽는다.</summary>
    public Func<int, bool>? Connected { get; set; }
    /// <summary>실제 현재 대상 송신에 같은 전체 예약을 전달한다.</summary>
    public Action<int, byte[]>? Send { get; set; }
    /// <summary>원문 함수 객체처럼 값 복사하며 장치를 실행하지 않는다.</summary>
    internal RawSurfaceTransmissionDevices Copy() => new() { Broadcast = Broadcast, Connected = Connected, Send = Send };
}

/// <summary>현재 World가 소유하고 Storage가 약하게 빌리는 실제 Pop/외부 Host 경계다.</summary>
public sealed class RawSurfacePopBindings
{
    /// <summary>공간/장부 뒤 선택하는 실제 Pop 송신이다.</summary>
    public Action<int, uint>? Transmit { get; set; }
    /// <summary>독립 typed 관찰에 writer 반환 크기만 전달한다.</summary>
    public Action<ReadOnlyMemory<byte>>? Broadcast { get; set; }
    /// <summary>독립 typed World의 현재 Player 관문이다.</summary>
    public Func<int, bool>? Connected { get; set; }
    /// <summary>독립 typed 대상에 writer 반환 크기만 전달한다.</summary>
    public Action<int, ReadOnlyMemory<byte>>? Send { get; set; }
    /// <summary>현재 외부 Host의 ProcessForm writer다.</summary>
    public SquidProcessPacketWriter? ProcessPacket { get; set; }
    /// <summary>실제 전체 Runtime owner를 선택 호출 동안 유지한다.</summary>
    public Func<object?>? Pin { get; set; }
}

/// <summary>같은 장기 물리/TYPE/공장·전송 상태를 한 writer에서 사용한다.</summary>
public sealed partial class RawSurfaceStorage
{
    /// <summary>실제 Client가 첫 실행기 전에 설치하는 함수 값 사본이다.</summary>
    private RawSurfaceTransmissionDevices _transmissionDevices = new();
    /// <summary>현재 World의 실제 대여이며 World를 장기 소유하지 않는다.</summary>
    private WeakReference<RawSurfacePopBindings>? _popRuntime;
    /// <summary>실제 공통 버퍼와 현재 TYPE/HP를 사용하는 유일한 writer다.</summary>
    private RawSquidTransmission? _transmissionExecutor;

    /// <summary>Source/자산/게임 모드 대신 실제 방송 장치가 명시 설치됐는지만 읽는다.</summary>
    public bool UsesNativeTransmissionDevices() => _transmissionDevices.Broadcast is not null;
    /// <summary>첫 공장/Host/World/writer 전에 완전한 실제 장치만 함수 값으로 복사한다.</summary>
    public void BindTransmissionDevices(RawSurfaceTransmissionDevices devices)
    {
        ArgumentNullException.ThrowIfNull(devices);
        if (_edition != OriginalEdition.Patch1078 || _borrowed || _factoryExecutor is not null || _processes is not null || _transmissionExecutor is not null
            || devices.Broadcast is null || devices.Connected is null || devices.Send is null)
            throw new ArgumentException("장기 Squid 전송의 실제 Client 장치 계약", nameof(devices));
        _transmissionDevices = devices.Copy();
    }
    /// <summary>같은 실제 Pop 대여만 허용하며 writer/전송/ACK를 실행하지 않는다.</summary>
    public void BindPopRuntime(RawSurfacePopBindings runtime)
    {
        ArgumentNullException.ThrowIfNull(runtime); var current = PopRuntime();
        if ((current is not null && !ReferenceEquals(current, runtime)) || runtime.Transmit is null) throw new ArgumentException("동일 실제 Pop writer 대여 필요", nameof(runtime));
        _popRuntime = new(runtime);
    }
    /// <summary>현재 자기 대여만 반납하며 다른 World의 대여는 변경하지 않는다.</summary>
    internal void ReleasePopRuntime(RawSurfacePopBindings runtime)
    {
        if (!ReferenceEquals(PopRuntime(), runtime)) throw new ArgumentException("Pop의 같은 현재 대여 반납 필요", nameof(runtime));
        _popRuntime = null;
    }
    /// <summary>그때 살아 있는 현재 대여를 선택한 호출 동안 강하게 빌린다.</summary>
    private RawSurfacePopBindings? PopRuntime() => _popRuntime is not null && _popRuntime.TryGetTarget(out var current) ? current : null;
    /// <summary>선택된 함수 누락을 기본 성공/다른 Runtime으로 바꾸지 않는다.</summary>
    private static T NeedTransmission<T>(T? callback) where T : Delegate => callback ?? throw new InvalidOperationException("장기 Squid 전송의 선택된 실제 장치 미연결");

    /// <summary>현재 TYPE/HP/진단과 실제 Client 또는 현재 typed 장치를 한 번 연결한다.</summary>
    public RawSquidTransmission TransmissionExecutor()
    {
        if (_transmissionExecutor is null)
        {
            var hooks = new RawSquidTransmissionHooks
            {
                Type = number => new SquidPopTypeReference(() => Types[number]), DefaultType = () => Links.BridgeType,
                MaxHp = number => FactoryExecutor().ReadSelectedHitPoints(Types[number], Types), Trace = TraceGraph, Assertion = AssertDisplay,
                Connected = ConnectedTransmission, ProcessPacket = WriteCurrentProcessPacket,
            };
            if (UsesNativeTransmissionDevices()) { hooks.BroadcastMutable = _transmissionDevices.Broadcast; hooks.SendMutable = _transmissionDevices.Send; }
            else { hooks.Broadcast = BroadcastTypedTransmission; hooks.Send = SendTypedTransmission; }
            _transmissionExecutor = new(Pool, GameAux().SquidTransmission, hooks);
        }
        return _transmissionExecutor;
    }
    /// <summary>실제 Native 관문이 있으면 바로 사용하고 없을 때만 현재 typed 대여를 pin한다.</summary>
    private bool ConnectedTransmission(int target)
    {
        if (_transmissionDevices.Connected is { } native) return native(target);
        var current = PopRuntime(); if (current?.Connected is null) throw new InvalidOperationException("독립 Squid의 현재 Player 장치 미연결");
        object? owner = current.Pin?.Invoke();
        try { return NeedTransmission(current.Connected)(target); } finally { GC.KeepAlive(owner); }
    }
    /// <summary>writer 반환 크기를 유지하고 선택된 실제 typed 방송만 pin한다.</summary>
    private void BroadcastTypedTransmission(ReadOnlyMemory<byte> packet)
    {
        var current = PopRuntime(); if (current?.Broadcast is null) throw new InvalidOperationException("독립 Squid의 실제 방송 관측 미연결");
        object? owner = current.Pin?.Invoke();
        try { NeedTransmission(current.Broadcast)(packet); } finally { GC.KeepAlive(owner); }
    }
    /// <summary>선택된 실제 typed 대상에 같은 반환 크기를 전달한다.</summary>
    private void SendTypedTransmission(int target, ReadOnlyMemory<byte> packet)
    {
        var current = PopRuntime(); if (current?.Send is null) throw new InvalidOperationException("독립 Squid의 실제 대상 송신 미연결");
        object? owner = current.Pin?.Invoke();
        try { NeedTransmission(current.Send)(target, packet); } finally { GC.KeepAlive(owner); }
    }
    /// <summary>선택된 외부 Host writer만 우선하고 없으면 같은 장기 Host의 실제 PID를 조회한다.</summary>
    private int WriteCurrentProcessPacket(int form, Span<byte> packet, uint argument)
    {
        var current = PopRuntime();
        if (current?.ProcessPacket is null) return Processes().Host().WriteFormPacket(form, argument, packet, GameAux().SquidTransmission);
        object? owner = current.Pin?.Invoke();
        try { return NeedTransmission(current.ProcessPacket)(form, packet, argument); } finally { GC.KeepAlive(owner); }
    }
    /// <summary>공간/장부 뒤 현재 실제 Pop writer가 있으면 선택하고 없으면 같은 장기 writer를 사용한다.</summary>
    public void TransmitPop(int sid, uint flags)
    {
        var current = PopRuntime();
        if (current?.Transmit is null) { TransmissionExecutor().Pop(sid, flags); return; }
        object? owner = current.Pin?.Invoke();
        try { NeedTransmission(current.Transmit)(sid, flags); } finally { GC.KeepAlive(owner); }
    }
    /// <summary>signed 반환 범위를 확인하고 양수일 때만 실제 Native 방송에 전체 예약을 전달한다.</summary>
    public void BroadcastProcessForm(int length)
    {
        byte[] buffer = GameAux().SquidTransmission.Buffer;
        if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "현재 ProcessForm 공통 송신 예약 범위");
        var send = _transmissionDevices.Broadcast ?? throw new InvalidOperationException("현재 ProcessForm의 실제 Native 방송 장치 미연결");
        if (length > 0) send(buffer);
    }
}
