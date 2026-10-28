using Netstorm.Client.Display;

namespace Netstorm.Tests;

/// <summary>
/// 원본 기계어 fixture(owner-capture-x86.tsv의 M행 18개)로 표준 소유자 색표 L1 일치를 검사한다.
/// C행 480개(세션 자산·SHP 필요)는 R4·R5 몫이며 여기서는 읽지 않는다.
/// 근거: cpppj/tests/OwnerCaptureTests.cpp. 기대값은 C# 결과로 바꾸지 않는다. (R2c, 2026-10-28)
/// </summary>
public sealed class X86OwnerColorMapsTests
{
    /// <summary>fixture M 행 수(표지만 대조하는 행).</summary>
    private const int Rows = 18;

    /// <summary>원래 72DWORD와 다른 후보·높은 비트·다리 색 2·7의 9×256 전체 원문을 비교한다.</summary>
    [Fact]
    public void NativeAllRowsAndBridgeMatch()
    {
        string[][] rows = X86Fixture.Read("owner-capture");
        // 반환 9표는 입력 후보에서 직접 생성한 뒤에만 기대값을 읽어 비교한다
        int count = 0;
        foreach (string[] row in rows)
        {
            if (row[0] != "M")
            {
                continue;
            }
            Assert.Equal(5, row.Length);
            byte[] input = ColorMapFixture.Bytes(row[3]);
            Assert.Equal(72 * 4, input.Length);
            var candidates = new uint[72];
            for (int i = 0; i < 72; i++)
            {
                candidates[i] = input[i * 4] | ((uint)input[i * 4 + 1] << 8) | ((uint)input[i * 4 + 2] << 16) | ((uint)input[i * 4 + 3] << 24);
            }
            if (count == 0)
            {
                Assert.Equal(OwnerColorMapBuilder.OriginalOwnerColorCandidates(), candidates);
            }
            OwnerColorMaps maps = OwnerColorMapBuilder.BuildStandardOwnerColorMaps(X86Fixture.UInt(row[1]), X86Fixture.UInt(row[2]), candidates);
            var output = new byte[9 * 256];
            for (int owner = 0; owner < 9; owner++)
            {
                Array.Copy(maps.Tables[owner], 0, output, owner * 256, 256);
            }
            Assert.Equal(ColorMapFixture.Bytes(row[4]), output);
            count++;
        }
        Assert.Equal(Rows, count);
    }
}
