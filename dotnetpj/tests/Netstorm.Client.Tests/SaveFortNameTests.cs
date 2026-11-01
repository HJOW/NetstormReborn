using System.Buffers.Binary;
using System.Text;
using Netstorm.Client.Gump;

namespace Netstorm.Client.Tests;

/// <summary>새 Fort 이름 확인·취소의 실제 원본 관측과 부분 쓰기를 비교한다.</summary>
public sealed class SaveFortNameTests
{
    private static string Hex(string text) => Convert.ToHexString(Encoding.Latin1.GetBytes(text)).ToLowerInvariant();

    [Fact]
    public void MatchesNativeConfirmAndCancel()
    {
        string path = Path.Combine(AppContext.BaseDirectory, "X86", "save-fort-name-x86.tsv");
        string[] rows = File.ReadAllLines(path).Where(x => x.Length != 0 && x[0] != '#').ToArray(); Assert.Equal(288, rows.Length);
        foreach (string row in rows)
        {
            string[] col = row.Split('\t'); byte[] header = new byte[20];
            BinaryPrimitives.WriteUInt32LittleEndian(header, col[3] == "0" ? 0u : 0x12345678u); BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(8), uint.Parse(col[4]));
            byte[] input = col[6] == "-" ? [] : Convert.FromHexString(col[6]); var trace = new List<string>(); string name = "";
            var hooks = new SaveFortNameHooks
            {
                CopyText = output => { trace.Add("copy"); input.CopyTo(output); output[input.Length] = 0; },
                Alnum = value => { trace.Add("alnum:" + value); return value is >= (sbyte)'0' and <= (sbyte)'9' or >= (sbyte)'A' and <= (sbyte)'Z' or >= (sbyte)'a' and <= (sbyte)'z'; },
                Path = value => { trace.Add("path:" + Hex(value)); return "d/" + value + ".fort"; },
                Exists = _ => { trace.Add("exists"); return int.Parse(col[5]); }, Header = () => header,
                SetName = value => { name = value; trace.Add($"set:{Hex(value)}:{BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(8))}"); },
                Tell = value => trace.Add("tell:" + value), ReadFort = value => { name = value; trace.Add("store:" + Hex(value)); trace.Add("queue:33554638,287454020,287454020"); }
            };
            uint result = col[0] == "Confirm" ? SaveFortName.Confirm(uint.Parse(col[1]), hooks) : SaveFortName.Cancel(uint.Parse(col[2]), hooks.Tell);
            Assert.Equal(uint.Parse(col[7]), result); Assert.Equal(uint.Parse(col[8]), BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(8)));
            Assert.Equal(col[9], name.Length == 0 ? "-" : Hex(name)); Assert.Equal(col[10], trace.Count == 0 ? "-" : string.Join(';', trace));
        }
    }

    [Fact]
    public void NameChangeFailurePreservesPriorReadonlyClear()
    {
        byte[] header = new byte[20]; BinaryPrimitives.WriteUInt32LittleEndian(header, 1); BinaryPrimitives.WriteUInt32LittleEndian(header.AsSpan(8), 0x12345678);
        var hooks = new SaveFortNameHooks { CopyText = output => { "New"u8.CopyTo(output); output[3] = 0; }, Alnum = _ => true, Path = name => name, Exists = _ => 0, Header = () => header };
        Assert.Throws<InvalidOperationException>(() => SaveFortName.Confirm(1, hooks)); Assert.Equal(0u, BinaryPrimitives.ReadUInt32LittleEndian(header.AsSpan(8)));
        Assert.Equal(1u, SaveFortName.Cancel(1, null));
    }

    [Fact]
    public void ReadsCurrentFortModeAfterExistenceCheck()
    {
        uint mode = 0; bool changed = false;
        var hooks = new SaveFortNameHooks
        {
            CopyText = output => { "New"u8.CopyTo(output); output[3] = 0; }, Alnum = _ => true, Path = name => name,
            Exists = _ => { mode = 1; return 0; }, Header = () => new byte[20], SetName = _ => changed = true
        };
        Assert.Equal(1u, SaveFortName.Confirm(() => mode, hooks)); Assert.True(changed);
    }
}
