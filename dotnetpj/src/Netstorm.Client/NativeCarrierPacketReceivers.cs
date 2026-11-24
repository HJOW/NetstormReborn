using Netstorm.Core.Original;
using Netstorm.Net;

// 원문 Carrier·동맹·의식의 고정 F와 호출 시점의 실제 Runtime 선택을 분리한다.
// cpppj/src/client/NativeCarrierPacketReceivers.cpp 의 15행 설치·대여와
// cpppj/src/o/RawBombEffect.cpp·RawBombTimerPacket.cpp·RawMeltdown.cpp 의 송신 옵저버를 이식한다.
// 0.963에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client;

/// <summary>원문40b4b0의 고정 수신 코드와 같은 실제 Session 본문을 직접 연결하는 Carrier 수신기다.</summary>
public static class NativeCarrierPacketReceivers
{
    /// <summary>원문40b380 비NULL 빈 옵저버와 세 실제 가변 길이 옵저버를 구별한다.</summary>
    private enum CarrierObserverKind
    {
        /// <summary>40b380의 실제 비NULL ret4다.</summary>
        Empty,
        /// <summary>0x5f 주문 효과의 실제 송신 길이 확정이다.</summary>
        Effect,
        /// <summary>0x7d 상태 타이머의 실제 송신 길이 확정이다.</summary>
        Timer,
        /// <summary>0x75 Meltdown의 실제 송신 길이 확정이다.</summary>
        Meltdown,
    }

    /// <summary>보호10.78의 고정 F와 등록 정보 한 행이다.</summary>
    /// <param name="Type">원문 메시지 번호다.</param>
    /// <param name="Entry">불변 Native 수신 코드 주소다.</param>
    /// <param name="Length">보호 길이 WORD이며 ffff는 가변이다.</param>
    /// <param name="Observer">현재 mutable 옵저버와 비NULL 빈 옵저버의 구분이다.</param>
    private readonly record struct CarrierReceiverSpec(uint Type, uint Entry, ushort Length, CarrierObserverKind Observer);

    /// <summary>원문 등록 함수의 전체 쓰기와 PE0052ec40을 대조한 검토 15행이다.</summary>
    private static readonly CarrierReceiverSpec[] CarrierReceivers =
    [
        new(0x77, 0x0048f7f0u, 6, CarrierObserverKind.Empty), new(0x79, 0x0048f8f0u, 6, CarrierObserverKind.Empty),
        new(0x6f, 0x004900e0u, 6, CarrierObserverKind.Empty), new(0x40, 0x00428570u, 18, CarrierObserverKind.Empty),
        new(0x4f, 0x00426520u, 9, CarrierObserverKind.Empty), new(0x50, 0x00444bc0u, 9, CarrierObserverKind.Empty),
        new(0x60, 0x004267c0u, 10, CarrierObserverKind.Empty), new(0x61, 0x0048bfe0u, 11, CarrierObserverKind.Empty),
        new(0x5c, 0x0041e860u, 6, CarrierObserverKind.Empty), new(0x5f, 0x00420bf0u, 0xffff, CarrierObserverKind.Effect),
        new(0x7d, 0x0041e320u, 0xffff, CarrierObserverKind.Timer), new(0x6b, 0x00448e90u, 10, CarrierObserverKind.Empty),
        new(0x6d, 0x00493f80u, 8, CarrierObserverKind.Empty), new(0x75, 0x004205f0u, 0xffff, CarrierObserverKind.Meltdown),
        new(0x76, 0x004207a0u, 6, CarrierObserverKind.Empty),
    ];

    /// <summary>원문40aca0의 송신 옵저버다. 송신 비영일 때만 현재 count low WORD로 길이를 쓴다.</summary>
    /// <param name="packet">호출자 소유의 같은 현재 패킷이다.</param>
    /// <param name="sending">비영이면 송신 확정 방향이다.</param>
    /// <param name="invalidCount">count -1의 실제 복귀형 진단 장치다.</param>
    public static void ObserveBombEffectPacket(Memory<byte> packet, uint sending, Action? invalidCount)
    {
        // 수신 방향은 payload를 읽지 않으며 진단 뒤 현재 count를 다시 읽는다.
        if (sending == 0)
        {
            return;
        }

        Span<byte> bytes = packet.Span;
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (bytes.Length < 10)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "주문 효과 패킷의 실제10BYTE 범위");
        }

        if (System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(6, 4)) == 0xffffffffu)
        {
            (invalidCount ?? throw new InvalidOperationException("주문 효과 count의 실제 진단 장치 미연결"))();
        }

        uint length = (uint)(System.Buffers.Binary.BinaryPrimitives.ReadUInt16LittleEndian(bytes.Slice(6, 2)) + 5u) * 2u;
        System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), (ushort)length);
    }

    /// <summary>원문40b390의 송신 옵저버로 현재 수에 맞게 선언 길이만 확정한다.</summary>
    /// <param name="packet">호출자 소유의 같은 현재 패킷이다.</param>
    /// <param name="writing">참이면 송신 확정 방향이다.</param>
    public static void FinalizeBombTimerPacket(Memory<byte> packet, bool writing)
    {
        if (!writing)
        {
            return;
        }

        Span<byte> bytes = packet.Span;
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (bytes.Length < 18)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "상태 타이머 패킷의 실제18BYTE 범위");
        }

        uint count = System.Buffers.Binary.BinaryPrimitives.ReadUInt16LittleEndian(bytes.Slice(16, 2));
        if (count == 0xffffu)
        {
            throw new InvalidOperationException("원문 상태 타이머 대상 수 진단bda");
        }

        System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), (ushort)((count + 11u) * 2u));
    }

    /// <summary>원문40b150의 송신 옵저버로 쓰기 요청일 때만 현재 count로 WORD 길이를 갱신한다.</summary>
    /// <param name="packet">호출자 소유의 같은 현재 패킷이다.</param>
    /// <param name="writing">참이면 송신 확정 방향이다.</param>
    /// <param name="invalidCount">count -1의 실제 복귀형 진단 장치다.</param>
    public static void FinalizeMeltdownPacket(Memory<byte> packet, bool writing, Action? invalidCount)
    {
        if (!writing)
        {
            return;
        }

        Span<byte> bytes = packet.Span;
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (bytes.Length < 20)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "Meltdown 패킷의 실제20BYTE 범위");
        }

        if (System.Buffers.Binary.BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(16, 4)) == 0xffffffffu)
        {
            (invalidCount ?? throw new InvalidOperationException("Meltdown count의 실제 진단 장치 미연결"))();
        }

        uint length = (uint)(System.Buffers.Binary.BinaryPrimitives.ReadUInt16LittleEndian(bytes.Slice(16, 2)) + 10u) * 2u;
        System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), (ushort)length);
    }

    /// <summary>같은 장기 game/진단만 보유하며 등록 시점에는 현재 Session/Player/모드를 조회하지 않는다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="game">호출 시점의 실제 Session/Runtime을 선택하는 장기 전역이다.</param>
    /// <param name="storage">실제 진단·GameAux의 장기 소유자다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientGameplayGlobals game, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(storage);
        if (!ReferenceEquals(storage.GameAux(), game.GameAux))
        {
            throw new ArgumentException("Carrier 수신의 같은 실제 GameAux 필요", nameof(storage));
        }

        // 검토한 15행만 설치하고 나머지 현재 표는 보존한다.
        foreach (CarrierReceiverSpec spec in CarrierReceivers)
        {
            switch (spec.Observer)
            {
                case CarrierObserverKind.Effect:
                    dispatch.ObservePacket(spec.Type, (packet, sending) => ObserveBombEffectPacket(packet, sending, () => storage.AssertDisplay("count != (int)-1", "\\Ns\\O\\_p_packets.cpp", 0x96b)));
                    break;
                case CarrierObserverKind.Timer:
                    dispatch.ObservePacket(spec.Type, (packet, sending) => FinalizeBombTimerPacket(packet, sending != 0));
                    break;
                case CarrierObserverKind.Meltdown:
                    dispatch.ObservePacket(spec.Type, (packet, sending) => FinalizeMeltdownPacket(packet, sending != 0, () => storage.AssertDisplay("count != (int)-1", "\\Ns\\O\\_p_packets.cpp", 0xb23)));
                    break;
                default:
                    dispatch.Observe(spec.Type, static _ => { });
                    break;
            }

            dispatch.Version(spec.Type, 1, 0); dispatch.RequireLength(spec.Type, spec.Length);
            dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (sender, _, packet) =>
            {
                var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedCarrierPacket(spec.Entry, sender, packet);
            });
        }
    }

    /// <summary>원래 caller 위치에서 같은 불변 F와 현재 metadata를 확인한 뒤 lease만 다시 붙인다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="type">대여할 원문 메시지 번호다.</param>
    public static void Bind(RawPacketDispatch dispatch, uint type)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        // 검토한 한 행만 선택하며 미지 타입을 다른 함수에 넘기지 않는다.
        foreach (CarrierReceiverSpec spec in CarrierReceivers)
        {
            if (type != spec.Type)
            {
                continue;
            }

            Net.PacketRegistration row = dispatch.Registration(type); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(type);
            bool observerMatches = spec.Observer == CarrierObserverKind.Empty
                ? row.Observer is not null && row.PacketObserver is null
                : row.PacketObserver is not null && row.Observer is null;
            if (receiver is null || receiver.NativeEntry != spec.Entry || row.Version != 1 || row.Mismatch != 0 || row.Length != spec.Length || !observerMatches)
            {
                throw new InvalidOperationException("Native Carrier의 실제 고정F/metadata 불일치");
            }

            dispatch.RegisterCanonical(type, receiver); return;
        }

        throw new ArgumentException("미등록 Native Carrier 메시지", nameof(type));
    }
}
