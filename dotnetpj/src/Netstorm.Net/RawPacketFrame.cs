using Netstorm.Core.Original;
// 단일/집합의 서로 다른 현재 시각·버전·관찰·수신 순서와 폐기 검증 정책을 보존한다.
namespace Netstorm.Net;

/// <summary>cpppj RawPacketFrame.cpp의 OS 소켓/Player와 독립인 실제 프레임 부모다.</summary>
public static class RawPacketFrame
{
    /// <summary>서비스 종료로 은퇴한 CD 검증 요청/응답 쌍이다.</summary>
    private static bool Retired(byte type) => type is 0x7f or 0x80;
    /// <summary>도달한 현재 실제 backend 장치만 요구한다.</summary>
    private static T Need<T>(T? effect) where T : Delegate => effect ?? throw new InvalidOperationException("프레임 분배의 실제 장치 미연결");
    /// <summary>같은 mutable 버퍼에서 집합 순서와 저장 type·현재 F/관찰기 재조회를 실행한다.</summary>
    public static bool Dispatch(RawPacketDispatch dispatch, PacketConnection connection, Memory<byte> frame, PacketDispatchHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch); ArgumentNullException.ThrowIfNull(connection); ArgumentNullException.ThrowIfNull(hooks);
        if (frame.Length >= 4 && Retired(frame.Span[1])) return (connection.Flags & 0x40) == 0;
        if (frame.Length < 4 || frame.Span[1] != 4) return dispatch.Dispatch(connection, frame);
        if ((connection.Flags & 0x40) != 0) return false;
        int total = RawPacketDispatch.Length(frame.Span), cursor = 4, delay = -1;
        if (total > frame.Length) { Need(hooks.Terminate)(connection); return false; }
        // 앞선 효과가 뒤 등록/버퍼를 바꿀 수 있으므로 각 하위 헤더와 현재 표를 소비 시점에 읽는다.
        while (cursor < total)
        {
            if (total - cursor < 4) { Need(hooks.Terminate)(connection); return false; }
            ushort length = RawPacketDispatch.Length(frame.Span[cursor..]);
            if (length == 0 || length > total - cursor) { Need(hooks.Terminate)(connection); return false; }
            Memory<byte> packet = frame.Slice(cursor, length); cursor += length;
            if (!dispatch.Validate(packet.Span) || Retired(packet.Span[1])) continue;
            if (packet.Span[1] == 4) throw new InvalidOperationException("공통 중첩 집합 패킷 미복원");
            byte type = packet.Span[1]; uint now = Need(hooks.Now)();
            if (type == 6) connection.Heartbeat = now; else connection.Received = now;
            PacketRegistration registration = dispatch.Registration(type); int incoming = packet.Span[0] & 0x7f;
            if (incoming != registration.Version)
            {
                bool lower = incoming < registration.Version;
                if ((registration.Mismatch & (lower ? 8 : 4)) != 0) { Need(hooks.Terminate)(connection); return false; }
                if ((registration.Mismatch & (lower ? 2 : 1)) != 0) continue;
            }
            using IDisposable? recordPin = registration.Pin(); Need(hooks.Record)(connection, packet, length);
            PacketRegistration observer = dispatch.Registration(type).Snapshot(); observer.NotifyObserver(packet, 0);
            int selectedDelay = dispatch.DeliverReceiver(type, connection, packet, delay);
            if (selectedDelay >= 0) delay = selectedDelay;
        }
        if (cursor != total) { Need(hooks.Terminate)(connection); return false; }
        if (connection.Depth == 0 && (connection.Flags & 0x40) != 0) { Need(hooks.Close)(connection); Need(hooks.Release)(connection); return false; }
        return true;
    }
}
