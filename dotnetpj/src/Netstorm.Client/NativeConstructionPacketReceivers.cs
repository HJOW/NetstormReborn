using Netstorm.Core.Original;
using Netstorm.Net;

// 원문 건설/연구의 고정 F와 현재 Session 선택을 분리하여 지연큐에 이전 Session을 저장하지 않는다.
// cpppj/src/client/NativeConstructionPacketReceivers.cpp 의 8행 설치·대여와
// cpppj/src/o/RawConstructionPacket.cpp 의 송신 옵저버를 이식한다.
// 0.963에서 R4-6 전송·수신 묶음용으로 추가됐다.
namespace Netstorm.Client;

/// <summary>원문40b4b0의 고정 수신 코드와 같은 실제 Session 본문을 직접 연결하는 건설/연구 수신기다.</summary>
public static class NativeConstructionPacketReceivers
{
    /// <summary>NULL·40b380·40a800의 실제 서로 다른 옵저버 슬롯이다.</summary>
    private enum ConstructionObserverKind
    {
        /// <summary>NULL 옵저버이며 송신 확정도 수행하지 않는다.</summary>
        None,
        /// <summary>40b380의 실제 비NULL 빈 ret4다.</summary>
        Empty,
        /// <summary>0x3f 건설 패킷의 실제 송신 길이 확정이다.</summary>
        Construction,
    }

    /// <summary>실제 원문 수신/metadata 한 행이다.</summary>
    /// <param name="Type">원문 메시지 번호다.</param>
    /// <param name="Entry">고정 Native F의 코드 식별자다.</param>
    /// <param name="Length">원문 고정 WORD 또는 가변 ffff다.</param>
    /// <param name="Observer">해당 행의 NULL/빈/건설 최종화 옵저버다.</param>
    private readonly record struct ConstructionReceiverSpec(uint Type, uint Entry, ushort Length, ConstructionObserverKind Observer);

    /// <summary>보호10.78의 실제 등록 함수/길이 표와 대조한 건설계 여덟 행이다.</summary>
    private static readonly ConstructionReceiverSpec[] ConstructionReceivers =
    [
        new(0x3e, 0x00444af0u, 25, ConstructionObserverKind.Empty), new(0x3f, 0x004441b0u, 0xffff, ConstructionObserverKind.Construction),
        new(0x37, 0x004427b0u, 0xffff, ConstructionObserverKind.None), new(0x58, 0x00453d70u, 16, ConstructionObserverKind.Empty),
        new(0x54, 0x004436f0u, 8, ConstructionObserverKind.Empty), new(0x51, 0x004dc050u, 8, ConstructionObserverKind.Empty),
        new(0x2f, 0x004332c0u, 15, ConstructionObserverKind.Empty), new(0x6c, 0x004d56f0u, 11, ConstructionObserverKind.Empty),
    ];

    /// <summary>원문40a800이다. sending이 비영이면 현재 sidCount로 선언 길이를 갱신한다.</summary>
    /// <param name="packet">호출자 소유의 같은 현재 패킷이다.</param>
    /// <param name="sending">참이면 송신 확정 방향이다.</param>
    /// <param name="invalidCount">sidCount 255의 실제 복귀형 진단 장치다.</param>
    public static void FinalizeConstructionPacket(Memory<byte> packet, bool sending, Action? invalidCount)
    {
        // 원문은 sending 0일 때 개수 진단과 길이 쓰기를 실행하지 않는다.
        if (!sending)
        {
            return;
        }

        Span<byte> bytes = packet.Span;
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (bytes.Length < 0x1b)
        {
            throw new ArgumentOutOfRangeException(nameof(packet), "건설 패킷 길이 확정의 실제27BYTE 범위");
        }

        if (bytes[26] == 0xff)
        {
            (invalidCount ?? throw new InvalidOperationException("건설 패킷 sidCount의 실제 진단 장치 미연결"))();
        }

        System.Buffers.Binary.BinaryPrimitives.WriteUInt16LittleEndian(bytes.Slice(2, 2), (ushort)((uint)bytes[26] * 2u + 0x1bu));
    }

    /// <summary>초기 등록은 같은 장기 전역만 캡처하고 현재 Session/Player/모드를 읽지 않는다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="game">호출 시점의 실제 Session/Runtime을 선택하는 장기 전역이다.</param>
    /// <param name="storage">실제 진단·GameAux의 장기 소유자다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientGameplayGlobals game, RawSurfaceStorage storage)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(game); ArgumentNullException.ThrowIfNull(storage);
        if (!ReferenceEquals(storage.GameAux(), game.GameAux))
        {
            throw new ArgumentException("건설/연구 수신의 같은 실제 GameAux 필요", nameof(storage));
        }

        // 여덟 원문 행만 설치하고 다른 현재 receiver/옵저버에는 손대지 않는다.
        foreach (ConstructionReceiverSpec spec in ConstructionReceivers)
        {
            if (spec.Observer == ConstructionObserverKind.Construction)
            {
                dispatch.ObservePacket(spec.Type, (packet, sending) => FinalizeConstructionPacket(packet, sending != 0, () => storage.AssertDisplay("sidCount != (unsigned char)-1", "\\Ns\\O\\_p_packets.cpp", 0x7c9)));
            }
            else if (spec.Observer == ConstructionObserverKind.None)
            {
                dispatch.Observe(spec.Type, null);
            }
            else
            {
                dispatch.Observe(spec.Type, static _ => { });
            }

            dispatch.Version(spec.Type, 1, 0); dispatch.RequireLength(spec.Type, spec.Length);
            dispatch.RegisterCanonicalDirect(spec.Type, spec.Entry, (sender, _, packet) =>
            {
                var session = game.RequireSession(); using var runtime = game.PinCurrentRuntime(session); session.ReceiveFixedConstructionPacket(spec.Entry, sender, packet);
            });
        }
    }

    /// <summary>metadata/코드 정체성을 검증하고 같은 F값만 이번 등록 lease에 다시 붙인다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="expected">대여할 검토 행이다.</param>
    private static void BindReceiver(RawPacketDispatch dispatch, ConstructionReceiverSpec expected)
    {
        Net.PacketRegistration row = dispatch.Registration(expected.Type); Net.CanonicalPacketReceiver? receiver = dispatch.SnapshotReceiver(expected.Type);
        bool observerMatches = expected.Observer == ConstructionObserverKind.None
            ? row.Observer is null && row.PacketObserver is null
            : expected.Observer == ConstructionObserverKind.Construction
                ? row.PacketObserver is not null && row.Observer is null
                : row.Observer is not null && row.PacketObserver is null;
        if (receiver is null || receiver.NativeEntry != expected.Entry || row.Version != 1 || row.Mismatch != 0 || row.Length != expected.Length || !observerMatches)
        {
            throw new InvalidOperationException("Native 건설/연구의 실제 고정F/metadata 불일치");
        }

        dispatch.RegisterCanonical(expected.Type, receiver);
    }

    /// <summary>3e→3f의 원래 등록 순서를 유지한다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    public static void BindCommandReceivers(RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        BindReceiver(dispatch, ConstructionReceivers[0]); BindReceiver(dispatch, ConstructionReceivers[1]);
    }

    /// <summary>buildingData 확인 뒤에만 37의 같은 F를 대여하는 기존 caller 순서를 유지한다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    public static void BindDataReceiver(RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        BindReceiver(dispatch, ConstructionReceivers[2]);
    }

    /// <summary>연구의 기존 현재 본문/metadata와 같은 F값을 대여한다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    public static void BindFactoryResearchReceiver(RawPacketDispatch dispatch)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        BindReceiver(dispatch, ConstructionReceivers[3]);
    }

    /// <summary>각 caller의 기존 등록 순서에서만 해당 한 행을 대여하고 다른 행의 쓰기를 앞당기지 않는다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="type">대여할 원문 메시지 번호다.</param>
    public static void BindReceiver(RawPacketDispatch dispatch, uint type)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        // 검토한 같은 행만 선택하며 미지 타입을 다른 본문으로 보내지 않는다.
        foreach (ConstructionReceiverSpec spec in ConstructionReceivers)
        {
            if (spec.Type == type)
            {
                BindReceiver(dispatch, spec); return;
            }
        }

        throw new ArgumentException("미등록 Native 건설 수신 메시지", nameof(type));
    }
}
