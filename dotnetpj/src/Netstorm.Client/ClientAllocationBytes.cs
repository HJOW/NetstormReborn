using System.Runtime.InteropServices;

// 원문생성자가쓰지않는바이트는같은현재호스트할당입력을사용한다.
namespace Netstorm.Client;

/// <summary>cpppj CurrentAllocationBytes의실제힙입력경계다. 고정검사는명시공급자로대체한다.</summary>
public static class ClientAllocationBytes
{
    /// <summary>새할당을0으로채우지않고현재바이트만복사한뒤즉시반납한다. 목적버퍼의폭을늘리거나줄이지않는다.</summary>
    public static void ReadCurrent(Span<byte> destination)
    {
        if (destination.IsEmpty) return;
        nint allocation = Marshal.AllocHGlobal(destination.Length);
        try { byte[] bytes = new byte[destination.Length]; Marshal.Copy(allocation, bytes, 0, bytes.Length); bytes.CopyTo(destination); }
        finally { Marshal.FreeHGlobal(allocation); }
    }
}
