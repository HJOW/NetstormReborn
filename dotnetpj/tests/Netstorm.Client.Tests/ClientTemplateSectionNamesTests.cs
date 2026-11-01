using Netstorm.Client.Host;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests;

/// <summary>
/// Client 수명의 lazy 섹션 이름표 owner(<see cref="ClientTemplateSectionNames"/>)를 알려진 할당 바이트로 검사한다.
/// cpppj <c>DataManagerInitializeTests</c>의 <c>ClientTemplateSectionNames_LazyMutableAndUnknownTail</c>·<c>ClientTemplateSectionNames_AllocationZeroIsFailure</c>를 옮겼다(2026-11-01 0.129).
/// 이름표 내용 자체는 원본 x86 fixture(<c>template-section-names</c>)로 Core 검사가 대조하며, 이 owner의 lazy 수명은 직역 검사라 L1이 아니다.
/// </summary>
public sealed class ClientTemplateSectionNamesTests
{
    /// <summary>알려진 0xa5로 채운 512바이트를 만들어 최초 선택 때만 쓰이는 할당 장치를 구성한다.</summary>
    /// <param name="onCall">할당이 호출될 때마다 실행할 관측 동작.</param>
    private static TemplateSectionNamesAllocator Known(Action onCall) => size =>
    {
        onCall();
        Assert.Equal(512, size);
        byte[] bytes = new byte[size];
        Array.Fill(bytes, (byte)0xa5);
        return bytes;
    };

    /// <summary>최초 선택 전에는 할당하지 않고, 최초 선택이 277바이트만 쓰며 같은 표를 재사용하고, 읽기 변환이 제자리 상태를 남기는지 확인한다.</summary>
    [Fact]
    public void NamesAreLazyMutableAndTailIsUnknown()
    {
        int calls = 0;
        ClientTemplateSectionNames names = new(Known(() => calls++));
        Assert.Equal(0u, names.CurrentToken);
        Assert.Equal(0, calls);
        Span<byte> bytes = names.Names();
        Assert.Equal(1, calls);
        Assert.Equal(1u, names.CurrentToken);
        // 원문이 쓰지 않은 235바이트는 읽거나 쓰지 않는다
        Assert.All(bytes[277..].ToArray(), value => Assert.Equal(0xa5, value));
        Assert.Equal((byte)'/', bytes[0]);
        Assert.Equal((byte)'/', bytes[275]);
        Assert.Equal(0, bytes[276]);
        Assert.Equal("Subscriber", RawSnart.ReadName(bytes, 0, null));
        Assert.Equal("Terr19", RawSnart.ReadName(bytes, 34, null));
        Assert.Equal("", RawSnart.ReadName(bytes, 35, null));
        // SNART 변환은 같은 표를 제자리에서 바꾼다
        Assert.Equal(0, bytes[0]);
        Assert.Equal(0xff, bytes[276]);
        Span<byte> again = names.Names();
        Assert.Equal(1, calls);
        Assert.Equal(0, again[0]);
        Assert.True(System.Runtime.CompilerServices.Unsafe.AreSame(ref System.Runtime.InteropServices.MemoryMarshal.GetReference(bytes), ref System.Runtime.InteropServices.MemoryMarshal.GetReference(again)));
        Assert.All(again[277..].ToArray(), value => Assert.Equal(0xa5, value));
    }

    /// <summary>실제 할당이 NULL이면 물리 실패로 남고 현재 token은 0으로 보존한다.</summary>
    [Fact]
    public void AllocationZeroIsFailure()
    {
        int calls = 0;
        ClientTemplateSectionNames names = new(size =>
        {
            calls++;
            Assert.Equal(512, size);
            return null;
        });
        Assert.Throws<ArgumentOutOfRangeException>(() => names.Names());
        Assert.Equal(0u, names.CurrentToken);
        Assert.Equal(1, calls);
    }
}
