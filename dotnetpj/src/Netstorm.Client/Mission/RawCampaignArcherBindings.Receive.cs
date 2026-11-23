using System.Buffers.Binary;
using Netstorm.Core.Original;

// 같은 실제 전투 애니메이션 수신/송신과 현재 World/Host 선택을 직역한다.
namespace Netstorm.Client.Mission;

/// <summary>RawCampaignArcherBindings.cpp의 3b/3c 수신 공장과 공통 writer/송신 경계다.</summary>
public sealed partial class RawCampaignArcherBindings
{
    /// <summary>직전 실제 writer의 공통 버퍼 선택이며 다른 파생 fallback 버퍼와 구별한다.</summary>
    private bool _writingAnimation;
    /// <summary>원문 공통 outer+20을 가진 현재 다섯 파생의 실제 필드만 빌리며 복사하지 않는다.</summary>
    private static Span<byte> AnimationFields(SquidProcess? process)
    {
        if (process is GunAnimation gun) return gun.Fields.Bytes;
        if (process is CannonFire cannon) return cannon.Fields.Bytes;
        if (process is ArcherAnimation archer) return archer.Fields.Bytes;
        if (process is CrossbowAnimation crossbow) return crossbow.Fields.Bytes;
        if (process is IceCannonFire ice) return ice.Fields.Common.Bytes;
        return Span<byte>.Empty;
    }
    /// <summary>3b/3c의 실제 기본 생성 공장/같은 PID decoder/명시 Form 부착을 같은 World에 연결한다.</summary>
    public void ReceiveAnimationPacket(uint message, ReadOnlyMemory<byte> packet)
    {
        if (message != 0x3b && message != 0x3c) throw new ArgumentException("애니메이션 수신의 실제 메시지 범위", nameof(message));
        if (packet.Length < 9) throw new ArgumentOutOfRangeException(nameof(packet), "애니메이션 수신의 실제9BYTE 머리 부족");
        uint number = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[4..]);
        if (number == 0) throw new ArgumentOutOfRangeException(nameof(packet), "애니메이션 수신의 명시 Form0");
        if (number > ushort.MaxValue || number >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "애니메이션 수신의 DWORD SID 물리 범위");
        int form = (int)number; bool fire = message == 0x3b;
        string file = fire ? @"c:\ns\o\_p_fireanimprocessdata.cpp" : @"c:\ns\o\_p_rotateanimprocessdata.cpp";
        RawSurfaceStorage storage = _world.Storage(); RawGameAuxState aux = storage.GameAux();
        ReadOnlySpan<byte> raw = Pool.Slot(form);
        if ((raw[11] & 1) != 0)
        {
            if (packet.Span[8] != 0xff) storage.AssertDisplay("*(( char *)((char *)zacket+8) ) == (char)0xFFFFFFFF", file, 0x42);
            uint parentNumber = BinaryPrimitives.ReadUInt32LittleEndian(packet.Span[9..]);
            if (parentNumber > ushort.MaxValue || parentNumber >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "애니메이션 수신의 DWORD 부모 SID 물리 범위");
            byte type = Pool.Slot((int)parentNumber)[10];
            SquidProcess? pending = null; Span<byte> fields = Span<byte>.Empty;
            if (fire && type == aux.SunArcherTypeBox.Value)
            {
                var created = new ArcherAnimationFields(); RawGunAnimation.InitializeReceived(created.Bytes, GunAnimationReceiveKind.ArcherFire);
                pending = new ArcherAnimation(_animation, created, true); fields = created.Bytes;
            }
            else if (type == aux.SunCannonTypeBox.Value)
            {
                var created = new GunAnimationFields(); RawGunAnimation.InitializeReceived(created.Bytes, fire ? GunAnimationReceiveKind.SunFire : GunAnimationReceiveKind.SunRotate);
                pending = fire ? new CannonFire(_sunFire, created) : new GunAnimation(_cannon, created, GunAnimationKind.SunRotate); fields = created.Bytes;
            }
            else if (!fire && type == aux.WindArcherTypeBox.Value)
            {
                var created = new ArcherAnimationFields(); RawGunAnimation.InitializeReceived(created.Bytes, GunAnimationReceiveKind.ArcherRotate);
                pending = new ArcherAnimation(_crossbowRotate, created, false); fields = created.Bytes;
            }
            else storage.AssertDisplay("false", "\\Ns\\O\\Gunanim.cpp", fire ? 0x23bu : 0x248u);
            uint parent = RawGunAnimation.ReadProcess(fields, packet.Span);
            if (parent > ushort.MaxValue || parent >= Pool.Capacity) throw new ArgumentOutOfRangeException(nameof(packet), "애니메이션 수신의 DWORD 부모 SID 물리 범위");
            if (pending is null) throw new InvalidOperationException("애니메이션 수신의 미복원 파생 부착 실패");
            try
            {
                _ = _core.Processes.Attach(pending, fire ? aux.FireAnimationProcessType : aux.RotateAnimationProcessType, (int)parent, 0x10, form);
                pending = null;
            }
            finally { pending?.Dispose(); }
            return;
        }
        if ((packet.Span[8] & 1) != 0) storage.AssertDisplay("(*(char *)(( (char *)zacket+8) ) & 1) == 0", file, 0x4d);
        SquidProcess current = _core.Processes.FromForm(form) ?? throw new InvalidOperationException("애니메이션 수신의 현재 PID/outer 객체 미발견");
        _ = RawGunAnimation.ReadProcess(AnimationFields(current), packet.Span);
    }
    /// <summary>실제 파생들의 같은 공통 필드/버퍼와 일반·강제 writer/현재 packet/송신을 연결한다. 다른 프로세스는 기존 경계를 사용한다.</summary>
    public ProcessFormTransmission Transmission(Action<ReadOnlyMemory<byte>>? send, ProcessFormTransmission? fallback = null)
    {
        RawSurfaceStorage storage = _world.Storage(); RawGameAuxState aux = storage.GameAux(); byte[] buffer = aux.SquidTransmission.Buffer;
        int Write(Span<byte> fields, bool parentChanged, bool force)
        {
            byte[] header = RawGunAnimation.UsesFirePacket(fields) ? aux.FireAnimationHeader : aux.RotateAnimationHeader;
            return RawGunAnimation.WriteProcess(fields, buffer, parentChanged, force, header[0], header[1]);
        }
        return new()
        {
            WriteData = (process, changed) =>
            {
                Span<byte> fields = AnimationFields(process); _writingAnimation = !fields.IsEmpty;
                if (_writingAnimation)
                {
                    if (storage.UsesNativeTransmissionDevices()) return storage.SerializeProcessForm(process, buffer, changed, 0);
                    return Write(fields, changed, false);
                }
                return (fallback?.WriteData ?? throw new InvalidOperationException("다른 프로세스의 실제 일반 writer 미연결"))(process, changed);
            },
            WriteFullData = (process, force) =>
            {
                Span<byte> fields = AnimationFields(process); _writingAnimation = !fields.IsEmpty;
                if (_writingAnimation)
                {
                    if (storage.UsesNativeTransmissionDevices()) return storage.SerializeProcessForm(process, buffer, false, force);
                    return Write(fields, false, force != 0);
                }
                return (fallback?.WriteFullData ?? throw new InvalidOperationException("다른 프로세스의 실제 전체 writer 미연결"))(process, force);
            },
            Packet = length =>
            {
                if (!_writingAnimation) return (fallback?.Packet ?? throw new InvalidOperationException("다른 프로세스의 실제 작성 버퍼 미연결"))(length);
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "애니메이션의 실제 작성 버퍼 범위");
                return buffer.AsSpan(0, length).ToArray();
            },
            Transmit = length =>
            {
                if (!_writingAnimation) { (fallback?.Transmit ?? throw new InvalidOperationException("다른 프로세스의 실제 송신 미연결"))(length); return; }
                if (storage.UsesNativeTransmissionDevices()) { storage.BroadcastProcessForm(length); return; }
                if (send is null) throw new InvalidOperationException("애니메이션의 실제 송신 장치 미연결");
                if (length < 0 || length > buffer.Length) throw new ArgumentOutOfRangeException(nameof(length), "애니메이션의 실제 송신 길이 범위");
                send(buffer.AsMemory(0, length));
            },
        };
    }
}
