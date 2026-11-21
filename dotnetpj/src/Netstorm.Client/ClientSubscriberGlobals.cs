using Netstorm.Core.Original;

// Client 수명의 두 Subscriber DWORD와 현재 handle을 World/Session/Chat과 공유한다.
namespace Netstorm.Client;

/// <summary>원문 ClientSubscriberGlobals.h의 동일 주소 소유자다. 생성 시에만 PE/BSS 초기값을 만든다.</summary>
public sealed class ClientSubscriberGlobals
{
    /// <summary>00540c74의 현재 DWORD이며 PE 최초-1이다.</summary>
    public UIntBox MySubscriber { get; } = new() { Value = uint.MaxValue };
    /// <summary>0054265c의 현재 DWORD이며 PE 최초1이다.</summary>
    public UIntBox LoginSubscriber { get; } = new() { Value = 1 };
    /// <summary>현재 NUL 이름의 고정64BYTE이며 Session 반납으로 지우지 않는다.</summary>
    public byte[] Handle { get; } = new byte[64];
}
