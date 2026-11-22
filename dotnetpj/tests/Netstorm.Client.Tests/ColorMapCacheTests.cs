using System.Buffers.Binary;
using System.Globalization;
using System.Text;
using Netstorm.Core.Original;

namespace Netstorm.Client.Tests; // 실제 부모와 독립 원문 관측의 순서/부분 효과를 비교한다.

/// <summary>기대값을 계산하지 않고 같은 명시 장치 입력과130행 전체 출력을 재생한다.</summary>
public sealed class ColorMapCacheTests
{
    /// <summary>명시 장치 중단이다.</summary>
    private sealed class Stop : Exception { }
    /// <summary>열기 marker와 다른 명시 예외다.</summary>
    private sealed class Other : Exception { }
    /// <summary>NUL을 자르지 않는 전체 소문자 hex다.</summary>
    private static string Hex(ReadOnlySpan<byte> bytes) => Convert.ToHexStringLower(bytes);
    /// <summary>명시 십진 입력이다.</summary>
    private static int Number(string value) => int.Parse(value, CultureInfo.InvariantCulture);
    /// <summary>130행이 지정한 raw 입력/장치 효과만 소유한다.</summary>
    private sealed class Device
    {
        /// <summary>선택 진입점과 단일 변경·현재 예외 분류다.</summary>
        public string Entry, Mutation, Pending = "";
        /// <summary>getter가 현재 선택하는 두 실제 primary 물리다.</summary>
        public byte[][] Sources = [new byte[0x3800], new byte[0x3800]];
        /// <summary>현재 primary 번호와 부분 읽기 위치·쓰기 상한이다.</summary>
        public int Current, Position, WriteLimit;
        /// <summary>실제 cleanup과 외부 typed 분배 횟수다.</summary>
        public int Cleanups, Dispatches;
        /// <summary>독립 cleanup 진입점의 첫 호출이다.</summary>
        public bool DirectCleanup;
        /// <summary>각 행에서 현재 읽는 실제8DWORD다.</summary>
        public uint[] Owners = [84, 249, 184, 140, 150, 112, 156, 222];
        /// <summary>현재 실제 출력/reader/writer/경로/부분 파일 바이트다.</summary>
        public byte[] Highlight = new byte[2048], Solid = new byte[2048], Reader = new byte[24], Writer = new byte[24], Path = new byte[256], Input;
        /// <summary>파일 순서의 실제9표다.</summary>
        public byte[][] Maps = Enumerable.Range(0, 9).Select(_ => new byte[256]).ToArray();
        /// <summary>장치가 실제 기록한 호출과 쓰기 접두부다.</summary>
        public List<string> Trace = [];
        /// <summary>장치별 호출 번호다.</summary>
        public Dictionary<string, int> Calls = [];
        /// <summary>실제 쓰인 바이트다.</summary>
        public List<byte> Output = [];
        /// <summary>독립 원문과 동일한 입력만 준비한다.</summary>
        public Device(string input)
        {
            string[] f = input.Split(','); Assert.Equal(6, f.Length); Entry = f[0]; Mutation = f[5]; int profile = Number(f[1]), initial = Number(f[2]); WriteLimit = Number(f[4]);
            double[] values = [0.8, 0.800000011920929, 0.80000002, double.NaN, double.PositiveInfinity, double.NegativeInfinity, -1.0, 1.0];
            // 실제 미사용 필드까지 명시한 source 두 할당이다.
            for (int alternate = 0; alternate < 2; alternate++)
            {
                for (int index = 0; index < Sources[alternate].Length; index++) Sources[alternate][index] = unchecked((byte)(0x37 + index * 13 + alternate * 29)); // 원시 초기 입력이다.
                for (int index = 0; index < 256; index++) BinaryPrimitives.WriteDoubleLittleEndian(Sources[alternate].AsSpan(index * 56 + 8), profile != 0 ? values[(index + alternate) % 8] : index / 255.0); // 명도만 덮는다.
            }
            Name("!color.dat"); Array.Fill(Reader, initial != 0 ? (byte)0xa5 : (byte)0x3c); Reader.CopyTo(Writer, 0);
            for (int index = 0; index < 2048; index++) { Highlight[index] = unchecked((byte)(initial * 31 + index * 11)); Solid[index] = unchecked((byte)(initial * 31 + 7 + index * 11)); } // 미작성 잔여 입력이다.
            for (int map = 0; map < 9; map++) for (int index = 0; index < 256; index++) Maps[map][index] = unchecked((byte)(initial * 13 + map * 19 + index * 5)); // 부분 읽기의 잔여다.
            Input = new byte[Number(f[3])];
            for (int index = 0; index < Input.Length; index++) Input[index] = unchecked((byte)(19 + index * 7)); // 실제 파일 입력이다.
        }
        /// <summary>같은 NUL 경로 할당을 변경한다.</summary>
        private void Name(string text) { int length = Encoding.ASCII.GetBytes(text, Path); Path[length] = 0; }
        /// <summary>호출 순서를 기록하고 현재 번호를 반환한다.</summary>
        private string Event(string kind, string text) { Trace.Add(text); Calls.TryGetValue(kind, out int count); Calls[kind] = ++count; return kind + count; }
        /// <summary>원문 장치 경계에서만 현재 입력 변경/명시 예외를 적용한다.</summary>
        private void Mutate(string boundary)
        {
            int dash = Mutation.IndexOf('-'); if (dash < 0) return; string trigger = Mutation[..dash], action = Mutation[(dash + 1)..];
            string kind = boundary.TrimEnd('0', '1', '2', '3', '4', '5', '6', '7', '8', '9'); if (trigger != boundary && trigger != kind) return;
            switch (action) { case "sourceB": Current = 1; break; case "owner7": Owners[0] = 0x12345607; break; case "pathChanged": Name("changed-color.dat"); break; case "stop": Pending = action; throw new Stop(); case "other": Pending = action; throw new Other(); case "unable": Pending = action; throw new ColorCacheUnableToOpenException(); default: throw new InvalidOperationException(action); }
        }
        /// <summary>배열 내용이 아닌 같은 실제 메모리로 파일 역할을 판별한다.</summary>
        private char Role(Memory<byte> file) => file.Equals(Reader.AsMemory()) ? 'R' : file.Equals(Writer.AsMemory()) ? 'W' : throw new InvalidOperationException("미등록 File 물리");
        /// <summary>현재 같은 실제 출력/입력을 빌린다.</summary>
        public ColorMapCacheGlobals Globals() => new() { Primary = () => Sources[Current], OwnerColors = Owners, OwnerHighlight = Highlight, OwnerSolid = Solid, Maps = Maps.Select(map => map.AsMemory()).ToArray() };
        /// <summary>명시 파일 생성 효과 뒤 변경 장치를 호출한다.</summary>
        private void Open(Memory<byte> file, ReadOnlyMemory<byte> name, uint mode, uint flag)
        {
            char role = Role(file); Assert.Equal(role == 'R' ? 0u : 1u, mode); Assert.Equal(1u, flag); int end = name.Span.IndexOf((byte)0); Assert.True(end >= 0);
            string boundary = Event("open" + role, $"O:{role}:{Hex(file.Span)}:{Hex(name.Span[..end])}");
            for (int index = 0; index < 24; index++) file.Span[index] = (byte)(0x10 + (role == 'W' ? 0x30 : 0) + index); // 같은 파일 물리의 명시 store다.
            Mutate(boundary);
        }
        /// <summary>부모 분기와 표 계산은 제품에 맡기는 실제 장치다.</summary>
        public ColorMapCacheHooks Hooks() => new()
        {
            Path = () => { Mutate(Event("path", "P")); return Path; }, Owners = () => Mutate(Event("owners", "T")),
            BuildMaps = () =>
            {
                if (Pending == "unable" && Entry == "parent") { Dispatches++; Pending = ""; }
                for (int map = 0; map < 9; map++) for (int index = 0; index < 256; index++) Maps[map][index] = unchecked((byte)(0x80 + map * 7 + index * 3)); // 원문 관측의 명시 하위 출력이다.
                Mutate(Event("maps", "M"));
            },
            OpenRead = Open, OpenWrite = Open,
            Read = (file, target, limit) =>
            {
                Assert.Equal('R', Role(file)); Assert.Equal(uint.MaxValue, limit); int map = Array.FindIndex(Maps, bytes => target.Equals(bytes.AsMemory())); Assert.True(map >= 0);
                int count = Math.Min(256, Input.Length - Position); Input.AsMemory(Position, count).CopyTo(target); Position += count; Mutate(Event("read", $"R:{map}:{count}")); return (uint)count;
            },
            Write = (file, target, limit) =>
            {
                Assert.Equal('W', Role(file)); Assert.Equal(uint.MaxValue, limit); int map = Array.FindIndex(Maps, bytes => target.Equals((ReadOnlyMemory<byte>)bytes)); Assert.True(map >= 0);
                int count = Math.Min(256, WriteLimit); Output.AddRange(target.Span[..count].ToArray()); Mutate(Event("write", $"W:{map}:{count}")); return (uint)count;
            },
            Close = file =>
            {
                if (Pending.Length != 0 || DirectCleanup) { Cleanups++; DirectCleanup = false; } char role = Role(file); string boundary = Event("close" + role, $"D:{role}:{Hex(file.Span)}");
                for (int index = 0; index < 24; index++) file.Span[index] = (byte)(0xd0 + (role == 'W' ? 7 : 0) + index); // 같은 객체 정리 효과다.
                Mutate(boundary);
            }
        };
    }
    /// <summary>독립130관측의 모든 표/파일/예외/호출순서를 비교한다.</summary>
    [Fact]
    public void WholeNativeParentCatchAndCleanup130()
    {
        var rows = File.ReadLines(System.IO.Path.Combine(AppContext.BaseDirectory, "X86", "colorsort-cache-parent-x86.tsv")).Where(line => line.Length != 0 && line[0] != '#').Select(line => line.Split('\t')).ToArray(); Assert.Equal(130, rows.Length);
        // 두 정밀도의 각 행을 별도 실제 입력으로 실행한다.
        foreach (var row in rows)
        {
            var device = new Device(row[1]); var globals = device.Globals(); var hooks = device.Hooks(); string completion = "return";
            try
            {
                switch (device.Entry) { case "prefix": RawColorMapCache.Prepare(globals, hooks); completion = "prefix"; break; case "parent": RawColorMapCache.Initialize(globals, hooks, device.Reader, device.Writer); break; case "catch": RawColorMapCache.Regenerate(device.Path, globals, hooks, device.Writer); break; default: device.DirectCleanup = true; RawColorMapCache.Close(device.Entry == "cleanupR" ? device.Reader : device.Writer, hooks); break; }
            }
            catch (Stop) { completion = "stop"; } catch (Other) { completion = "other"; } catch (ColorCacheUnableToOpenException) { completion = "unable"; }
            Assert.Equal(row[2], completion); Assert.Equal(Number(row[3]), device.Cleanups); Assert.Equal(Number(row[4]), device.Dispatches); Assert.Equal(Number(row[5]), device.Current);
            byte[] owners = new byte[32];
            for (int index = 0; index < 8; index++) BinaryPrimitives.WriteUInt32LittleEndian(owners.AsSpan(index * 4), device.Owners[index]); // 원시DWORD 표기다.
            Assert.Equal(row[6], Hex(owners)); Assert.Equal(row[7], Hex(device.Highlight) + Hex(device.Solid)); Assert.Equal(row[8], string.Concat(device.Maps.Select(map => Hex(map)))); Assert.Equal(row[9], Hex(device.Reader) + Hex(device.Writer)); Assert.Equal(row[10], device.Output.Count == 0 ? "-" : Hex(device.Output.ToArray())); Assert.Equal(row[11], device.Trace.Count == 0 ? "-" : string.Join(';', device.Trace)); Assert.True(Number(row[12]) > 0);
        }
    }
    /// <summary>빈 primary는 첫 solid/identity 저장 뒤에만 실패한다.</summary>
    [Fact]
    public void MissingPhysicalSourcePreservesPrefix()
    {
        var device = new Device("prefix,0,1,2304,256,none"); var supplied = device.Globals(); var globals = new ColorMapCacheGlobals { Primary = () => ReadOnlyMemory<byte>.Empty, OwnerColors = supplied.OwnerColors, OwnerHighlight = supplied.OwnerHighlight, OwnerSolid = supplied.OwnerSolid, Maps = supplied.Maps }; byte oldSecond = device.Highlight[1];
        Assert.Throws<ArgumentOutOfRangeException>(() => RawColorMapCache.Prepare(globals, device.Hooks())); Assert.Equal(84, device.Solid[0]); Assert.Equal(0, device.Highlight[0]); Assert.Equal(oldSecond, device.Highlight[1]); Assert.False(device.Calls.ContainsKey("owners"));
    }
    /// <summary>hit은 미선택 재생성 장치가 없어도 성공하며 정확 marker에서만 그 누락을 요구한다.</summary>
    [Fact]
    public void CacheHitRequiresOnlySelectedDevices()
    {
        var hit = new Device("parent,0,0,2304,256,none"); var hooks = hit.Hooks(); hooks.BuildMaps = null; hooks.OpenWrite = null; hooks.Write = null; RawColorMapCache.Initialize(hit.Globals(), hooks, hit.Reader, hit.Writer); Assert.False(hit.Calls.ContainsKey("maps")); Assert.Equal(1, hit.Calls["closeR"]);
        var missing = new Device("parent,0,0,2304,256,openR-unable"); hooks = missing.Hooks(); hooks.BuildMaps = null; Assert.Throws<InvalidOperationException>(() => RawColorMapCache.Initialize(missing.Globals(), hooks, missing.Reader, missing.Writer)); Assert.False(missing.Calls.ContainsKey("closeR")); Assert.False(missing.Calls.ContainsKey("openW"));
    }
}
