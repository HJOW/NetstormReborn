// NSInfo 루트의 원문 입력을 같은 정렬표/CS와 현재 메뉴·카메라·송신 장치에 연결한다.
// cpppj/src/client/NativeNsInfoInput.cpp 의 HandleNativeNsInfo(0047c8c0) 를 이식한다.
// 0.911에서 RU-3 NSInfo HUD 묶음용으로 추가됐다.
using Netstorm.Client.Display;

namespace Netstorm.Client.Gump;

// NSInfo 루트 입력이 선택한 효과의 실제 외부 장치다.
public sealed class NativeNsInfoInputHooks
{
    /// <summary>현재 명령 DWORD를 읽는다.</summary>
    public Func<uint, uint>? Command { get; set; }

    /// <summary>5930b0의 같은 정렬표에서 실제 참가자 번호를 읽는다.</summary>
    public Func<uint, uint>? Roster { get; set; }

    /// <summary>현재 실제 CS의 참가자 물리를 빌린다.</summary>
    public Func<uint, Memory<byte>>? Participant { get; set; }

    /// <summary>5cae94/98의 원시 화면 좌표를 소비 시점에 읽는다.</summary>
    public Func<ScreenPoint>? Mouse { get; set; }

    /// <summary>4785f0 전체 현재 참가자 메뉴 공장이다.</summary>
    public Action<uint, ScreenPoint>? Menu { get; set; }

    /// <summary>현재 참가자 SID를 실제 433ba0/즉시 카메라에 전달한다.</summary>
    public Action<uint>? Center { get; set; }

    /// <summary>실제 1fh/12BYTE 헤더 뒤 부분효과를 수행하고 11로 보낸다.</summary>
    public Action<Action<Memory<byte>>>? Status { get; set; }

    /// <summary>chalKickedoff.wav의 원문 우선순위1 재생이다.</summary>
    public Action? KickoffSound { get; set; }

    /// <summary>처리하지 않은 원래 세 DWORD를 같은 BaseHelp에 전달한다.</summary>
    public Func<uint, uint, uint, uint>? Base { get; set; }
}

// 0047c8c0 전체: 정렬 메뉴/중심·직접 참가자 추방·원래 입력의 기본 처리다.
public static class NativeNsInfoInput
{
    /// <summary>선택된 실제 장치만 요구하며 가짜 성공으로 대신하지 않는다. 0.911에서 추가됐다.</summary>
    private static T Need<T>(T? value) where T : Delegate => value ?? throw new InvalidOperationException("NSInfo 루트 입력의 실제 장치 미연결");

    /// <summary>원문 DWORD 폭만 읽고 밖 접근은 그 위치에서 거부한다. 0.911에서 추가됐다.</summary>
    private static uint Read(ReadOnlySpan<byte> bytes, int offset)
    {
        // 낮은 BYTE부터 원문 폭만 조립한다.
        if (offset < 0 || offset > bytes.Length || 4 > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "NSInfo 루트 입력의 실제 CS DWORD 부족");
        }

        return (uint)bytes[offset]
            | ((uint)bytes[offset + 1] << 8)
            | ((uint)bytes[offset + 2] << 16)
            | ((uint)bytes[offset + 3] << 24);
    }

    /// <summary>원문의 DWORD 저장을 같은 네 BYTE에만 기록한다. 0.911에서 추가됐다.</summary>
    private static void Put(Memory<byte> bytes, int offset, uint value)
    {
        // 원문 폭 밖 접근은 그 위치에서 거부한다.
        if (offset < 0 || offset > bytes.Length || 4 > bytes.Length - offset)
        {
            throw new ArgumentOutOfRangeException(nameof(offset), "NSInfo 루트 입력의 실제 패킷 DWORD 부족");
        }

        Span<byte> span = bytes.Span;
        for (int index = 0; index < 4; index++)
        {
            span[offset + index] = (byte)(value >> (8 * index));
        }
    }

    /// <summary>DWORD 대신 원문의 test BYTE 한 개만 읽는다. 0.911에서 추가됐다.</summary>
    private static bool Active(Memory<byte> record)
    {
        // 원문 flag BYTE 밖 접근은 그 위치에서 거부한다.
        if (record.Length <= 0x64)
        {
            throw new ArgumentOutOfRangeException(nameof(record), "NSInfo 루트 입력의 실제 CS flag BYTE 부족");
        }

        return (record.Span[0x64] & 1u) != 0;
    }

    /// <summary>비교만 26비트로 마스킹하며 기본 처리에는 수정하지 않은 event/first/second를 전달한다. 0.911에서 추가됐다.</summary>
    /// <param name="event">원문 입력 event DWORD다.</param>
    /// <param name="first">원문 입력 첫 DWORD다.</param>
    /// <param name="second">원문 입력 둘째 DWORD다.</param>
    /// <param name="hooks">실제 CS/메뉴/카메라/송신 장치다.</param>
    public static uint Handle(uint @event, uint first, uint second, NativeNsInfoInputHooks hooks)
    {
        ArgumentNullException.ThrowIfNull(hooks);
        uint code = @event & 0x03ffffffu;
        if (code == Need(hooks.Command)(0x542790) && unchecked((int)second) > 0)
        {
            uint index = Need(hooks.Roster)(second);
            Memory<byte> record = Need(hooks.Participant)(index);
            if (Active(record))
            {
                ScreenPoint point = Need(hooks.Mouse)();
                Need(hooks.Menu)(index, point);
                return 1;
            }
        }

        if (code == Need(hooks.Command)(0x54278c) && unchecked((int)second) > 0)
        {
            uint index = Need(hooks.Roster)(second);
            Memory<byte> record = Need(hooks.Participant)(index);
            if (Active(record))
            {
                Need(hooks.Center)(Read(record.Span, 0xb8));
                return 1;
            }
        }

        if (code == Need(hooks.Command)(0x5429dc) && unchecked((int)second) > 0)
        {
            Memory<byte> record = Need(hooks.Participant)(second);
            if (Active(record))
            {
                Need(hooks.Status)(packet =>
                {
                    Put(packet, 4, Read(record.Span, 4));
                    Put(packet, 8, 8);
                    Need(hooks.KickoffSound)();
                });
                return 1;
            }
        }

        return Need(hooks.Base)(@event, first, second);
    }
}
