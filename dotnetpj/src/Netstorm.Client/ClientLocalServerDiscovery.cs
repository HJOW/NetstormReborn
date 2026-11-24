// 0x66 로컬 서버 발견의 현재 블록 직접 쓰기와 경쟁 검색 signed 비교를 같은 분배기에 연결한다.
// cpppj/src/o/RawLocalServerDiscovery.cpp 의 ReceiveLocalServerDiscovery(004b8a10)를 이식한다.
// 0.949에서 R4-6 전송·수신 묶음용으로 추가됐다.
using System.Buffers.Binary;
using Netstorm.Core.Original;
using Netstorm.Net;

namespace Netstorm.Client;

// 004b8a10 로컬 서버 발견이 선택한 효과의 실제 외부 장치다.
public sealed class ClientLocalServerDiscoveryHooks
{
    /// <summary>같은 실제 로컬 상태 블록 owner다.</summary>
    public LocalMissionStateStorage? State { get; set; }

    /// <summary>같은 실제 검색 토큰 owner다.</summary>
    public UIntBox? Token { get; set; }

    /// <summary>같은 실제 검색 만료 기한 owner다.</summary>
    public DoubleBox? SearchEnd { get; set; }

    /// <summary>경쟁 검색의 지연 선택에서만 호출하는 현재 벽시각이다.</summary>
    public Func<double>? Wall { get; set; }

    /// <summary>현재 sender의 실제 주소 바이트다.</summary>
    public Func<uint, byte[]>? PeerAddress { get; set; }

    /// <summary>현재 36 작성 뒤에 호출하는 실제 부모다.</summary>
    public Action? ProcessState { get; set; }
}

// 004b8a10 전체이며 명령1이면 현재36/주소/+0x30포트를 쓰고 그외는 signed토큰이 작을 때만 검색을 12초 연장한다. 메뉴·세션을 요구하지 않는다.
public static class ClientLocalServerDiscovery
{
    /// <summary>원문 0x66 로컬 서버 발견 종류다. 0.949에서 추가됐다.</summary>
    private const uint Type = 0x66;

    /// <summary>원문 0x66 수신의 실제 고정 F 주소다. 0.949에서 추가됐다.</summary>
    private const uint NativeEntry = 0x004b8a10;

    /// <summary>원문 0x66 수신의 실제 고정 길이다. 0.949에서 추가됐다.</summary>
    private const ushort FixedLength = 12;

    /// <summary>경쟁 검색 연장 시 더하는 실제 12초다. 0.949에서 추가됐다.</summary>
    private const double ExtendSeconds = 12.0;

    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.949에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("로컬 서버 발견의 선택된 실제 장치 미연결");

    /// <summary>원문 비정렬 DWORD를 낮은 자리부터 조립한다. 0.949에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || 4 > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(bytes), "로컬 서버 발견의 실제 읽기 범위");
        }

        return BinaryPrimitives.ReadUInt32LittleEndian(bytes.Slice(offset, 4));
    }

    /// <summary>서버 발견의 현재 블록 직접 쓰기와 경쟁 검색의 signed 비교를 그대로 구별한다. 0.949에서 추가됐다.</summary>
    /// <param name="hooks">실제 상태·토큰·기한·벽시각·주소·부모 장치다.</param>
    /// <param name="sender">원문 송신자 번호다.</param>
    /// <param name="packet">원문 0x66 패킷이다.</param>
    public static void Receive(ClientLocalServerDiscoveryHooks hooks, uint sender, ReadOnlyMemory<byte> packet)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        LocalMissionStateStorage state = hooks.State ?? throw new InvalidOperationException("로컬 서버 발견의 실제 상태 미연결");
        UIntBox token = hooks.Token ?? throw new InvalidOperationException("로컬 서버 발견의 실제 토큰 미연결");
        if (Read(packet.Span, 4) == 1)
        {
            BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0, 4), 36);
            // 현재 36 쓰기 뒤에 현재 sender 주소를 조회한다.
            byte[] address = Need(hooks.PeerAddress)(sender);
            if (address.Length >= state.Current.Length - 8)
            {
                throw new ArgumentOutOfRangeException(nameof(packet), "로컬 서버 발견의 실제 주소 물리 초과");
            }

            address.CopyTo(state.Current.Span.Slice(8));
            state.Current.Span[8 + address.Length] = 0;
            BinaryPrimitives.WriteUInt32LittleEndian(state.Current.Span.Slice(0x30, 4), Read(packet.Span, 8));
            Need(hooks.ProcessState)();
            return;
        }

        if (unchecked((int)Read(packet.Span, 8)) < unchecked((int)token.Value))
        {
            DoubleBox searchEnd = hooks.SearchEnd ?? throw new InvalidOperationException("로컬 서버 발견의 실제 기한 미연결");
            searchEnd.Value = Need(hooks.Wall)() + ExtendSeconds;
        }
    }

    /// <summary>0x66의 원문 빈 observer·버전·고정 길이와 고정 F를 설치한다. 0.949에서 추가됐다.</summary>
    /// <param name="dispatch">같은 장기 분배기의 실제 공유 표다.</param>
    /// <param name="hooks">실제 상태·토큰·기한·벽시각·주소·부모 장치다.</param>
    public static void Install(RawPacketDispatch dispatch, ClientLocalServerDiscoveryHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(dispatch);
        ArgumentNullException.ThrowIfNull(hooks);
        // 40b380은 비NULL 빈 observer이며 원문 고정 길이는 12다.
        dispatch.Observe(Type, static _ => { });
        dispatch.Version(Type, 1, 0);
        dispatch.RequireLength(Type, FixedLength);
        dispatch.RegisterCanonicalDirect(Type, NativeEntry, (sender, _, packet) => Receive(hooks, sender, packet));
    }
}
