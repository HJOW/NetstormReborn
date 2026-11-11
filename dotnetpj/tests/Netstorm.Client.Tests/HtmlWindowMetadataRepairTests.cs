using System.Text;
using Netstorm.Client.Gump;
using Netstorm.Client.Scripting;

namespace Netstorm.Client.Tests;

/// <summary>
/// % 삽입 목록의 NB08 교정(끄면 원문 동작, 켜면 본문 전체 삽입)을 검사한다
/// (cpppj <c>client/HtmlWindowMetadata.cpp</c>의 HEAD 계약을 직접 작성했다).
/// </summary>
/// <remarks>2026-11-11 0.348에서 10차 NB08 재대조와 함께 추가했다. 원본 x86 fixture로 대조하지 않으므로 PORT_MAP 상태는 port다.</remarks>
public sealed class HtmlWindowMetadataRepairTests
{
    /// <summary>삽입 시험용 메타 장치를 만든다.</summary>
    /// <param name="node">연결할 부모다.</param><param name="replacement">% 목록 본문이다.</param><param name="repair">NB08 교정 여부다.</param>
    /// <returns>메타 처리기다.</returns>
    private static HtmlWindowMetadata Metadata(HtmlWindowGumpNode node, string replacement, bool repair)
    {
        var hooks = new HtmlWindowMetadataHooks
        {
            Node = _ => node,
            ClearButtons = _ => { },
            Commands = new() { Lookup = _ => 0x02000095U, Unknown = _ => { } },
            StoreText = _ => 0x12000000U,
            BeginMenu = _ => { },
            InlineList = (_, _, _) => replacement,
            RepairInlineList = repair,
        };
        return new HtmlWindowMetadata(hooks);
    }

    /// <summary>버퍼에 NUL 종료 원본을 채운다.</summary>
    /// <param name="source">채울 본문이다.</param>
    /// <returns>8192바이트 버퍼다.</returns>
    private static byte[] Buffer(string source)
    {
        byte[] buffer = new byte[8192];
        Encoding.Latin1.GetBytes(source, buffer);
        buffer[source.Length] = 0;
        return buffer;
    }

    /// <summary>버퍼의 현재 NUL 앞 출력을 읽는다.</summary>
    /// <param name="buffer">읽을 버퍼다.</param>
    /// <returns>출력 본문이다.</returns>
    private static string Output(byte[] buffer) => Encoding.Latin1.GetString(buffer, 0, Array.IndexOf(buffer, (byte)0));

    /// <summary>교정을 켜면 목록 전체와 꼬리가 본문에 남는다.</summary>
    [Fact]
    public void RepairInsertsWholeListWithTail()
    {
        var node = new HtmlWindowGumpNode();
        byte[] buffer = Buffer("A\n$Menu=%list,Tell,X\nB\n");
        Assert.Equal(1U, Metadata(node, "L1\nL2\n", true).Process(1, buffer));
        // 잘린 목록이 아니라 합친 전체와 끝 꼬리다
        Assert.Equal("A\nL1\nL2\nB\n", Output(buffer));
    }

    /// <summary>교정을 끄면 원문처럼 앞에서 replacement 길이만 복사하고 NUL을 다시 쓰지 않아 잘린 목록과 낡은 꼬리가 남는다.</summary>
    [Fact]
    public void OriginalCopiesOnlyReplacementLength()
    {
        var node = new HtmlWindowGumpNode();
        byte[] buffer = Buffer("A\n$Menu=%list,Tell,X\nB\n");
        Assert.Equal(1U, Metadata(node, "L1\nL2\n", false).Process(1, buffer));
        // 합친 전체가 아니라 앞 6바이트만 덮고 낡은 꼬리가 그대로 남는다
        Assert.Equal("A\nL1\nLu=%list,Tell,X\nB\n", Output(buffer));
    }

    /// <summary>교정을 켜도 버퍼를 넘지 않고 넘는 꼬리만 버린다.</summary>
    [Fact]
    public void RepairTruncatesOnlyOverflowingTail()
    {
        var node = new HtmlWindowGumpNode();
        byte[] buffer = Buffer("A\n$Menu=%list,Tell,X\nB\n");
        string big = new string('L', 9000);
        Assert.Equal(1U, Metadata(node, big, true).Process(1, buffer));
        string output = Output(buffer);
        // 8192바이트 물리 안에서만 쓴다
        Assert.Equal(8191, output.Length);
        Assert.Equal("A\n", output[..2]);
    }
}
